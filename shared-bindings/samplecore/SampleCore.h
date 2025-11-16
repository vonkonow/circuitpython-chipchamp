// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 CircuitPython Contributors
//
// SPDX-License-Identifier: MIT

#ifndef MICROPY_INCLUDED_SHARED_BINDINGS_SAMPLECORE_SAMPLECORE_H
#define MICROPY_INCLUDED_SHARED_BINDINGS_SAMPLECORE_SAMPLECORE_H

#include "py/obj.h"
#include "py/runtime.h"

// Type declarations
extern const mp_obj_type_t samplecore_SampleCore_type;

// Constructor prototype
mp_obj_t samplecore_SampleCore_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args);

#endif // MICROPY_INCLUDED_SHARED_BINDINGS_SAMPLECORE_SAMPLECORE_H
