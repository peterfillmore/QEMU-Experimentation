/*
 * Status and system control registers for ARM RealView/Versatile boards.
 *
 * Copyright (c) 2006-2007 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 */

#include "hw/hw.h"
#include "qemu/timer.h"
#include "qemu/bitops.h"
#include "hw/sysbus.h"
//#include "hw/arm/primecell.h"
#include "sysemu/sysemu.h"

//#define LOCK_VALUE 0xa05f

#define TYPE_ARM_LPC213X_SYSCTL "lpc213x_sysctl"
#define ARM_LPC213X_SYSCTL(obj) \
    OBJECT_CHECK(arm_lpc213x_sysctl_state, (obj), TYPE_ARM_LPC213X_SYSCTL)

typedef struct {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    qemu_irq pl110_mux_ctrl;

    uint32_t mamcr;
    uint32_t mamtim;

    uint32_t extint;
    uint32_t intwake;
    uint32_t extmode;
    uint32_t extpolar;
    
    uint32_t memmap;
    
    uint32_t pllcon;
    uint32_t pllcfg;
    uint32_t pllstat;
    uint32_t pllfeed;
    
    uint32_t pcon;
    uint32_t pconp;
    
    uint32_t apbdiv;
    
    uint32_t rsid;
    
    uint32_t cspr;
    
    uint32_t scs;
} arm_lpc213x_sysctl_state;

static const VMStateDescription vmstate_arm_lpc213x_sysctl = {
    .name = "lpc213x_sysctl",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(mamcr, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(mamtim, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(extint, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(intwake, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(extmode, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(extpolar, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(memmap, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(pllcon, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(pllcfg, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(pllstat, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(pllfeed, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(pcon, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(pconp, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(apbdiv, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(rsid, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(cspr, arm_lpc213x_sysctl_state),
        VMSTATE_UINT32(scs, arm_lpc213x_sysctl_state),
        
        //VMSTATE_UINT32_V(sys_mci, arm_sysctl_state, 2),
        //VMSTATE_UINT32_V(sys_cfgdata, arm_sysctl_state, 2),
        //VMSTATE_UINT32_V(sys_cfgctrl, arm_sysctl_state, 2),
        //VMSTATE_UINT32_V(sys_cfgstat, arm_sysctl_state, 2),
        //VMSTATE_UINT32_V(sys_clcd, arm_sysctl_state, 3),
        //VMSTATE_UINT32_ARRAY_V(mb_clock, arm_sysctl_state, 6, 4),
        //VMSTATE_VARRAY_UINT32(db_clock, arm_sysctl_state, db_num_clocks,
        //                      4, vmstate_info_uint32, uint32_t),
        VMSTATE_END_OF_LIST()
    }
};

/* The PB926 actually uses a different format for
 * its SYS_ID register. Fortunately the bits which are
 * board type on later boards are distinct.
 */
//#define BOARD_ID_PB926 0x100
//#define BOARD_ID_EB 0x140
//#define BOARD_ID_PBA8 0x178
//#define BOARD_ID_PBX 0x182
//#define BOARD_ID_VEXPRESS 0x190


//static int board_id(arm_sysctl_state *s)
//{
//    /* Extract the board ID field from the SYS_ID register value */
//    return (s->sys_id >> 16) & 0xfff;
//}

static void arm_lpc213x_sysctl_reset(DeviceState *d)
{
    arm_lpc213x_sysctl_state *s = ARM_LPC213X_SYSCTL(d);
    //int i;
    s->mamcr = 0;
    s->mamtim = 0x07; 
    s->extint = 0;
    s->intwake = 0;
    s->extmode = 0;
    s->extpolar = 0;
    s->memmap = 0;
    s->pllcon = 0;
    s->pllcfg = 0; 
    s->pllstat = 0x400; 
    s->pllfeed = 0; 
    s->pcon = 0; 
    s->pconp = 0x03BE; /* System bus global clock: 24MHz */
    s->apbdiv = 0; /* IO FPGA reserved clock: 24MHz */
    s->rsid = 0;
    s->cspr = 0;
    s->scs = 0; 
}

static uint64_t arm_lpc213x_sysctl_read(void *opaque, hwaddr offset,
                                unsigned size)
{
    arm_lpc213x_sysctl_state *s = (arm_lpc213x_sysctl_state *)opaque;

    switch (offset) {
    case 0x00:
        return s->mamcr;
    case 0x04:
        return s->mamtim; 
    case 0x140: 
        return s->extint;
    case 0x144: 
        return s->intwake;
    case 0x148: 
        return s->extmode;
    case 0x14C: 
        return s->extpolar;
    case 0x40: 
        return s->memmap;
    case 0x80: 
        return s->pllcon; 
    case 0x84: 
        return s->pllcfg;
    case 0x88:
        return s->pllstat;
    case 0x8C:
        return s->pllfeed;
    case 0xC0:
        return s->pcon;
    case 0xC4:
        return s->pconp;
    case 0x100:
        return s->apbdiv;
    case 0x180:
        return s->rsid;
    case 0x184:
        return s->cspr;
    case 0x1A0:
        return s->scs; 
    default:
    bad_reg:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "arm_lpc2138x_sysctl_read: Bad register offset 0x%x\n",
                      (int)offset);
        return 0;
    }
}

/* SYS_CFGCTRL functions */
//#define SYS_CFG_OSC 1
//#define SYS_CFG_VOLT 2
//#define SYS_CFG_AMP 3
//#define SYS_CFG_TEMP 4
//#define SYS_CFG_RESET 5
//#define SYS_CFG_SCC 6
//#define SYS_CFG_MUXFPGA 7
//#define SYS_CFG_SHUTDOWN 8
//#define SYS_CFG_REBOOT 9
//#define SYS_CFG_DVIMODE 11
//#define SYS_CFG_POWER 12
//#define SYS_CFG_ENERGY 13

/* SYS_CFGCTRL site field values */
//#define SYS_CFG_SITE_MB 0
//#define SYS_CFG_SITE_DB1 1
//#define SYS_CFG_SITE_DB2 2

/**
 * vexpress_cfgctrl_read:
 * @s: arm_sysctl_state pointer
 * @dcc, @function, @site, @position, @device: split out values from
 * SYS_CFGCTRL register
 * @val: pointer to where to put the read data on success
 *
 * Handle a VExpress SYS_CFGCTRL register read. On success, return true and
 * write the read value to *val. On failure, return false (and val may
 * or may not be written to).
 */
//static bool vexpress_cfgctrl_read(arm_sysctl_state *s, unsigned int dcc,
//                                  unsigned int function, unsigned int site,
//                                  unsigned int position, unsigned int device,
//                                  uint32_t *val)
//{
//    /* We don't support anything other than DCC 0, board stack position 0
//     * or sites other than motherboard/daughterboard:
//     */
//    if (dcc != 0 || position != 0 ||
//        (site != SYS_CFG_SITE_MB && site != SYS_CFG_SITE_DB1)) {
//        goto cfgctrl_unimp;
//    }
//
//    switch (function) {
//    case SYS_CFG_VOLT:
//        if (site == SYS_CFG_SITE_DB1 && device < s->db_num_vsensors) {
//            *val = s->db_voltage[device];
//            return true;
//        }
//        if (site == SYS_CFG_SITE_MB && device == 0) {
//            /* There is only one motherboard voltage sensor:
//             * VIO : 3.3V : bus voltage between mother and daughterboard
//             */
//            *val = 3300000;
//            return true;
//        }
//        break;
//    case SYS_CFG_OSC:
//        if (site == SYS_CFG_SITE_MB && device < ARRAY_SIZE(s->mb_clock)) {
//            /* motherboard clock */
//            *val = s->mb_clock[device];
//            return true;
//        }
//        if (site == SYS_CFG_SITE_DB1 && device < s->db_num_clocks) {
//            /* daughterboard clock */
//            *val = s->db_clock[device];
//            return true;
//        }
//        break;
//    default:
//        break;
//    }
//
//cfgctrl_unimp:
//    qemu_log_mask(LOG_UNIMP,
//                  "arm_sysctl: Unimplemented SYS_CFGCTRL read of function "
//                  "0x%x DCC 0x%x site 0x%x position 0x%x device 0x%x\n",
//                  function, dcc, site, position, device);
//    return false;
//}

/**
 * vexpress_cfgctrl_write:
 * @s: arm_sysctl_state pointer
 * @dcc, @function, @site, @position, @device: split out values from
 * SYS_CFGCTRL register
 * @val: data to write
 *
 * Handle a VExpress SYS_CFGCTRL register write. On success, return true.
 * On failure, return false.
 */
//static bool vexpress_cfgctrl_write(arm_sysctl_state *s, unsigned int dcc,
//                                   unsigned int function, unsigned int site,
//                                   unsigned int position, unsigned int device,
//                                   uint32_t val)
//{
//    /* We don't support anything other than DCC 0, board stack position 0
//     * or sites other than motherboard/daughterboard:
//     */
//    if (dcc != 0 || position != 0 ||
//        (site != SYS_CFG_SITE_MB && site != SYS_CFG_SITE_DB1)) {
//        goto cfgctrl_unimp;
//    }
//
//    switch (function) {
//    case SYS_CFG_OSC:
//        if (site == SYS_CFG_SITE_MB && device < ARRAY_SIZE(s->mb_clock)) {
//            /* motherboard clock */
//            s->mb_clock[device] = val;
//            return true;
//        }
//        if (site == SYS_CFG_SITE_DB1 && device < s->db_num_clocks) {
//            /* daughterboard clock */
//            s->db_clock[device] = val;
//            return true;
//        }
//        break;
//    case SYS_CFG_MUXFPGA:
//        if (site == SYS_CFG_SITE_MB && device == 0) {
//            /* Select whether video output comes from motherboard
//             * or daughterboard: log and ignore as QEMU doesn't
//             * support this.
//             */
//            qemu_log_mask(LOG_UNIMP, "arm_sysctl: selection of video output "
//                          "not supported, ignoring\n");
//            return true;
//        }
//        break;
//    case SYS_CFG_SHUTDOWN:
//        if (site == SYS_CFG_SITE_MB && device == 0) {
//            qemu_system_shutdown_request();
//            return true;
//        }
//        break;
//    case SYS_CFG_REBOOT:
//        if (site == SYS_CFG_SITE_MB && device == 0) {
//            qemu_system_reset_request();
//            return true;
//        }
//        break;
//    case SYS_CFG_DVIMODE:
//        if (site == SYS_CFG_SITE_MB && device == 0) {
//            /* Selecting DVI mode is meaningless for QEMU: we will
//             * always display the output correctly according to the
//             * pixel height/width programmed into the CLCD controller.
//             */
//            return true;
//        }
//    default:
//        break;
//    }
//
//cfgctrl_unimp:
//    qemu_log_mask(LOG_UNIMP,
//                  "arm_sysctl: Unimplemented SYS_CFGCTRL write of function "
//                  "0x%x DCC 0x%x site 0x%x position 0x%x device 0x%x\n",
//                  function, dcc, site, position, device);
//    return false;
//}

static void arm_lpc213x_sysctl_write(void *opaque, hwaddr offset,
                             uint64_t val, unsigned size)
{
    arm_lpc213x_sysctl_state *s = (arm_lpc213x_sysctl_state *)opaque;
    switch (offset) {
    case 0x00:
        s->mamcr = val;
        break;
    case 0x04:
        s->mamtim = val;
        break; 
    case 0x140: 
        s->extint = val;
        break;
    case 0x144: 
        s->intwake = val;
        break;
    case 0x148: 
        s->extmode = val;
        break;
    case 0x14C: 
        s->extpolar = val;
        break;
    case 0x40: 
        s->memmap = val;
        break;
    case 0x80: 
        s->pllcon = val; 
        break;
    case 0x84: 
        s->pllcfg = val;
        break; 
    case 0x88:
        s->pllstat = val;
        break;
    case 0x8C:
        s->pllfeed = val; 
        break; 
    case 0xC0:
        s->pcon = val;
        break;
    case 0xC4:
        s->pconp = val;
        break;
    case 0x100:
        s->apbdiv = val;
        break;
    case 0x180:
        s->rsid = val;
        break;
    case 0x184:
        s->cspr = val;
        break;
    case 0x1A0:
        s->scs = val;
        break;
    default:
    bad_reg:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "arm_lpc213x_sysctl_write: Bad register offset 0x%x\n",
                      (int)offset);
        return;
    }
}

static const MemoryRegionOps arm_lpc213x_sysctl_ops = {
    .read = arm_lpc213x_sysctl_read,
    .write = arm_lpc213x_sysctl_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};


//static void arm_sysctl_gpio_set(void *opaque, int line, int level)
//{
//    arm_sysctl_state *s = (arm_sysctl_state *)opaque;
//    switch (line) {
//    case ARM_SYSCTL_GPIO_MMC_WPROT:
//    {
//        /* For PB926 and EB write-protect is bit 2 of SYS_MCI;
//         * for all later boards it is bit 1.
//         */
//        int bit = 2;
//        if ((board_id(s) == BOARD_ID_PB926) || (board_id(s) == BOARD_ID_EB)) {
//            bit = 4;
//        }
//        s->sys_mci &= ~bit;
//        if (level) {
//            s->sys_mci |= bit;
//        }
//        break;
//    }
//    case ARM_SYSCTL_GPIO_MMC_CARDIN:
//        s->sys_mci &= ~1;
//        if (level) {
//            s->sys_mci |= 1;
//        }
//        break;
//    }
//}

static void arm_lpc213x_sysctl_init(Object *obj)
{
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sd = SYS_BUS_DEVICE(obj);
    arm_lpc213x_sysctl_state *s = ARM_LPC213X_SYSCTL(obj);

    memory_region_init_io(&s->iomem, OBJECT(dev), &arm_lpc213x_sysctl_ops, s,"arm-lpc213x-sysctl", 0x1000);
    sysbus_init_mmio(sd, &s->iomem);
    //qdev_init_gpio_in(dev, arm_sysctl_gpio_set, 2);
    //qdev_init_gpio_out(dev, &s->pl110_mux_ctrl, 1);
}

static void arm_lpc213x_sysctl_realize(DeviceState *d, Error **errp)
{
    arm_lpc213x_sysctl_state *s = ARM_LPC213X_SYSCTL(d);

    //s->db_clock = g_new0(uint32_t, s->db_num_clocks);
}

static void arm_lpc213x_sysctl_finalize(Object *obj)
{
    arm_lpc213x_sysctl_state *s = ARM_LPC213X_SYSCTL(obj);

    //g_free(s->db_voltage);
    //g_free(s->db_clock);
    //g_free(s->db_clock_reset);
}

static Property arm_lpc213x_sysctl_properties[] = {
    DEFINE_PROP_UINT32("mamcr", arm_lpc213x_sysctl_state, mamcr, 0),
    DEFINE_PROP_UINT32("mamtim", arm_lpc213x_sysctl_state, mamtim, 0),
    DEFINE_PROP_UINT32("extint", arm_lpc213x_sysctl_state, extint, 0),
    DEFINE_PROP_UINT32("intwake", arm_lpc213x_sysctl_state, intwake, 0),
    DEFINE_PROP_UINT32("extmode", arm_lpc213x_sysctl_state, extmode, 0),
    DEFINE_PROP_UINT32("extpolar", arm_lpc213x_sysctl_state, extpolar, 0),
    DEFINE_PROP_UINT32("memmap", arm_lpc213x_sysctl_state, memmap, 0),
    DEFINE_PROP_UINT32("pllcon", arm_lpc213x_sysctl_state, pllcon, 0),
    DEFINE_PROP_UINT32("pllcfg", arm_lpc213x_sysctl_state, pllcfg, 0),
    DEFINE_PROP_UINT32("pllstat", arm_lpc213x_sysctl_state, pllstat, 0),
    DEFINE_PROP_UINT32("pllfeed", arm_lpc213x_sysctl_state, pllfeed, 0),
    DEFINE_PROP_UINT32("pcon", arm_lpc213x_sysctl_state, pcon, 0),
    DEFINE_PROP_UINT32("pconp", arm_lpc213x_sysctl_state, pconp, 0),
    DEFINE_PROP_UINT32("apbdiv", arm_lpc213x_sysctl_state, apbdiv, 0),
    DEFINE_PROP_UINT32("rsid", arm_lpc213x_sysctl_state, rsid, 0),
    DEFINE_PROP_UINT32("cspr", arm_lpc213x_sysctl_state, cspr, 0),
    DEFINE_PROP_UINT32("scs", arm_lpc213x_sysctl_state, scs, 0),
    /* Daughterboard power supply voltages (as reported via SYS_CFG) */
    //DEFINE_PROP_ARRAY("db-voltage", arm_lpc213x_sysctl_state, db_num_vsensors,
    //                  db_voltage, qdev_prop_uint32, uint32_t),
    /* Daughterboard clock reset values (as reported via SYS_CFG) */
    //DEFINE_PROP_ARRAY("db-clock", arm_lpc213x_sysctl_state, db_num_clocks,
    //                  db_clock_reset, qdev_prop_uint32, uint32_t),
    DEFINE_PROP_END_OF_LIST(),
};

static void arm_lpc213x_sysctl_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = arm_lpc213x_sysctl_realize;
    dc->reset = arm_lpc213x_sysctl_reset;
    dc->vmsd = &vmstate_arm_lpc213x_sysctl;
    dc->props = arm_lpc213x_sysctl_properties;
}

static const TypeInfo arm_lpc213x_sysctl_info = {
    .name          = TYPE_ARM_LPC213X_SYSCTL,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(arm_lpc213x_sysctl_state),
    .instance_init = arm_lpc213x_sysctl_init,
    .instance_finalize = arm_lpc213x_sysctl_finalize,
    .class_init    = arm_lpc213x_sysctl_class_init,
};

static void arm_lpc213x_sysctl_register_types(void)
{
    type_register_static(&arm_lpc213x_sysctl_info);
}

type_init(arm_lpc213x_sysctl_register_types)
