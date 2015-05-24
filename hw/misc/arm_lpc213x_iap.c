/*
 * embedded boot code for the lpc213x 
 *
 * 
 * 
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

#define TYPE_ARM_LPC213X_IAP "lpc213x_iap"
#define ARM_LPC213X_IAP(obj) \
    OBJECT_CHECK(arm_lpc213x_iap_state, (obj), TYPE_ARM_LPC213X_IAP)

char iapcode[40] = {0x00,0xaf,0x80,0xb5,
0x00,0x22,0x0b,0x1c,
0x03,0x1c,0x1a,0x60,
0x36,0x2b,0x1b,0x68,
0x0b,0x1c,0x03,0xd1,
0x03,0x4a,0x04,0x33,
0xbd,0x46,0x1a,0x60,
0x01,0xbc,0x80,0xbc,
0xc0,0x46,0x70,0x47,
0x25,0xff,0x02,0x00};

typedef struct {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
} arm_lpc213x_iap_state;

static const VMStateDescription vmstate_arm_lpc213x_iap = {
    .name = "lpc213x_iap",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static void arm_lpc213x_iap_reset(DeviceState *d)
{
    arm_lpc213x_iap_state *s = ARM_LPC213X_IAP(d);
    //int i;
}

static uint64_t arm_lpc213x_iap_read(void *opaque, hwaddr offset,
                                unsigned size)
{
    arm_lpc213x_iap_state *s = (arm_lpc213x_iap_state *)opaque;
    
    switch (offset) {
    default:
    bad_reg:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "arm_lpc2138x_sysctl_read: Bad register offset 0x%x\n",
                      (int)offset);
        return 0;
    }
}

static void arm_lpc213x_iap_write(void *opaque, hwaddr offset,
                             uint64_t val, unsigned size)
{
    arm_lpc213x_iap_state *s = (arm_lpc213x_iap_state *)opaque;
    switch (offset) {

        break;
    default:
    bad_reg:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "arm_lpc213x_iap_write: Bad register offset 0x%x\n",
                      (int)offset);
        return;
    }
}

static const MemoryRegionOps arm_lpc213x_iap_ops = {
    .read = arm_lpc213x_iap_read,
    .write = arm_lpc213x_iap_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void arm_lpc213x_iap_init(Object *obj)
{
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sd = SYS_BUS_DEVICE(obj);
    arm_lpc213x_iap_state *s = ARM_LPC213X_IAP(obj);
    memory_region_init_ram(&s->iomem, OBJECT(dev), "arm-lpc213x-iap", 0x1000, NULL);
    //memory_region_init_io(&s->iomem, OBJECT(dev), &arm_lpc213x_iap_ops, s,"arm-lpc213x-iap", 0x1000);
    //copy shell code to assigned memory 
    sysbus_init_mmio(sd, &s->iomem);
    char *memptr = memory_region_get_ram_ptr(&s->iomem) ;
    memcpy(memptr,iapcode, sizeof(iapcode));
}

static void arm_lpc213x_iap_realize(DeviceState *d, Error **errp)
{
    arm_lpc213x_iap_state *s = ARM_LPC213X_IAP(d);
}

static void arm_lpc213x_iap_finalize(Object *obj)
{
    arm_lpc213x_iap_state *s = ARM_LPC213X_IAP(obj);
    
}

static Property arm_lpc213x_iap_properties[] = {
    //DEFINE_PROP_UINT32("mamcr", arm_lpc213x_iap_state, mamcr, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void arm_lpc213x_iap_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = arm_lpc213x_iap_realize;
    dc->reset = arm_lpc213x_iap_reset;
    dc->vmsd = &vmstate_arm_lpc213x_iap;
    dc->props = arm_lpc213x_iap_properties;
}

static const TypeInfo arm_lpc213x_iap_info = {
    .name          = TYPE_ARM_LPC213X_IAP,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(arm_lpc213x_iap_state),
    .instance_init = arm_lpc213x_iap_init,
    .instance_finalize = arm_lpc213x_iap_finalize,
    .class_init    = arm_lpc213x_iap_class_init,
};

static void arm_lpc213x_iap_register_types(void)
{
    type_register_static(&arm_lpc213x_iap_info);
}

type_init(arm_lpc213x_iap_register_types)
