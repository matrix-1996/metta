/*!
 * Implement safe_card64table and card64table modules as well as factories for them.
 *
 * It is a simple wrapper around STL map class.
 * TODO: Since this type of wrapper is often repeated (see stretch_table_mod, string_address_table) 
 * it makes sense to make a generic reusable version.
 */
#include "map_card64_address_factory_v1_interface.h"
#include "map_card64_address_factory_v1_impl.h"
#include "map_card64_address_v1_interface.h"
#include "map_card64_address_v1_impl.h"
#include "heap_v1_interface.h"
#include "default_console.h"
#include "heap_allocator.h"
#include "hash_map"

typedef std::allocator<std::pair<map_card64_address_v1::key, map_card64_address_v1::value>> card64_table_heap_allocator;

typedef std::hash_map<
			map_card64_address_v1::key, 
			map_card64_address_v1::value, 
			std::hash<map_card64_address_v1::key>, 
			std::equal_to<map_card64_address_v1::key>, 
			card64_table_heap_allocator>

				card64table_t;

struct map_card64_address_v1::state_t
{
	map_card64_address_v1::closure_t* closure;
	heap_v1::closure_t* heap;
	card64table_t* table;
};

static bool get(map_card64_address_v1::closure_t* self, map_card64_address_v1::key k, map_card64_address_v1::value* v)
{
    card64table_t::iterator it = self->d_state->table->find(k);
    if (it != self->d_state->table->end())
    {
    	*v = (*it).second;
        return true;
    }
    return false;
}

static bool put(map_card64_address_v1::closure_t* self, map_card64_address_v1::key k, map_card64_address_v1::value v)
{
	return self->d_state->table->insert(std::make_pair(k, v)).second;
}

static bool remove(map_card64_address_v1::closure_t* self, map_card64_address_v1::key k, map_card64_address_v1::value* v)
{
    card64table_t::iterator it = self->d_state->table->find(k);
    if (it != self->d_state->table->end())
    {
    	*v = (*it).second;
        self->d_state->table->erase(it);
        return true;
    }
    return false;
}

static uint32_t size(map_card64_address_v1::closure_t* self)
{
	return self->d_state->table->size();
}

static void dispose(map_card64_address_v1::closure_t* self) // RENAME to destroy()? See stretch_table_mod for ref.
{
	self->d_state->table->clear();
	//TODO: delete self
}

struct map_card64_address_v1::ops_t map_methods =
{
	get,
	put,
	remove,
	size,
	dispose
};

//=====================================================================================================================
// The Factory
//=====================================================================================================================

static map_card64_address_v1::closure_t* 
map_card64_address_factory_v1_create(map_card64_address_factory_v1::closure_t* self, heap_v1::closure_t* heap)
{
	map_card64_address_v1::state_t* state = new(heap) map_card64_address_v1::state_t;
    auto heap_alloc = new(heap) std::heap_allocator_implementation(heap); // FIXME: a mem leak!
	// TODO: if (!state) raise Exception
	state->heap = heap;
	state->table = new(heap) card64table_t(heap_alloc);
	closure_init(state->closure, &map_methods, state);
	return state->closure;
}

struct map_card64_address_factory_v1::ops_t methods =
{
	map_card64_address_factory_v1_create
};

struct map_card64_address_factory_v1::closure_t clos =
{
	&methods,
	NULL
};

EXPORT_CLOSURE_TO_ROOTDOM(map_card64_address_factory, v1, clos);

