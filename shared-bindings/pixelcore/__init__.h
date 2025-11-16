// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 Your Name
//
// SPDX-License-Identifier: MIT

#ifndef MICROPY_INCLUDED_SHARED_BINDINGS_PIXELCORE___INIT___H
#define MICROPY_INCLUDED_SHARED_BINDINGS_PIXELCORE___INIT___H

#include "py/obj.h"

#if CIRCUITPY_PIXELCORE
// Module globals
extern const mp_obj_module_t pixelcore_module;

// Module initialization functions
void pixelcore_module_init(void);
void pixelcore_module_deinit(void);

// Drawing functions declared here are optional utility functions
// The actual type definition is in the port-specific common-hal
#endif

#endif  // MICROPY_INCLUDED_SHARED_BINDINGS_PIXELCORE___INIT___H
