// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 Your Name
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "shared-bindings/pixelcore/__init__.h"
#include "shared-bindings/pixelcore/pixelcore.h"

//| """PixelCore module for ESP32
//| The `pixelcore` module provides a class to control a display device.
//| """

static const mp_rom_map_elem_t pixelcore_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_pixelcore) },
    { MP_ROM_QSTR(MP_QSTR_PixelCore), MP_ROM_PTR(&pixelcore_PixelCore_type) },
};

static MP_DEFINE_CONST_DICT(pixelcore_module_globals, pixelcore_module_globals_table);

const mp_obj_module_t pixelcore_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pixelcore_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_pixelcore, pixelcore_module);
