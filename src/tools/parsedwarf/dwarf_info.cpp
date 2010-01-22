#include "dwarf_info.h"
#include "datarepr.h"
#include "form_reader.h"
#include "local_panic.h"
#include <stdio.h>

void cuh_t::decode(address_t from, size_t& offset)
{
    unit_length = *reinterpret_cast<uint32_t*>(from + offset);
    if (unit_length == 0xffffffff)
        PANIC("DWARF64 is not supported!");
    offset += sizeof(uint32_t);
    version = *reinterpret_cast<uint16_t*>(from + offset);
    offset += sizeof(uint16_t);
    debug_abbrev_offset = *reinterpret_cast<uint32_t*>(from + offset);
    offset += sizeof(uint32_t);
    address_size = *reinterpret_cast<uint8_t*>(from + offset);
    offset += sizeof(uint8_t);
}

die_t& die_t::operator=(const die_t& d)
{
    if (this != &d)
    {
        parser = d.parser;
        abbrev_code = d.abbrev_code;
        node_attributes = d.node_attributes;
        tag = d.tag;
        has_children = d.has_children;
        parent = d.parent;
        children = d.children;
    }
    return *this;
}

bool die_t::decode(address_t from, size_t& offset)
{
    offs = offset;
    abbrev_code = uleb128_t::decode(from, offset, -1);
    if (abbrev_code == 0)
    {
//         printf("Last sibling node\n");
        return false;
    }
    // find abbreviation for tag
    auto abbrev = parser.debug_info->find_abbrev(abbrev_code);
    if (abbrev)
    {
        tag = abbrev->tag;
        has_children = abbrev->has_children;
        for (size_t i = 0; i < abbrev->attributes.size()-1; ++i)
        {
            uint32_t name = abbrev->attributes[i].name;
            node_attributes[name] = form_reader_t::create(parser, abbrev->attributes[i].form);
            node_attributes[name]->decode(from, offset);
        }
        return true;
    }
    return false;
}

die_t* die_t::find_address(address_t addr)
{
    address_t low_pc = -1, high_pc = 0;

    addr_form_reader_t* f1 = dynamic_cast<addr_form_reader_t*>(node_attributes[DW_AT_low_pc]);
    if (f1)
        low_pc = f1->data;

    addr_form_reader_t* f2 = dynamic_cast<addr_form_reader_t*>(node_attributes[DW_AT_high_pc]);
    if (f2)
        high_pc = f2->data;

    if (is_subprogram() && low_pc <= addr && high_pc >= addr)
    {
        printf("FOUND TARGET SUBROUTINE FROM %x to %x\n", low_pc, high_pc);
        return this;
    }

    for (size_t i = 0; i < children.size(); ++i)
    {
        die_t* c = children[i]->find_address(addr);
        if (c)
            return c;
    }

    return 0;
}

die_t* die_t::find_by_offset(size_t offset)
{
    if (offs == offset)
        return this;

    for (size_t i = 0; i < children.size(); ++i)
    {
        die_t* c = children[i]->find_by_offset(offset);
        if (c)
            return c;
    }

    return 0;
}

die_t* die_t::find_compile_unit()
{
    die_t* node = this;
    while (node)
    {
        if (node->tag == DW_TAG_compile_unit)
            return node;
        node = node->parent;
    }
    return 0;
}

void die_t::dump()
{
    printf("*DIE: abbrev %d tag %08x %s (has_children %d)\n", abbrev_code, tag, tag2name(tag), has_children);
    auto abbrev = parser.debug_info->find_abbrev(abbrev_code);
    if (abbrev)
    {
        for (size_t i = 0; i < abbrev->attributes.size()-1; ++i)
        {
            uint32_t name = abbrev->attributes[i].name;
            uint32_t form = abbrev->attributes[i].form;
            printf("  form: %s, name: %x %s, value: ", form2name(form), name, attr2name(name));
            node_attributes[name]->print();
        }
    }
}