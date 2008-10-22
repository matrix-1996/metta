//
// Copyright 2007 - 2008, Stanislav Karchebnyy <berkus+metta@madfire.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "Atomic.h"

/**
 * A class that implements a spinlock / binary semaphore.
 */
class lockable_t
{
public:
	lockable_t()
	{
		lock = 0;
	}

	~lockable_t() {}

	// Spin until we get the lock.
	void get_lock()
	{
		uint32_t new_val = 1;
		// If we exchange the lock value with 1 and get 1 out, it was locked.
		while (atomic_t::exchange(&lock, new_val) == 1)
		{
			// Do nothing.
		}
		// We got the lock, return.
	}

	bool try_lock()
	{
		// Spin once.
		uint32_t new_val = 1;
		if (atomic_t::exchange(&lock, new_val) == 0)
		{
			return true;
		}
		return false;
	}

	bool test_lock()
	{
		return lock;
	}

	void release_lock()
	{
		lock = 0;
	}

private:
	/**
	 * The actual lock variable
	 */
	uint32_t lock;
};

