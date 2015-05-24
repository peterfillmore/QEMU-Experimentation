/*
 *  GPIO Controller for NXP LPC 213x SoCs 
 * 
 * Copyright (C) 2014 Peter Fillmore. All rights reserved.
 *
 * Author: Peter Fillmore, <peter@peterfillmore.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "hw/sysbus.h"

#define TYPE_LPC213X_GPIO "lpc213x_gpio"
#define LPC213X_GPIO(OBJ) OBJECT_CHECK(LPC213XGPIOState, (OBJ), TYPE_LPC213X_GPIO)

typedef struct LPC213XGPIOState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    //qemu_irq irq;
    //qemu_irq out[32];

    uint32_t iopin;
    uint32_t ioset;
    uint32_t iodir;
    uint32_t ioclr;
} LPC213XGPIOState;

static const VMStateDescription vmstate_lpc213x_gpio = {
    .name = "lpc213x_gpio",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(iopin, LPC213XGPIOState),
        VMSTATE_UINT32(ioset, LPC213XGPIOState),
        VMSTATE_UINT32(iodir, LPC213XGPIOState),
        VMSTATE_UINT32(ioclr, LPC213XGPIOState),
        VMSTATE_END_OF_LIST()
    }
};

static void lpc213x_gpio_update(LPC213XGPIOState *s)
{
    //qemu_set_irq(s->irq, !!(s->ier & s->imr));
}

static uint64_t lpc213x_gpio_read(void *opaque, hwaddr offset,
                                  unsigned size)
{
    LPC213XGPIOState *s = (LPC213XGPIOState *)opaque;

    if (size != 4) {
        /* All registers are 32bit */
        return 0;
    }

    switch (offset) {
    case 0x0: /* IOPIN - current data */
        return s->iopin;
    case 0x4: /* IOSET - set high */
        return s->ioset;
    case 0x8: /* IODIR - set directions */
        return s->iodir;
    case 0xC: /* IOCLEAR - Clear pin */
        return s->ioclr;
    default:
        return 0;
    }
}

static void lpc213x_write_data(LPC213XGPIOState *s, uint32_t new_data)
{
    uint32_t old_data = s->iopin;
    uint32_t diff = old_data ^ new_data;
    int i;

    for (i = 0; i < 32; i++) {
        uint32_t mask = 1 << i;
        if (!(diff & mask)) {
            continue;
        }

        //if (s->dir & mask) {
        //    /* Output */
        //    qemu_set_irq(s->out[i], (new_data & mask) != 0);
        //}
    }

    s->iopin = new_data;
}

static void lpc213x_gpio_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    LPC213XGPIOState *s = (LPC213XGPIOState *)opaque;

    if (size != 4) {
        /* All registers are 32bit */
        return;
    }

    switch (offset) {
    case 0x0: /* IOPIN */
        lpc213x_write_data(s, value);
        break;
    case 0x4: /* IOSET */
        s->ioset |= value;
        break;
    case 0x8: /* IODIR */
        s->iodir = value;
        break;
    case 0xC: /* IOCLR */
        s->ioclr &= ~value;
        break;
    }
    lpc213x_gpio_update(s);
}

static void lpc213x_gpio_reset(LPC213XGPIOState *s)
{
    s->iopin = 0x800;
    s->ioset = 0;
    s->iodir = 0;
    s->ioclr = 0;
}

//static void lpc213x_gpio_set_irq(void * opaque, int irq, int level)
//{
//    lpc213xGPIOState *s = (lpc213xGPIOState *)opaque;
//    uint32_t mask;
//
//    mask = 0x80000000 >> irq;
//    if ((s->dir & mask) == 0) {
//        uint32_t old_value = s->dat & mask;
//
//        s->dat &= ~mask;
//        if (level)
//            s->dat |= mask;
//
//        if (!(s->icr & irq) || (old_value && !level)) {
//            s->ier |= mask;
//        }
//
//        lpc213x_gpio_update(s);
//    }
//}

static const MemoryRegionOps lpc213x_gpio_ops = {
    .read = lpc213x_gpio_read,
    .write = lpc213x_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int lpc213x_gpio_initfn(SysBusDevice *sbd)
{
    DeviceState *dev = DEVICE(sbd);
    LPC213XGPIOState *s = LPC213X_GPIO(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &lpc213x_gpio_ops, s, "lpc213x_gpio", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    //sysbus_init_irq(sbd, &s->irq);
    //qdev_init_gpio_in(dev, lpc213x_gpio_set_irq, 32);
    //qdev_init_gpio_out(dev, s->out, 32);
    lpc213x_gpio_reset(s);
    return 0;
}
static Property lpc213x_gpio_properties[] = {
    DEFINE_PROP_UINT32("ioset", LPC213XGPIOState, ioset, 0),
    DEFINE_PROP_UINT32("iodir", LPC213XGPIOState, iodir, 0),
    DEFINE_PROP_UINT32("ioclr", LPC213XGPIOState, ioclr, 0),
    DEFINE_PROP_UINT32("iopin", LPC213XGPIOState, iopin, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void lpc213x_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = lpc213x_gpio_initfn;
    dc->vmsd = &vmstate_lpc213x_gpio;
    dc->props = lpc213x_gpio_properties;
}

static const TypeInfo lpc213x_gpio_info = {
    .name          = TYPE_LPC213X_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(LPC213XGPIOState),
    .instance_init = lpc213x_gpio_initfn,
    .class_init    = lpc213x_gpio_class_init,
};

static void lpc213x_gpio_register_types(void)
{
    type_register_static(&lpc213x_gpio_info);
}

type_init(lpc213x_gpio_register_types)
