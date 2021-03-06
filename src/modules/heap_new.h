//
// Part of Metta OS. Check https://atta-metta.net for latest version.
//
// Copyright 2007 - 2017, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "heap_v1_interface.h"

void* operator new(size_t size, heap_v1::closure_t* heap) throw();
void* operator new[](size_t size, heap_v1::closure_t* heap) throw();
void operator delete(void* p, heap_v1::closure_t* heap) throw();
void operator delete[](void* p, heap_v1::closure_t* heap) throw();
