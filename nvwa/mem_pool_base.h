// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2004-2024 Wu Yongwei <wuyongwei at gmail dot com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must
 *    not claim that you wrote the original software.  If you use this
 *    software in a product, an acknowledgement in the product
 *    documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must
 *    not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 *
 * This file is part of Stones of Nvwa:
 *      https://github.com/adah1972/nvwa
 *
 */

/**
 * @file  mem_pool_base.h
 *
 * Header file for the memory pool base.
 *
 * @date  2024-05-20
 */

#ifndef NVWA_MEM_POOL_BASE_H
#define NVWA_MEM_POOL_BASE_H

#include <stddef.h>             // size_t
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++_features.h"       // _DELETE

NVWA_NAMESPACE_BEGIN

/**
 * Base class for memory pools.
 */
class mem_pool_base {
public:
    virtual ~mem_pool_base();
    virtual void recycle() = 0;
    static void* alloc_sys(size_t size);
    static void dealloc_sys(void* ptr);

    /** Structure to store the next available memory block. */
    struct _Block_list {
        _Block_list* _M_next;   ///< Pointer to the next memory block
    };

private:
    mem_pool_base(const mem_pool_base&) _DELETED;
    mem_pool_base& operator=(const mem_pool_base&) _DELETED;
};

NVWA_NAMESPACE_END

#endif // NVWA_MEM_POOL_BASE_H
