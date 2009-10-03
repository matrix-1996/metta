//
// Copyright 2007 - 2009, Stanislav Karchebnyy <berkus+metta@madfire.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "page_directory.h"

namespace scheduler
{

/*!
* Context-switch state.
*/
struct csw_state_t
{
    /*!
    * Page directory to load while running this thread.
    * Kept up-to-date by the task code in task.cpp.
    */
    page_directory_t* page_dir;

    /*!
    * Saved kernel register state for this thread.
    */
    jmp_buf         state;
};

/*!
* Thread is an execution flow abstration, supported by the scheduler.
*/
class thread_t
{
    thread_t();
private:
};

}

// kate: indent-width 4; replace-tabs on;
// vim: set et sw=4 ts=4 sts=4 cino=(4 :