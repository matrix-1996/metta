//
// Part of Metta OS. Check https://atta-metta.net for latest version.
//
// Copyright 2010 - 2017, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "form_reader.h"
#include "datarepr.h"
#include <stdio.h>

form_reader_t* form_reader_t::create(dwarf_parser_t& parser, uint32_t form)
{
    switch (form)
    {
        case DW_FORM_addr:
            return new addr_form_reader_t(parser);
        case DW_FORM_block2:
            return new block2_form_reader_t(parser);
        case DW_FORM_block4:
            return new block4_form_reader_t(parser);
        case DW_FORM_data2:
            return new data2_form_reader_t(parser);
        case DW_FORM_data4:
            return new data4_form_reader_t(parser);
        case DW_FORM_data8:
            return new data8_form_reader_t(parser);
        case DW_FORM_string:
            return new string_form_reader_t(parser);
        case DW_FORM_block:
            return new block_form_reader_t(parser);
        case DW_FORM_block1:
            return new block1_form_reader_t(parser);
        case DW_FORM_data1:
            return new data1_form_reader_t(parser);
        case DW_FORM_flag:
            return new flag_form_reader_t(parser);
        case DW_FORM_sdata:
            return new sdata_form_reader_t(parser);
        case DW_FORM_strp:
            return new strp_form_reader_t(parser);
        case DW_FORM_udata:
            return new udata_form_reader_t(parser);
        case DW_FORM_ref_addr:
            return new ref_addr_form_reader_t(parser);
        case DW_FORM_ref1:
            return new ref1_form_reader_t(parser);
        case DW_FORM_ref2:
            return new ref2_form_reader_t(parser);
        case DW_FORM_ref4:
            return new ref4_form_reader_t(parser);
        case DW_FORM_ref8:
            return new ref8_form_reader_t(parser);
        case DW_FORM_ref_udata:
            return new ref_udata_form_reader_t(parser);
        case DW_FORM_indirect:
            return new indirect_form_reader_t(parser);
    }
    return 0;
}

bool addr_form_reader_t::decode(address_t from, size_t& offset)
{
    data = *reinterpret_cast<uint32_t*>(from + offset);
    offset += sizeof(uint32_t);
    return true;
}

bool block_form_reader_t::decode(address_t from, size_t& offset)
{
    length.decode(from, offset);
    data = reinterpret_cast<char*>(from + offset);
    offset += length;
    return true;
}

bool block1_form_reader_t::decode(address_t from, size_t& offset)
{
    length = *reinterpret_cast<uint8_t*>(from + offset);
    offset += sizeof(uint8_t);
    data = reinterpret_cast<char*>(from + offset);
    offset += length;
    return true;
}

bool block2_form_reader_t::decode(address_t from, size_t& offset)
{
    length = *reinterpret_cast<uint16_t*>(from + offset);
    offset += sizeof(uint16_t);
    data = reinterpret_cast<char*>(from + offset);
    offset += length;
    return true;
}

bool block4_form_reader_t::decode(address_t from, size_t& offset)
{
    length = *reinterpret_cast<uint32_t*>(from + offset);
    offset += sizeof(uint32_t);
    data = reinterpret_cast<char*>(from + offset);
    offset += length;
    return true;
}

bool sdata_form_reader_t::decode(address_t from, size_t& offset)
{
    data.decode(from, offset);
    return true;
}

bool udata_form_reader_t::decode(address_t from, size_t& offset)
{
    data.decode(from, offset);
    return true;
}

bool data1_form_reader_t::decode(address_t from, size_t& offset)
{
    data = *reinterpret_cast<uint8_t*>(from + offset);
    offset += sizeof(uint8_t);
    return true;
}

bool data2_form_reader_t::decode(address_t from, size_t& offset)
{
    data = *reinterpret_cast<uint16_t*>(from + offset);
    offset += sizeof(uint16_t);
    return true;
}

bool data4_form_reader_t::decode(address_t from, size_t& offset)
{
    data = *reinterpret_cast<uint32_t*>(from + offset);
    offset += sizeof(uint32_t);
    return true;
}

bool data8_form_reader_t::decode(address_t from, size_t& offset)
{
    data = *reinterpret_cast<uint64_t*>(from + offset);
    offset += sizeof(uint64_t);
    return true;
}

bool flag_form_reader_t::decode(address_t from, size_t& offset)
{
    data = *reinterpret_cast<uint8_t*>(from + offset);
    offset += sizeof(uint8_t);
    return true;
}

bool string_form_reader_t::decode(address_t from, size_t& offset)
{
    data = reinterpret_cast<char*>(from + offset);
    char* d = reinterpret_cast<char*>(from + offset);
    while (*d++)
        ++offset;
    ++offset;
    return true;
}

bool strp_form_reader_t::decode(address_t from, size_t& offset)
{
    const char* strpool = reinterpret_cast<const char*>(parser.elf_parser.start() + parser.debug_str->offset);
    uint32_t stroff = *reinterpret_cast<uint32_t*>(from + offset);
    offset += sizeof(uint32_t);
    data = strpool + stroff;
    return true;
}

bool ref1_form_reader_t::decode(address_t from, size_t& offset)
{
    data = *reinterpret_cast<uint8_t*>(from + offset);
    offset += sizeof(uint8_t);
    return true;
}

bool ref2_form_reader_t::decode(address_t from, size_t& offset)
{
    data = *reinterpret_cast<uint16_t*>(from + offset);
    offset += sizeof(uint16_t);
    return true;
}

bool ref4_form_reader_t::decode(address_t from, size_t& offset)
{
    data = *reinterpret_cast<uint32_t*>(from + offset);
    offset += sizeof(uint32_t);
    return true;
}

bool ref8_form_reader_t::decode(address_t from, size_t& offset)
{
    data = *reinterpret_cast<uint64_t*>(from + offset);
    offset += sizeof(uint64_t);
    return true;
}

bool ref_udata_form_reader_t::decode(address_t from, size_t& offset)
{
    data.decode(from, offset);
    return true;
}

bool ref_addr_form_reader_t::decode(address_t from, size_t& offset)
{
    data = *reinterpret_cast<uint32_t*>(from + offset);
    offset += sizeof(uint32_t);
    return true;
}

bool indirect_form_reader_t::decode(address_t from, size_t& offset)
{
    form.decode(from, offset);
    data = create(parser, form);
    if (data)
    {
        data->decode(from, offset);
        return true;
    }
    return false;
}

// Printing
void addr_form_reader_t::print()
{
    printf("<a>0x%08x\n", data);
}

void block_form_reader_t::print()
{
    printf("<bu>%u bytes\n", (uint32_t)length);
}

void block1_form_reader_t::print()
{
    printf("<b8>%u bytes\n", length);
}

void block2_form_reader_t::print()
{
    printf("<b16>%u bytes\n", length);
}

void block4_form_reader_t::print()
{
    printf("<b32>%u bytes\n", length);
}

void sdata_form_reader_t::print()
{
    printf("<i>%d\n", (int32_t)data);
}

void udata_form_reader_t::print()
{
    printf("<u>%u\n", (uint32_t)data);
}

void data1_form_reader_t::print()
{
    printf("<8>0x%02x\n", data);
}

void data2_form_reader_t::print()
{
    printf("<16>0x%04x\n", data);
}

void data4_form_reader_t::print()
{
    printf("<32>0x%08x\n", data);
}

void data8_form_reader_t::print()
{
    printf("<64>0x%llu\n", data);
}

void flag_form_reader_t::print()
{
    printf("<f>%s\n", data == 0 ? "unset" : "set");
}

void string_form_reader_t::print()
{
    printf("<s>%s\n", data);
}

void strp_form_reader_t::print()
{
    printf("<Is>%s\n", data);
}

void ref1_form_reader_t::print()
{
    printf("<r8>0x%02x\n", data);
}

void ref2_form_reader_t::print()
{
    printf("<r16>0x%04x\n", data);
}

void ref4_form_reader_t::print()
{
    printf("<r32>0x%08x\n", data);
}

void ref8_form_reader_t::print()
{
    printf("<r64>0x%llu\n", data);
}

void ref_udata_form_reader_t::print()
{
    printf("<ru>%u\n", (uint32_t)data);
}

void ref_addr_form_reader_t::print()
{
    printf("<ra>%u\n", data);
}

void indirect_form_reader_t::print()
{
    printf("<I>");
    data->print();
}
