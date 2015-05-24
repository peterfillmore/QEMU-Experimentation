/*
 * ARM vivotech Platform/Application Baseboard System emulation.
 *
 * Copyright (c) 2005-2007 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 */
#include <stdio.h>

#include "hw/sysbus.h"
#include "hw/arm/arm.h"
#include "hw/devices.h"
#include "net/net.h"
#include "sysemu/sysemu.h"
#include "hw/pci/pci.h"
#include "hw/i2c/i2c.h"
#include "hw/boards.h"
#include "sysemu/block-backend.h"
#include "exec/address-spaces.h"
#include "hw/block/flash.h"
#include "qemu/error-report.h"

//setup LPC2138 flash
#define VIVOTECH_FLASH_ADDR 0x00000000
#define VIVOTECH_FLASH_SIZE (512 * 1024)
#define VIVOTECH_FLASH_SECT_SIZE 512  
#define VIVOTECH_RAM_ADDR 0x40000000
#define VIVOTECH_RAM_SIZE (64 * 1024)

char iapcode[41] = {
0x01,0x68,0x36,0x29,
0xc0,0x46,0xc0,0x46,
0xc0,0x46,0xc0,0x46,
0xc0,0x46,0xc0,0x46,
0x03,0xd1,0x03,0x4a,
0x62,0x60,0x70,0x47,
0xc0,0x46,0x70,0x47,
0xc0,0x46,0xc0,0x46,
0x25,0xff,0x02,0x00};

/* Board init.  */

/* The AB and PB boards both use the same core, just with different
   peripherals and expansion busses.  For now we emulate a subset of the
   PB peripherals and just change the board ID.  */

static struct arm_boot_info vivotech_binfo;

static void vivotech_init(MachineState *machine, int board_id)
{
    FILE *ramfile;
    char ramdata[VIVOTECH_RAM_SIZE];
 
    ObjectClass *cpu_oc;
    Object *cpuobj;
    ARMCPU *cpu;
    MemoryRegion *sysmem = get_system_memory();
    MemoryRegion *ram = g_new(MemoryRegion, 1);
    MemoryRegion *iap = g_new(MemoryRegion, 1);
 
    qemu_irq pic[32]; //primary interrupt controller
    DeviceState *dev, *sysctl, *pllctl, *gpio0, *gpio1;//, *iap;
    SysBusDevice *busdev;
    DeviceState *pl041;
    NICInfo *nd;
    I2CBus *i2c;
    int n;
    int done_smc = 0;
    DriveInfo *dinfo;
    Error *err = NULL;

    if (!machine->cpu_model) {
        machine->cpu_model = "arm926";
    }

    cpu_oc = cpu_class_by_name(TYPE_ARM_CPU, machine->cpu_model);
    if (!cpu_oc) {
        fprintf(stderr, "Unable to find CPU definition\n");
        exit(1);
    }
     
    cpuobj = object_new(object_class_get_name(cpu_oc));

    /* By default ARM1176 CPUs have EL3 enabled.  This board does not
     * currently support EL3 so the CPU EL3 property is disabled before
     * realization.
     */
    if (object_property_find(cpuobj, "has_el3", NULL)) {
        object_property_set_bool(cpuobj, false, "has_el3", &err);
        if (err) {
            error_report("%s", error_get_pretty(err));
            exit(1);
        }
    }

    object_property_set_bool(cpuobj, true, "realized", &err);
    if (err) {
        error_report("%s", error_get_pretty(err));
        exit(1);
    }

    cpu = ARM_CPU(cpuobj);

    memory_region_init_ram(ram, NULL, "vivotech.ram", machine->ram_size,
                          &error_abort);
    vmstate_register_ram_global(ram);
        /* ??? RAM should repeat to fill physical memory space.  */
    /* SDRAM at address zero.  */
    memory_region_add_subregion(sysmem, VIVOTECH_RAM_ADDR, ram);
    
    sysctl = qdev_create(NULL, "lpc213x_sysctl");
    qdev_prop_set_uint32(sysctl, "pllstat", 0x400); 
    qdev_init_nofail(sysctl);
    sysbus_mmio_map(SYS_BUS_DEVICE(sysctl), 0, 0xE01FC000);

    //vectored interrupt controller
    dev = sysbus_create_varargs("pl190", 0xFFFFF000,
                                qdev_get_gpio_in(DEVICE(cpu), ARM_CPU_IRQ), //interrupt request
                                qdev_get_gpio_in(DEVICE(cpu), ARM_CPU_FIQ), //fast interrupt request
                                NULL);
    for(n = 0; n < 32; n++) { 
        pic[n] = qdev_get_gpio_in(dev, n);
    }
    //configure gpios
    gpio0 = qdev_create(NULL, "lpc213x_gpio");
    qdev_prop_set_uint32(gpio0, "iopin", 0x800); 
    qdev_init_nofail(gpio0);
    sysbus_mmio_map(SYS_BUS_DEVICE(gpio0), 0, 0xE0028000);
    
    gpio1 = qdev_create(NULL, "lpc213x_gpio");
    //qdev_prop_set_uint32(gpio1, "ioset", 0x800); 
    qdev_init_nofail(gpio1);
    sysbus_mmio_map(SYS_BUS_DEVICE(gpio1), 0, 0xE0028010);

    memory_region_init_ram(iap, NULL, "vivotech.iap", 0x2000,
                          &error_abort);
    vmstate_register_ram_global(iap);
    //vmstate_register_ram(iap
    //memory_region_add_subregion(sysmem, 0x7FFFFFF0, iap);
    memory_region_add_subregion(sysmem, 0x7FFFF000, iap);
    
    //copy data into the memory address 
    char *ramptr = (char *)memory_region_get_ram_ptr(iap);
    if(ramptr != NULL) 
        memcpy(ramptr+0xFF0, iapcode, sizeof(iapcode));
    
    //initialize timer   
    sysbus_create_simple("lpc213x-timer",  0xE0004000, pic[4]);
  
    
    
    //uart stuff 
    //sysbus_create_simple("pl011", 0x101f1000, pic[12]);
    //sysbus_create_simple("pl011", 0x101f2000, pic[13]);
    //sysbus_create_simple("pl011", 0x101f3000, pic[14]);
    //sysbus_create_simple("pl011", 0x10009000, sic[6]);
    //DMA controller
    //sysbus_create_simple("pl080", 0x10130000, pic[17]);
    //timer module 
    //sysbus_create_simple("sp804", 0x101e2000, pic[4]);
    //sysbus_create_simple("sp804", 0x101e3000, pic[5]);
    //gpio
    //sysbus_create_simple("pl061", 0x101e4000, pic[6]);
    //sysbus_create_simple("pl061", 0x101e5000, pic[7]);
    //sysbus_create_simple("pl061", 0x101e6000, pic[8]);
    //sysbus_create_simple("pl061", 0x101e7000, pic[9]);

    /* The vivotech/PB actually has a modified Color LCD controller
       that includes hardware cursor support from the PL111.  */
    //dev = sysbus_create_simple("pl110_vivotech", 0x10120000, pic[16]);
    /* Wire up the mux control signals from the SYS_CLCD register */
    //qdev_connect_gpio_out(sysctl, 0, qdev_get_gpio_in(dev, 0));

    //sysbus_create_varargs("pl181", 0x10005000, sic[22], sic[1], NULL);
    //sysbus_create_varargs("pl181", 0x1000b000, sic[23], sic[2], NULL);

    /* Add PL031 Real Time Clock. */
    //sysbus_create_simple("pl031", 0x101e8000, pic[10]);

    //dev = sysbus_create_simple("vivotech_i2c", 0x10002000, NULL);
    //i2c = (I2CBus *)qdev_get_child_bus(dev, "i2c");
    //i2c_create_slave(i2c, "ds1338", 0x68);

    /* Add PL041 AACI Interface to the LM4549 codec */
    //pl041 = qdev_create(NULL, "pl041");
    //qdev_prop_set_uint32(pl041, "nc_fifo_depth", 512);
    //qdev_init_nofail(pl041);
    //sysbus_mmio_map(SYS_BUS_DEVICE(pl041), 0, 0x10004000);
    //sysbus_connect_irq(SYS_BUS_DEVICE(pl041), 0, sic[24]);

    /* Memory map for vivotech:  */
    /* 0x10000000 System registers.  */
    /* 0x10001000 PCI controller config registers.  */
    /* 0x10002000 Serial bus interface.  */
    /*  0x10003000 Secondary interrupt controller.  */
    /* 0x10004000 AACI (audio).  */
    /*  0x10005000 MMCI0.  */
    /*  0x10006000 KMI0 (keyboard).  */
    /*  0x10007000 KMI1 (mouse).  */
    /* 0x10008000 Character LCD Interface.  */
    /*  0x10009000 UART3.  */
    /* 0x1000a000 Smart card 1.  */
    /*  0x1000b000 MMCI1.  */
    /*  0x10010000 Ethernet.  */
    /* 0x10020000 USB.  */
    /* 0x10100000 SSMC.  */
    /* 0x10110000 MPMC.  */
    /*  0x10120000 CLCD Controller.  */
    /*  0x10130000 DMA Controller.  */
    /*  0x10140000 Vectored interrupt controller.  */
    /* 0x101d0000 AHB Monitor Interface.  */
    /* 0x101e0000 System Controller.  */
    /* 0x101e1000 Watchdog Interface.  */
    /* 0x101e2000 Timer 0/1.  */
    /* 0x101e3000 Timer 2/3.  */
    /* 0x101e4000 GPIO port 0.  */
    /* 0x101e5000 GPIO port 1.  */
    /* 0x101e6000 GPIO port 2.  */
    /* 0x101e7000 GPIO port 3.  */
    /* 0x101e8000 RTC.  */
    /* 0x101f0000 Smart card 0.  */
    /*  0x101f1000 UART0.  */
    /*  0x101f2000 UART1.  */
    /*  0x101f3000 UART2.  */
    /* 0x101f4000 SSPI.  */
    /* 0x34000000 NOR Flash */

    dinfo = drive_get(IF_PFLASH, 0, 0);
    if (!pflash_cfi01_register(VIVOTECH_FLASH_ADDR, NULL, "vivotech.flash",
                          VIVOTECH_FLASH_SIZE,
                          dinfo ? blk_by_legacy_dinfo(dinfo) : NULL,
                          VIVOTECH_FLASH_SECT_SIZE,
                          VIVOTECH_FLASH_SIZE / VIVOTECH_FLASH_SECT_SIZE,
                          4, 0x0089, 0x0018, 0x0000, 0x0, 0)) {
        fprintf(stderr, "qemu: Error registering flash memory.\n");
    }
    
     
    vivotech_binfo.ram_size = machine->ram_size;
    vivotech_binfo.kernel_filename = machine->kernel_filename;
    vivotech_binfo.kernel_cmdline = machine->kernel_cmdline;
    vivotech_binfo.initrd_filename = machine->initrd_filename;
    vivotech_binfo.board_id = board_id;
    arm_load_kernel(cpu, &vivotech_binfo);
}

static void vpb_init(MachineState *machine)
{
    vivotech_init(machine, 0x183);
}

static void vab_init(MachineState *machine)
{
    vivotech_init(machine, 0x25e);
}

static QEMUMachine vivotech_machine = {
    .name = "vivotech",
    .desc = "ARM vivotech (NXP 2138/ARM7TDMI)",
    .init = vpb_init,
    .block_default_type = IF_SCSI,
};

static void vivotech_machine_init(void)
{
    qemu_register_machine(&vivotech_machine);
    //qemu_register_machine(&vivotechab_machine);
}

machine_init(vivotech_machine_init);

//static void vpb_sic_class_init(ObjectClass *klass, void *data)
//{
//    DeviceClass *dc = DEVICE_CLASS(klass);
//    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);
//
//    k->init = vpb_sic_init;
//    dc->vmsd = &vmstate_vpb_sic;
//}

//static const TypeInfo vpb_sic_info = {
//    .name          = TYPE_VIVOTECH_PB_SIC,
//    .parent        = TYPE_SYS_BUS_DEVICE,
//    .instance_size = sizeof(vpb_sic_state),
//    .class_init    = vpb_sic_class_init,
//};

static void vivotechpb_register_types(void)
{
    //type_register_static(&vpb_sic_info);
}

type_init(vivotechpb_register_types)
