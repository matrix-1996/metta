//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2012, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "bga.h"
#include "pci_bus.h"
#include "card_registers.h"
#include "default_console.h"

using namespace bga_card;

namespace graphics {

void bga::reg_write(int regno, uint16_t value)
{
    x86_cpu_t::outw(VBE_DISPI_IOPORT_INDEX, regno);
    x86_cpu_t::outw(VBE_DISPI_IOPORT_DATA, value);
}

uint16_t bga::reg_read(int regno)
{
    x86_cpu_t::outw(VBE_DISPI_IOPORT_INDEX, regno);
    return x86_cpu_t::inw(VBE_DISPI_IOPORT_DATA);
}

bool bga::is_available()
{
    return reg_read(VBE_DISPI_INDEX_ID) == VBE_DISPI_ID5;
}

#define BAR0 0x10

void bga::configure(pci_device_t* card)
{
    uint32_t bar0 = card->read_config_space(BAR0); // LFB

    kconsole << "This bga uses LFB at " << bar0 << endl;
}

void bga::init()
{
    kconsole << "Initializing BGA." << endl;
    // Do nothing.
}

void bga::set_mode(int width, int height, int bpp)
{
    reg_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    reg_write(VBE_DISPI_INDEX_XRES, width);
    reg_write(VBE_DISPI_INDEX_YRES, height);
    reg_write(VBE_DISPI_INDEX_BPP, bpp);
    reg_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED|VBE_DISPI_LFB_ENABLED);
}

} // namespace graphics