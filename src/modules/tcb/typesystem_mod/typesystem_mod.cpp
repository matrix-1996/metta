//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2012, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "type_system_v1_interface.h"
#include "type_system_f_v1_interface.h"
#include "type_system_f_v1_impl.h"
#include "type_system_factory_v1_interface.h"
#include "type_system_factory_v1_impl.h"
#include "enum_v1_interface.h"
#include "record_v1_interface.h"
#include "choice_v1_interface.h"
#include "operation_v1_interface.h"
#include "interface_v1_state.h"
#include "interface_v1_impl.h"
#include "naming_context_v1_interface.h"
#include "map_string_address_iterator_v1_interface.h"
#include "map_card64_address_factory_v1_interface.h"
#include "map_string_address_factory_v1_interface.h"
#include "map_card64_address_v1_interface.h"
#include "map_string_address_v1_interface.h"
#include "heap_new.h"
#include "default_console.h"
#include "exceptions.h"
#include "debugger.h"
#include "stringref.h"
#include "stringstuff.h"

/**
 * @defgroup typecode Type code
 *
 * Type code is 64 bit entity consisting of 48-bit unique identifier for the interface type plus 16 bit entity for indexing subtypes inside the interface
 * (exceptions, aliases, structures, etc.)
 * 48 bit code is generated by meddler while parsing the interface file. It attempts to maintain some sort of stability for type codes for same interfaces.
 * 16 bit code is just a sequential enumeration of all types inside an interface.
 */

//=====================================================================================================================
// Type system internal data structures.
//=====================================================================================================================

struct type_system_f_v1::state_t
{
    type_system_f_v1::closure_t        closure;
    map_card64_address_v1::closure_t*  interfaces_by_typecode;
    map_string_address_v1::closure_t*  interfaces_by_name;
};

extern interface_v1::closure_t meta_interface_closure; // forward declaration
extern interface_v1::state_t meta_interface; // forward declaration

//=====================================================================================================================
// add, remove, dup and destroy methods from naming_context are all nulls.
//=====================================================================================================================

static void
shared_add(naming_context_v1::closure_t*, const char*, types::any)
{
    OS_RAISE((exception_support_v1::id)"naming_context_v1.denied", 0);
    return;
}

static void
shared_remove(naming_context_v1::closure_t*, const char*)
{ 
    OS_RAISE((exception_support_v1::id)"naming_context_v1.denied", 0);
    return; 
}

// dup is not yet defined, but it raises denied as well

static void
shared_destroy(naming_context_v1::closure_t*)
{ 
    return; 
}

//=====================================================================================================================
// Typesystem
//=====================================================================================================================

/**
 * Look up a type name in this interface.
 */
static type_representation_t* internal_get(type_system_f_v1::state_t* state, const char* name)
{
    interface_v1::state_t* iface = nullptr;
    type_representation_t* result = nullptr;
    stringref_t name_sr(name);
    std::pair<stringref_t, stringref_t> refs = name_sr.split('.');

    // @todo: move this allocation to stringref_t guts?
    if (!refs.second.empty())
        name = string_n_copy(refs.first.data(), refs.first.size(), PVS(heap)); // @todo MEMLEAK

    /* now "name" is just the interface, and "extra" is any extra qualifier */

    if (state->interfaces_by_name->get(name, (address_t*)&iface))
    {
        /* We've found the first component. */
        if (!refs.second.empty())
        {
            for (int i = 0; iface->types[i]; ++i)
                if (refs.second == iface->types[i]->name)
                    result = iface->types[i];

            /* special case if it's an intf type defined by the metainterface */
            if (!result && (iface == &meta_interface))
            {
                result = internal_get(state, refs.second.data()); // this should trigger search in "meta_interface.<something>" but that won't work atm
            }
        }
        else
        {
            /* Otherwise return this interface clp */
            result = &iface->rep;
        }
    }

    return result;
}

static void
add_name(const char* name, heap_v1::closure_t* heap, naming_context_v1::names& n)
{
    n.push_back(string_copy(name, heap));
}

static void
add_qual_name(const char* name, const char* subname, heap_v1::closure_t* heap, naming_context_v1::names& n)
{
    size_t l1 = memutils::string_length(name);
    size_t len = l1 + memutils::string_length(subname) + 2;
    char* dst = stralloc(len, heap);
    memutils::copy_string(dst, name);
    *(dst+l1) = '.';
    memutils::copy_string(dst+l1+1, subname);
    n.push_back(dst);
}

/**
 *  Return a list of all types in the type system.
 */
static naming_context_v1::names
type_system_v1_list(naming_context_v1::closure_t* self)
{
    auto state = reinterpret_cast<type_system_f_v1::closure_t*>(self)->d_state;
    naming_context_v1::names n;
    map_string_address_iterator_v1::closure_t* it = nullptr;

    /* Run through all the interfaces */
    OS_TRY {
        const char* name;
        interface_v1::state_t* tb;
        type_representation_t* trep;

        it = state->interfaces_by_name->iterate();

        while (it->next(&name, (memory_v1::address*)&tb))
        {
            add_name(tb->rep.name, PVS(heap), n);
            /* Run through all the types defined in the current interface */
            for (size_t i = 0; i < tb->num_types; ++i)
            {
                trep = tb->types[i];
                add_qual_name(tb->rep.name, trep->name, PVS(heap), n);
            }
        }
        it->dispose();
    }
    OS_CATCH_ALL {
        if (it)
            it->dispose();
        OS_RAISE((exception_support_v1::id)"heap_v1.no_memory", 0);
    }
    OS_ENDTRY;

    return n;
}

/**
 * Type System Get method - return the type code as any.
 */ 
static bool
type_system_v1_get(naming_context_v1::closure_t* self, const char* name, types::any* obj)
{
    auto state = reinterpret_cast<type_system_f_v1::state_t*>(self->d_state);
    type_representation_t* trep = internal_get(state, name);

    if (!trep)
        return false;

    *obj = trep->code;
    return true;
}

/**
 * Info returns information about a type given its typecode.
 */
static interface_v1::closure_t*
type_system_v1_info(type_system_v1::closure_t* self, type_system_v1::alias tc, types::any* rep)
{
    interface_v1::state_t* iface = nullptr;

    /* Check the type code refers to a valid interface */
    if (!reinterpret_cast<type_system_f_v1::state_t*>(self->d_state)->interfaces_by_typecode->get(TCODE_INTF_CODE(tc), (address_t*)&iface))
        OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", tc);

    /* Deal with the case where the type code refers to an interface type */
    if (TCODE_IS_INTERFACE(tc))
    {
        *rep = iface->rep.any;
        return &meta_interface_closure;
    }
  
    /* Check that within the given interface this is a valid type */
    if (!TCODE_VALID_TYPE(tc, iface))
        OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", tc);

    type_representation_t* trep = TCODE_WHICH_TYPE(tc, iface);

    *rep = trep->any;
    return reinterpret_cast<interface_v1::closure_t*>(iface->rep.any.ptr32value);
}

/**
 * Return the size of a type.
 */
static memory_v1::size
type_system_v1_size(type_system_v1::closure_t* self, type_system_v1::alias tc)
{
    interface_v1::state_t* iface = nullptr;

    /* Check the type code refers to a valid interface */
    if (!reinterpret_cast<type_system_f_v1::state_t*>(self->d_state)->interfaces_by_typecode->get(TCODE_INTF_CODE(tc), (address_t*)&iface))
        OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", tc);

    /* Deal with the case where the type code refers to an interface type */
    if (TCODE_IS_INTERFACE(tc))
        return iface->rep.size;
  
    /* Check that within the given interface this is a valid type */
    if (!TCODE_VALID_TYPE (tc, iface))
        OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", tc);

    return TCODE_WHICH_TYPE(tc, iface)->size;
}

static types::name
type_system_v1_name(type_system_v1::closure_t* self, type_system_v1::alias tc)
{
    D(kconsole << "type_system.name" << endl);
    interface_v1::state_t* iface = nullptr;

    /* Check the type code refers to a valid interface */
    if (!reinterpret_cast<type_system_f_v1::state_t*>(self->d_state)->interfaces_by_typecode->get(TCODE_INTF_CODE(tc), (address_t*)&iface))
        OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", tc);

    /* Deal with the case where the type code refers to an interface type */
    if (TCODE_IS_INTERFACE(tc))
    {
        return string_copy(iface->rep.name, PVS(heap));
    }
  
    /* Check that within the given interface this is a valid type */
    if (!TCODE_VALID_TYPE (tc, iface))
        OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", tc);

    type_representation_t* trep = TCODE_WHICH_TYPE(tc, iface);
    return string_copy(trep->name, PVS(heap));
}

static const char*
type_system_v1_docstring(type_system_v1::closure_t* self, type_system_v1::alias tc)
{
    D(kconsole << "type_system.docstring" << endl);
    interface_v1::state_t* iface = nullptr;

    /* Check the type code refers to a valid interface */
    if (!reinterpret_cast<type_system_f_v1::state_t*>(self->d_state)->interfaces_by_typecode->get(TCODE_INTF_CODE(tc), (address_t*)&iface))
        OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", tc);

    /* Deal with the case where the type code refers to an interface type */
    if (TCODE_IS_INTERFACE(tc))
    {
        return string_copy(iface->rep.autodoc, PVS(heap));
    }
  
    /* Check that within the given interface this is a valid type */
    if (!TCODE_VALID_TYPE (tc, iface))
        OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", tc);

    type_representation_t* trep = TCODE_WHICH_TYPE(tc, iface);
    return string_copy(trep->autodoc, PVS(heap));
}

/**
 * Try to find out if two types are compatible.
 */
static bool
type_system_v1_is_type(type_system_v1::closure_t* self, type_system_v1::alias sub, type_system_v1::alias super)
{
    D(kconsole << "type_system.is_type" << endl;)
    type_system_f_v1::state_t* state = reinterpret_cast<type_system_f_v1::state_t*>(self->d_state);
    interface_v1::state_t* iface = nullptr;

    /* Check the super type code refers to a valid interface */
    if (!state->interfaces_by_typecode->get(TCODE_INTF_CODE(super), (address_t*)&iface))
        OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", super);

    D(kconsole << "type_system.is_type: " << super << " is valid supertype " << iface->rep.name << endl;)

    /* Quick and dirty check for equality. */
    if (sub == super)
    {
        D(kconsole << "subtype and supertype matched, returning true!" << endl;)
        return true;
    }

    /* Check the sub type code refers to a valid interface */
    if (!state->interfaces_by_typecode->get(TCODE_INTF_CODE(sub), (address_t*)&iface))
        OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", sub);

    D(kconsole << "type_system.is_type: " << sub << " is valid subtype " << iface->rep.name << endl;)

    /* Deal with the case where the type code refers to an interface type */
    if (TCODE_IS_INTERFACE(sub))
    {
        D(kconsole << "type_system.is_type: " << iface->rep.code.value << " vs " << super << endl;)
        while (iface->rep.code.value != super)
        {
            /* Look up the supertype */
            if (!iface->supertype)
            {
                D(kconsole << "no supertype found, returning false!" << endl;)
                return false;
            }

            if (!state->interfaces_by_typecode->get(iface->supertype, (address_t*)&iface))
                OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", iface->supertype);
            D(kconsole << "type_system.is_type: found valid supertype " << iface->rep.name << endl;
            kconsole << "type_system.is_type: " << iface->rep.code.value << " vs " << super << endl;)
        }

        D(kconsole << "valid supertype matched, returning true!" << endl;)
        return true;
    }

    /* We have a concrete type and it's not the same typecode, so fail. */
    D(kconsole << "concrete type not equal, returning false!" << endl;)
    return false;
}

/**
 * Try to narrow a type.
 */
static types::val
type_system_v1_narrow(type_system_v1::closure_t* self, types::any a, type_system_v1::alias tc)
{
    D(kconsole << "type_system.narrow {" << endl);
    if (!type_system_v1_is_type(self, a.type_, tc))
        OS_RAISE((exception_support_v1::id)"type_system_v1.incompatible", 0);

    D(kconsole << "type_system.narrow " << a << " }" << endl);
    return a.value;
}

/**
 * Unalias a type - recurse up alias chain until a non-alias type is found.
 */
static type_system_v1::alias
type_system_v1_unalias(type_system_v1::closure_t* self, type_system_v1::alias tc)
{
    D(kconsole << "type_system.unalias" << endl);
    type_system_f_v1::state_t* state = reinterpret_cast<type_system_f_v1::state_t*>(self->d_state);
    interface_v1::state_t* iface = nullptr;
    type_representation_t* trep = nullptr;

    while (true)
    {
        /* Check the type code refers to a valid interface */
        if (!state->interfaces_by_typecode->get(TCODE_INTF_CODE(tc), (address_t*)&iface))
            OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", tc);

        /* Deal with the case where the type code refers to an interface type */
        if (TCODE_IS_INTERFACE(tc))
            return tc;

        /* Check that within the given interface this is a valid type */
        if (!TCODE_VALID_TYPE (tc, iface))
            OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", tc);

        /* Get the representation of this type */
        trep = TCODE_WHICH_TYPE(tc, iface);

        /* If it's not an alias, return it */
        if (trep->any.type_ != type_system_v1::alias_type_code)
            return tc;

        /* Else go round again. */
        tc = trep->any.value;
    }

    OS_RAISE((exception_support_v1::id)"type_system_v1.bad_code", tc);
}

/*
 * Method suites for all type representation which include a closure.
 */ 
extern interface_v1::ops_t interface_ops;
extern operation_v1::ops_t operation_ops;
extern exception_v1::ops_t exception_ops;
extern record_v1::ops_t    record_ops;
extern enum_v1::ops_t      enum_ops;
extern choice_v1::ops_t    choice_ops;

static void
type_system_f_v1_register_interface(type_system_f_v1::closure_t* self, type_system_f_v1::interface_info intf)
{
    interface_v1::state_t* iface = reinterpret_cast<interface_v1::state_t*>(intf); // @todo do we need to convert this back and forth?
    address_t dummy;

    kconsole << "register_interface '" << iface->rep.name << "' {" << endl;

    if (self->d_state->interfaces_by_name->get(iface->rep.name, &dummy))
        OS_RAISE((exception_support_v1::id)"type_system_f_v1.name_clash", 0);

    if (self->d_state->interfaces_by_typecode->get(iface->rep.code.value, &dummy))
        OS_RAISE((exception_support_v1::id)"type_system_f_v1.type_code_clash", 0);

    if (iface != &meta_interface) // meta_interface needs no patching, it's all set up.
    {
        address_t clos_ptr;
        size_t i;

        /* Fill in operation tables of closures */
        reinterpret_cast<interface_v1::closure_t*>(iface->rep.any.value)->d_methods = &interface_ops;

        /* Types */
        for (i = 0; i < iface->num_types; i++)
        {
            clos_ptr = iface->types[i]->any.value;
            switch (iface->types[i]->any.type_)
            {
                case type_system_v1::choice_type_code:
                    reinterpret_cast<choice_v1::closure_t*>(clos_ptr)->d_methods = &choice_ops;
                    break;
                case type_system_v1::enum__type_code:
                    reinterpret_cast<enum_v1::closure_t*>(clos_ptr)->d_methods = &enum_ops;
                    break;
                case type_system_v1::record_type_code:
                    reinterpret_cast<record_v1::closure_t*>(clos_ptr)->d_methods = &record_ops;
                    break;
            }
        }

        /* Operations */
        for (i = 0; i < iface->num_methods; i++) {
            iface->methods[i]->closure->d_methods = &operation_ops;
        }

        /* Exceptions */
        for (i = 0; i < iface->num_exns; i++) {
            iface->exns[i]->closure.d_methods = &exception_ops;
        }
    }

    self->d_state->interfaces_by_name->put(iface->rep.name, intf);
    self->d_state->interfaces_by_typecode->put(iface->rep.code.value, intf);

    kconsole << "register_interface }" << endl;
}

static type_system_f_v1::ops_t typesystem_ops = 
{
    type_system_v1_list,
    type_system_v1_get,
    shared_add,
    shared_remove,
    shared_destroy,
    type_system_v1_info,
    type_system_v1_size,
    type_system_v1_name,
    type_system_v1_docstring,
    type_system_v1_is_type,
    type_system_v1_narrow,
    type_system_v1_unalias,
    type_system_f_v1_register_interface
};

//=====================================================================================================================
// Meta-interface
//=====================================================================================================================

static naming_context_v1::names
meta_interface_list(naming_context_v1::closure_t* self)
{
    auto state = reinterpret_cast<type_system_f_v1::closure_t*>(self)->d_state;
    naming_context_v1::names n;
    map_string_address_iterator_v1::closure_t* it = nullptr;

    OS_TRY {
        const char* name;
        interface_v1::state_t* tb;

        /* Run through all the predefined types */
        for (size_t i = 0; i < meta_interface.num_types; ++i)
        {
            add_name(meta_interface.types[i]->name, PVS(heap), n);
        }

        /* then all the others */
        it = state->interfaces_by_name->iterate();
        while (it->next(&name, (memory_v1::address*)&tb))
        {
            add_name(tb->rep.name, PVS(heap), n);
        }
        it->dispose();
    }
    OS_CATCH_ALL {
        if (it)
            it->dispose();
        OS_RAISE((exception_support_v1::id)"heap_v1.no_memory", 0);
    }
    OS_ENDTRY;

    return n;
}

/**
 * Meta-interface get method - this differs from typesystem's get in that 
 * we have to deal with PROCs and EXCEPTIONs as well as TYPEs.
 */ 
static bool
meta_interface_get(naming_context_v1::closure_t* self, const char* name, types::any* obj)
{
    bool exists = false;
    auto state = reinterpret_cast<type_system_f_v1::closure_t*>(self)->d_state;
    interface_v1::state_t* iface = nullptr;
    type_representation_t* trep = nullptr;

    /* First we check for builtin types (e.g. STRING, CHAR, etc.) */
    for (size_t i = 0; i < meta_interface.num_types; ++i)
    {
        trep = meta_interface.types[i];
        if (memutils::is_string_equal(trep->name, name))
        {
            *obj = trep->any;
            return true;
        }
    }

    /* Otherwise look up the leading component of "name" */
  
    stringref_t name_sr(name);
    std::pair<stringref_t, stringref_t> refs = name_sr.split('.');

    if (!refs.second.empty())
        name = string_n_copy(refs.first.data(), refs.first.size(), PVS(heap)); // @todo MEMLEAK

    /* now "name" is just the interface, and "extra" is any extra qualifier */

    if (state->interfaces_by_name->get(name, (address_t*)&iface))
    {
        // We've found the first component. If there are no more components,
        // then simply return the types.any; otherwise, have to recurse a bit.
        if (refs.second.empty())
        {
            *obj = iface->rep.any;
            exists = true;
        }
        else
        {
            type_system_v1::closure_t* ts = reinterpret_cast<type_system_v1::closure_t*>(&state->closure);

            /* in this case, need first half to be a context */
            if (!type_system_v1_is_type(ts, iface->rep.any.type_, naming_context_v1::type_code))
                OS_RAISE((exception_support_v1::id)"type_system_v1.not_context", 0);

            types::val v = type_system_v1_narrow(ts, iface->rep.any, naming_context_v1::type_code);
            naming_context_v1::closure_t* context = reinterpret_cast<naming_context_v1::closure_t*>(v);

            exists = context->get(refs.second.data(), obj);
            // @todo free (name) here
        }
    }
  
    return exists;
}

/**
 * extends method: the meta-interface does not extend anything.
 */
static bool
meta_interface_v1_extends(interface_v1::closure_t* self, interface_v1::closure_t** o)
{
    return false;
}

/**
 * info method: return information about the meta-interface.
 */
static bool
meta_interface_v1_info(interface_v1::closure_t* self, interface_v1::needs* need_list, types::name* name, types::code* code)
{
    *need_list = interface_v1::needs();
    *name = string_copy(TCODE_META_NAME, PVS(heap));
    *code = meta_interface_type_code;
    return /*local:*/false;
}

static interface_v1::ops_t meta_ops =
{
    meta_interface_list,
    meta_interface_get,
    shared_add,
    shared_remove,
    shared_destroy,
    meta_interface_v1_extends,
    meta_interface_v1_info
};

interface_v1::closure_t meta_interface_closure =
{
    &meta_ops,
    nullptr
};

//=====================================================================================================================
// Definition of types in the 'mythical' interface meta_interface which defines all predefined Meddle types 
// and all interfaces in Metta (including meta_interface itself).
//=====================================================================================================================

#define PREDEFINED_TYPE_REP(typename,idlname,tag) \
static type_representation_t type_##idlname##_rep = { \
    {  type_system_v1::predefined_type_code, { type_system_v1::predefined_##tag } }, \
    {  types::code_type_code, { idlname##_type_code } }, \
    #idlname, \
    "Built-in type "#idlname, \
    &meta_interface, \
    sizeof(typename) \
}

// @todo When meddler is fixed to accept octet and other types as identifiers, we'll remove the tag part from this.
PREDEFINED_TYPE_REP(uint8_t,octet, Octet);
PREDEFINED_TYPE_REP(int8_t,int8, Char);
PREDEFINED_TYPE_REP(uint16_t,card16, Card16);
PREDEFINED_TYPE_REP(uint32_t,card32, Card32);
PREDEFINED_TYPE_REP(uint64_t,card64, Card64);
PREDEFINED_TYPE_REP(int16_t,int16, Int16);
PREDEFINED_TYPE_REP(int32_t,int32, Int32);
PREDEFINED_TYPE_REP(int64_t,int64, Int64);
PREDEFINED_TYPE_REP(float,float, Float);
PREDEFINED_TYPE_REP(double,double, Double);
PREDEFINED_TYPE_REP(bool,boolean, Boolean);
PREDEFINED_TYPE_REP(cstring_t,string, String);
PREDEFINED_TYPE_REP(voidptr,opaque, Opaque);

static type_representation_t* const meta_interface__types[] =
{
    &type_octet_rep,
    &type_card16_rep,
    &type_card32_rep,
    &type_card64_rep,
    &type_int8_rep,
    &type_int16_rep,
    &type_int32_rep,
    &type_int64_rep,
    &type_float_rep,
    &type_double_rep,
    &type_boolean_rep,
    &type_string_rep,
    &type_opaque_rep
};

interface_v1::state_t meta_interface =
{
    { // representation
        { type_system_v1::iref_type_code, { .ptr32value = &meta_interface_closure } },  // any & cl.ptr
        { types::code_type_code, { meta_interface_type_code } },                        // Type Code
        TCODE_META_NAME,                                                                // Textual name
        "Meta interface representing all interfaces in the system.",                    // Autodoc
        &meta_interface,                                                                // Scope
        sizeof(interface_v1::closure_t*)                                                // Size
    }, // end representation
    nullptr,                    // Needs list
    0,                          // No. of needs
    meta_interface__types,      // Types list
    13,                         // No. of types
    true,                       // Local flag
    interface_v1::type_code,    // Supertype
    nullptr,                    // Methods
    0,                          // No. of methods
    nullptr,                    // Exceptions
    0                           // No. of exceptions
};

//=====================================================================================================================
// The Factory
//=====================================================================================================================

static type_system_f_v1::closure_t* 
create(type_system_factory_v1::closure_t* self, heap_v1::closure_t* h, 
       map_card64_address_factory_v1::closure_t* cardmap, map_string_address_factory_v1::closure_t* stringmap)
{
    type_system_f_v1::state_t* state = new(h) type_system_f_v1::state_t;
    closure_init(&state->closure, &typesystem_ops, state);

    state->interfaces_by_typecode = cardmap->create(h);
    state->interfaces_by_name = stringmap->create(h);

    state->closure.register_interface(reinterpret_cast<type_system_f_v1::interface_info>(&meta_interface));
    /*
     * The meta-interface closure is of type "interface_v1" but has different methods
     * since it's state record is type_system state rather than interface_v1::state.
     */
    meta_interface_closure.d_state = reinterpret_cast<interface_v1::state_t*>(state);

    return &state->closure;
}

static type_system_factory_v1::ops_t methods = 
{
    create
};

static type_system_factory_v1::closure_t clos =
{
    &methods,
    nullptr
};

EXPORT_CLOSURE_TO_ROOTDOM(type_system_factory, v1, clos);
