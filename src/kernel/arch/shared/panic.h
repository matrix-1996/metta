//
// Part of Metta OS. Check https://atta-metta.net for latest version.
//
// Copyright 2007 - 2017, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "types.h"
#include "macros.h"

#define PANIC(msg) panic(msg, __FILE__, __LINE__)

#ifdef UNIT_TESTS
#define ASSERT(b) assert(b)
#else
#define ASSERT(b) ((b) ? (void)0 : panic_assert(#b, __FILE__, __LINE__))
#endif

extern "C" void panic(const char* message, const char* file, uint32_t line) NEVER_RETURNS;
extern "C" void panic_assert(const char* desc, const char* file, uint32_t line) NEVER_RETURNS;
extern "C" void halt() NEVER_RETURNS;
