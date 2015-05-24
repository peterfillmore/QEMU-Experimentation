/*
 * ARM PrimeCell Timer modules.
 *
 * Copyright (c) 2005-2006 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 */

#include "hw/sysbus.h"
#include "qemu/timer.h"
#include "sysemu/sysemu.h"

#define DEBUG_LPC213X_TIMER

#ifdef DEBUG_LPC213X_TIMER
#define DPRINTF(fmt, ...) \
do { printf("LPC213X Timer: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#endif


/* Common timer implementation.  */

#define TIMER_CTRL_ONESHOT      (1 << 0)
#define TIMER_CTRL_32BIT        (1 << 1)
#define TIMER_CTRL_DIV1         (0 << 2)
#define TIMER_CTRL_DIV16        (1 << 2)
#define TIMER_CTRL_DIV256       (2 << 2)
#define TIMER_CTRL_IE           (1 << 5)
#define TIMER_CTRL_PERIODIC     (1 << 6)
#define TIMER_CTRL_ENABLE       (1 << 7)

#define TIMER_CTRL_MR0I         (1 << 0)
#define TIMER_CTRL_MR0R         (1 << 1)
#define TIMER_CTRL_MR0S         (1 << 2)
#define TIMER_CTRL_MR1I         (1 << 3)
#define TIMER_CTRL_MR1R         (1 << 4)
#define TIMER_CTRL_MR1S         (1 << 5)
#define TIMER_CTRL_MR2I         (1 << 6)
#define TIMER_CTRL_MR2R         (1 << 7)
#define TIMER_CTRL_MR2S         (1 << 8)
#define TIMER_CTRL_MR3I         (1 << 9)
#define TIMER_CTRL_MR3R         (1 << 10)
#define TIMER_CTRL_MR3S         (1 << 11)

#define TIMER_CTRL_CAP0RE       (1 << 0)
#define TIMER_CTRL_CAP0FE       (1 << 1)
#define TIMER_CTRL_CAP0I        (1 << 2)
#define TIMER_CTRL_CAP1RE       (1 << 3)
#define TIMER_CTRL_CAP1FE       (1 << 4)
#define TIMER_CTRL_CAP1I        (1 << 5)
#define TIMER_CTRL_CAP2RE       (1 << 6)
#define TIMER_CTRL_CAP2FE       (1 << 7)
#define TIMER_CTRL_CAP2I        (1 << 8)
#define TIMER_CTRL_CAP3RE       (1 << 9)
#define TIMER_CTRL_CAP3FE       (1 << 10)
#define TIMER_CTRL_CAP3I        (1 << 11)

#define TYPE_LPC213X_TIMER "lpc213x-timer"
#define LPC213X_TIMER(obj) OBJECT_CHECK(LPC213X_TIMERState, (obj), TYPE_LPC213X_TIMER)

typedef struct LPC213X_TIMERState{
    SysBusDevice parent_obj;
    
    MemoryRegion iomem;
    qemu_irq irq;

    uint32_t tick_offset_vmstate;
    uint32_t tick_offset;

    //ptimer_state *timer;
    uint32_t IR;
    uint32_t TCR;
    uint32_t TC;
    uint32_t PR;
    uint32_t PC;
    uint32_t MCR;
    uint32_t MR0;
    uint32_t MR1;
    uint32_t MR2;
    uint32_t MR3;
    uint32_t CCR;
    uint32_t CR0;
    uint32_t CR1;
    uint32_t CR2;
    uint32_t CR3;
    uint32_t EMR;
    uint32_t CTCR;

    //uint32_t control;
    //uint32_t limit;
    //uint8_t int_register; 
    //int freq;
    //int int_level[8]; //8 interrtupes
    //qemu_irq mr_irq[4]; //match control
    //qemu_irq cr_irq[4]; //capture control
    QEMUTimer *matchtimer[4];
    QEMUTimer *capturetimer[4]; 
    QEMUTimer *prescalertimer;
} LPC213X_TIMERState;

/* Check all active timers, and schedule the next timer interrupt.  */

static void lpc213x_timer_update(LPC213X_TIMERState *s)
{   
    //check interrupt register 
    int intstate = 0x00; //check for interrupt 
    for(int i=0; i < 4; i++){
        intstate |=  (s->IR && (1 << i)) && (s->MCR && (1 << (i*3)));
        intstate |=  (s->IR && (16 << i)) && (s->CCR && (4 << (i*3)));
    }
    qemu_set_irq(s->irq, intstate);
}

static void lpc213x_timer_interrupt_MR0(void *opaque){
    LPC213X_TIMERState *s = (LPC213X_TIMERState *) opaque;
    s->IR = 1 << 0;
    DPRINTF("Match Channel 0 Alarm raised\n");
    lpc213x_timer_update(s);
}   
static void lpc213x_timer_interrupt_MR1(void *opaque){
    LPC213X_TIMERState *s = (LPC213X_TIMERState *) opaque;
    s->IR = 1 << 1;
    DPRINTF("Match Channel 1 Alarm raised\n");
    lpc213x_timer_update(s);
}
static void lpc213x_timer_interrupt_MR2(void *opaque){
    LPC213X_TIMERState *s = (LPC213X_TIMERState *) opaque;
    s->IR = 1 << 2;
    DPRINTF("Match Channel 2 Alarm raised\n");
    lpc213x_timer_update(s);
}  
static void lpc213x_timer_interrupt_MR3(void *opaque){
    LPC213X_TIMERState *s = (LPC213X_TIMERState *) opaque;
    s->IR = 1 << 3;
    DPRINTF("Match Channel 3 Alarm raised\n");
    lpc213x_timer_update(s);
}
static void lpc213x_timer_interrupt_CR0(void *opaque){
    LPC213X_TIMERState *s = (LPC213X_TIMERState *) opaque;
    s->IR = 1 << 4;
    DPRINTF("Capture Channel 0 Alarm raised\n");
    lpc213x_timer_update(s);
}   
static void lpc213x_timer_interrupt_CR1(void *opaque){
    LPC213X_TIMERState *s = (LPC213X_TIMERState *) opaque;
    s->IR = 1 << 5;
    DPRINTF("Capture Channel 1 Alarm raised\n");
    lpc213x_timer_update(s);
}
static void lpc213x_timer_interrupt_CR2(void *opaque){
    LPC213X_TIMERState *s = (LPC213X_TIMERState *) opaque;
    s->IR = 1 << 6;
    DPRINTF("Capture Channel 2 Alarm raised\n");
    lpc213x_timer_update(s);
}  
static void lpc213x_timer_interrupt_CR3(void *opaque){
    LPC213X_TIMERState *s = (LPC213X_TIMERState *) opaque;
    s->IR = 1 << 7;
    DPRINTF("Capture Channel 3 Alarm raised\n");
    lpc213x_timer_update(s);
}


//get timer counter
static uint32_t lpc213x_timer_get_tc_count(LPC213X_TIMERState *s){
    return s->TC;
}

//get prescaler count
static uint32_t lpc213x_timer_get_pc_count(LPC213X_TIMERState *s){
    return (int32_t)(qemu_clock_get_ns(rtc_clock) & 0xFFFFFFFF); 
}

static void lpc213x_timer_set_alarm_PC(LPC213X_TIMERState *s){
    //uint32_t ticks;
    s->PC = s->PR - lpc213x_timer_get_pc_count(s);
    if(s->PC == 0){
        s->TC++;
        s->PC = s->PR;
    } else {
        int64_t now = qemu_clock_get_ns(rtc_clock);
        timer_mod(s->prescalertimer, now + (int64_t)s->PC * get_ticks_per_sec()); 
   } 
}

//interrupt function for the Prescaler
static void lpc213x_timer_interrupt_PC(void *opaque){
    LPC213X_TIMERState *s = (LPC213X_TIMERState *) opaque;
    lpc213x_timer_set_alarm_PC(s);
}

static void lpc213x_timer_set_alarm_MR0(LPC213X_TIMERState *s){
    uint32_t ticks;
    ticks = s->MR0 - lpc213x_timer_get_tc_count(s);
    DPRINTF("MR0 alarm set in %ud ticks\n", ticks);
    if(ticks == 0){
        timer_del(s->matchtimer[0]);
        lpc213x_timer_interrupt_MR0(s);
    } else {
        uint32_t now = (int32_t)qemu_clock_get_ns(rtc_clock) & 0xFFFFFFFF;
        timer_mod(s->matchtimer[0], (uint64_t)now + ticks);
    }
}

static void lpc213x_timer_set_alarm_MR1(LPC213X_TIMERState *s){
    uint32_t ticks;
    ticks = s->MR1 - lpc213x_timer_get_tc_count(s);
    DPRINTF("MR1 alarm set in %ud ticks\n", ticks);
    if(ticks == 0){
        timer_del(s->matchtimer[0]);
        lpc213x_timer_interrupt_MR1(s);
    } else {
        uint32_t now = (int32_t)qemu_clock_get_ns(rtc_clock) & 0xFFFFFFFF;
        timer_mod(s->matchtimer[0], (uint64_t)now + ticks);
    }
}
static void lpc213x_timer_set_alarm_MR2(LPC213X_TIMERState *s){
    uint32_t ticks;
    ticks = s->MR2 - lpc213x_timer_get_tc_count(s);
    DPRINTF("MR2 alarm set in %ud ticks\n", ticks);
    if(ticks == 0){
        timer_del(s->matchtimer[0]);
        lpc213x_timer_interrupt_MR2(s);
    } else {
        uint32_t now = (int32_t)qemu_clock_get_ns(rtc_clock) & 0xFFFFFFFF;
        timer_mod(s->matchtimer[0], (uint64_t)now + ticks);
    }
}
static void lpc213x_timer_set_alarm_MR3(LPC213X_TIMERState *s){
    uint32_t ticks;
    ticks = s->MR3 - lpc213x_timer_get_tc_count(s);
    DPRINTF("MR3 alarm set in %ud ticks\n", ticks);
    if(ticks == 0){
        timer_del(s->matchtimer[0]);
        lpc213x_timer_interrupt_MR3(s);
    } else {
        uint32_t now = (int32_t)qemu_clock_get_ns(rtc_clock) & 0xFFFFFFFF;
        timer_mod(s->matchtimer[0], (uint64_t)now + ticks);
    }
}
static void lpc213x_timer_set_alarm_CR0(LPC213X_TIMERState *s){
    uint32_t ticks;
    ticks = s->CR0 - lpc213x_timer_get_tc_count(s);
    DPRINTF("CR0 alarm set in %ud ticks\n", ticks);
    if(ticks == 0){
        timer_del(s->matchtimer[0]);
        lpc213x_timer_interrupt_CR0(s);
    } else {
        uint32_t now = (int32_t)qemu_clock_get_ns(rtc_clock) & 0xFFFFFFFF;
        timer_mod(s->matchtimer[0], (uint64_t)now + ticks);
    }
}

static void lpc213x_timer_set_alarm_CR1(LPC213X_TIMERState *s){
    uint32_t ticks;
    ticks = s->CR1 - lpc213x_timer_get_tc_count(s);
    DPRINTF("CR1 alarm set in %ud ticks\n", ticks);
    if(ticks == 0){
        timer_del(s->matchtimer[0]);
        lpc213x_timer_interrupt_CR1(s);
    } else {
        uint32_t now = (int32_t)qemu_clock_get_ns(rtc_clock) & 0xFFFFFFFF;
        timer_mod(s->matchtimer[0], (uint64_t)now + ticks);
    }
}
static void lpc213x_timer_set_alarm_CR2(LPC213X_TIMERState *s){
    uint32_t ticks;
    ticks = s->CR2 - lpc213x_timer_get_tc_count(s);
    DPRINTF("CR2 alarm set in %ud ticks\n", ticks);
    if(ticks == 0){
        timer_del(s->matchtimer[0]);
        lpc213x_timer_interrupt_CR2(s);
    } else {
        uint32_t now = (int32_t)qemu_clock_get_ns(rtc_clock) & 0xFFFFFFFF;
        timer_mod(s->matchtimer[0], (uint64_t)now + ticks);
    }
}
static void lpc213x_timer_set_alarm_CR3(LPC213X_TIMERState *s){
    uint32_t ticks;
    ticks = s->CR3 - lpc213x_timer_get_tc_count(s);
    DPRINTF("CR3 alarm set in %ud ticks\n", ticks);
    if(ticks == 0){
        timer_del(s->matchtimer[0]);
        lpc213x_timer_interrupt_CR3(s);
    } else {
        uint32_t now = (int32_t)qemu_clock_get_ns(rtc_clock) & 0xFFFFFFFF;
        timer_mod(s->matchtimer[0], (uint64_t)now + ticks);
    }
}


static uint64_t lpc213x_timer_read(void *opaque, hwaddr offset, unsigned size)
{
    LPC213X_TIMERState *s = (LPC213X_TIMERState *)opaque;
     
    switch (offset >> 2) {
    case 0: /* IR */
        return s->IR;
    case 1: /* TCR */
        return s->TCR;
    case 2: /* TC  */
        return s->TC;
    case 3: /* PR */
        return s->PR; 
    case 4: /* PC */
        return s->PC;
    case 5: /* MCR */
        return s->MCR;
    case 6: /* MR0 */
        return s->MR0;
    case 7: /* MR0 */
        return s->MR1;
    case 8: /* MR0 */
        return s->MR2;
    case 9: /* MR0 */
        return s->MR3;
    case 10: /* CCR */
        return s->CCR;
    case 11: /* CR0 */
        return s->CR0;
    case 12: /* CR1 */
        return s->CR1;
    case 13: /* CR2 */
        return s->CR2;
    case 14: /* CR3 */
        return s->CR3;
    case 15: /*EMR */
        return s->EMR;
    case 16: /*CTCR*/
        return s->CTCR;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset %x\n", __func__, (int)offset);
        return 0;
    }
}

static void lpc213x_timer_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    LPC213X_TIMERState *s = (LPC213X_TIMERState *) opaque;
    switch (offset >> 2) {
        case 0: /* IR */
            s->IR = (uint32_t) value & 0xFFFFFFFF;
            lpc213x_timer_update(s);
            break; 
        case 1: /* TCR */
            s->TCR = (uint32_t) value & 0xFFFFFFFF;
            lpc213x_timer_update(s);
            break; 
        case 2: /* TC  */
            s->TC = (uint32_t) value & 0xFFFFFFFF;
            lpc213x_timer_update(s);
            break;
        case 3: /* PR */
            s->PR = (uint32_t) value & 0xFFFFFFFF;  
            lpc213x_timer_update(s);
            break; 
        case 4: /* PC */
            s->PC = (uint32_t) value & 0xFFFFFFFF;  
            //s->tick_offset += value - lpc213x_timer_get_pc_count(s);
            lpc213x_timer_update(s);
            break;    
        case 5: /* MCR */
            s->MCR = (uint32_t) value & 0xFFFFFFFF; 
            lpc213x_timer_update(s);
            break;    
        case 6: /* MR0 */
            s->MR0 = (uint32_t) value & 0xFFFFFFFF; 
            lpc213x_timer_update(s);
            break;
        case 7: /* MR1 */
            s->MR1 = (uint32_t) value & 0xFFFFFFFF; 
            lpc213x_timer_update(s);
            break;
        case 8: /* MR2 */
            s->MR2 = (uint32_t) value & 0xFFFFFFFF; 
            lpc213x_timer_update(s);
            break;
        case 9: /* MR3 */
            s->MR3 = (uint32_t) value & 0xFFFFFFFF; 
            lpc213x_timer_update(s);
            break;
        case 10: /* CCR */
            s->CCR = (uint32_t) value & 0xFFFFFFFF; 
            lpc213x_timer_update(s);
            break;
        case 11: /* CR0 */
            qemu_log_mask(LOG_GUEST_ERROR,
                          "lpc213x_timer: write to read-only register at offset 0x%x\n", (int)offset);
            break;
        case 12: /* CR1 */
            qemu_log_mask(LOG_GUEST_ERROR,
                          "lpc213x_timer: write to read-only register at offset 0x%x\n", (int)offset);
            break;
        case 13: /* CR2 */
            qemu_log_mask(LOG_GUEST_ERROR,
                          "lpc213x_timer: write to read-only register at offset 0x%x\n", (int)offset);
            break;
        case 14: /* CR3 */
            qemu_log_mask(LOG_GUEST_ERROR,
                          "lpc213x_timer: write to read-only register at offset 0x%x\n", (int)offset);
            break;
        case 15: /*EMR */
            s->EMR = (uint32_t) value & 0xFFFFFFFF; 
            lpc213x_timer_update(s);
            break;
        case 16: /*CTCR*/
            s->CTCR = (uint32_t) value & 0xFFFFFFFF; 
            lpc213x_timer_update(s);
            break;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "lpc213x_timer: write to bad offset 0x%x\n", (int)offset);
            break;     
    }          
}

static const MemoryRegionOps lpc213x_timer_ops = {
    .read = lpc213x_timer_read,
    .write = lpc213x_timer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int lpc213x_timer_init(SysBusDevice *dev)
{
    LPC213X_TIMERState *s = LPC213X_TIMER(dev);
    memory_region_init_io(&s->iomem, OBJECT(s), &lpc213x_timer_ops, s, "lpc213x-timer", 0x1000);
    sysbus_init_mmio(dev, &s->iomem);
    sysbus_init_irq(dev, &s->irq);

    /* 
    //initialize match irqs and timers 
    for(int i = 0; i < 4; i++){
        sysbus_init_irq(dev, &s->mr_irq[i]);
    }
    s->matchtimer[0] = timer_new_ns(rtc_clock, lpc213x_timer_interrupt_MR0, s);
    s->matchtimer[1] = timer_new_ns(rtc_clock, lpc213x_timer_interrupt_MR1, s);
    s->matchtimer[2] = timer_new_ns(rtc_clock, lpc213x_timer_interrupt_MR2, s);
    s->matchtimer[3] = timer_new_ns(rtc_clock, lpc213x_timer_interrupt_MR3, s);

    //initialize capture irqs and timers 
    for(int i = 0; i < 4; i++){
        sysbus_init_irq(dev, &s->cr_irq[i]);
    }
    s->capturetimer[0] = timer_new_ns(rtc_clock, lpc213x_timer_interrupt_CR0, s);
    s->capturetimer[1] = timer_new_ns(rtc_clock, lpc213x_timer_interrupt_CR1, s);
    s->capturetimer[2] = timer_new_ns(rtc_clock, lpc213x_timer_interrupt_CR2, s);
    s->capturetimer[3] = timer_new_ns(rtc_clock, lpc213x_timer_interrupt_CR3, s);
    s->prescalertimer = timer_new_ns(rtc_clock, lpc213x_timer_interrupt_PC, s);
    */
    return 0;
} 

static int lpc213x_timer_post_load(void *opaque, int version_id)
{
    LPC213X_TIMERState *s = opaque;
    
    lpc213x_timer_set_alarm_PC(s);   //start prescaler counter 
    lpc213x_timer_set_alarm_MR0(s); 
    lpc213x_timer_set_alarm_MR1(s);
    lpc213x_timer_set_alarm_MR2(s);
    lpc213x_timer_set_alarm_MR3(s);
    lpc213x_timer_set_alarm_CR0(s);
    lpc213x_timer_set_alarm_CR1(s);
    lpc213x_timer_set_alarm_CR2(s);
    lpc213x_timer_set_alarm_CR3(s);

    return 0;
}
static const VMStateDescription vmstate_lpc213x_timer = {
    .name = "lpc213x-timer",
    .version_id = 1,
    .minimum_version_id = 1,
    .post_load = lpc213x_timer_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(IR, LPC213X_TIMERState),
        VMSTATE_UINT32(TCR, LPC213X_TIMERState),
        VMSTATE_UINT32(PR, LPC213X_TIMERState),
        VMSTATE_UINT32(PC, LPC213X_TIMERState),
        VMSTATE_UINT32(MCR, LPC213X_TIMERState),
        VMSTATE_UINT32(MR0, LPC213X_TIMERState),
        VMSTATE_UINT32(MR1, LPC213X_TIMERState),
        VMSTATE_UINT32(MR2, LPC213X_TIMERState),
        VMSTATE_UINT32(MR3, LPC213X_TIMERState),
        VMSTATE_UINT32(CCR, LPC213X_TIMERState),
        VMSTATE_UINT32(CR0, LPC213X_TIMERState),
        VMSTATE_UINT32(CR1, LPC213X_TIMERState),
        VMSTATE_UINT32(CR2, LPC213X_TIMERState),
        VMSTATE_UINT32(CR3, LPC213X_TIMERState),
        VMSTATE_UINT32(EMR, LPC213X_TIMERState),
        VMSTATE_UINT32(CTCR, LPC213X_TIMERState),
        VMSTATE_END_OF_LIST()
    }
};



static void lpc213x_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = lpc213x_timer_init;
    dc->vmsd = &vmstate_lpc213x_timer;
}

static const TypeInfo lpc213x_timer_info = {
    .name       = TYPE_LPC213X_TIMER,
    .parent     = TYPE_SYS_BUS_DEVICE,
    .instance_size   = sizeof(LPC213X_TIMERState),
    .class_init = lpc213x_timer_class_init,
};

static void lpc213x_timer_register_types(void){
    type_register_static(&lpc213x_timer_info);
}

type_init(lpc213x_timer_register_types)



