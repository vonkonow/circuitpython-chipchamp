// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 CircuitPython Contributors
//
// SPDX-License-Identifier: MIT

#ifndef MICROPY_INCLUDED_SHARED_BINDINGS_SAMPLECORE___INIT___H
#define MICROPY_INCLUDED_SHARED_BINDINGS_SAMPLECORE___INIT___H

#include "py/obj.h"

#if CIRCUITPY_SAMPLECORE
// Module globals
extern const mp_obj_module_t samplecore_module;

// Module initialization functions
void samplecore_module_init(void);
void samplecore_module_deinit(void);

// Module-level garbage collection function
void samplecore_gc_collect(void);

#endif

#endif  // MICROPY_INCLUDED_SHARED_BINDINGS_SAMPLECORE___INIT___H
