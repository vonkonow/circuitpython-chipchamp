// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 Your Name
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

// Temporary workaround for STATIC not being defined
#ifndef STATIC
#define STATIC static
#endif

#include "py/obj.h"
#include "py/runtime.h"
#include "py/objexcept.h"
#include "shared/runtime/context_manager_helpers.h"

#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/digitalio/DigitalInOut.h"
#include "shared-bindings/pixelcore/pixelcore.h"

// Include the port-specific types and function declarations
#include "common-hal/pixelcore/pixelcore_types.h"
#include "common-hal/pixelcore/pixelcore.h"

// Include the port-specific types after the shared-bindings header
#include "common-hal/pixelcore/pixelcore_types.h"

//| class PixelCore:
//|     """Controls a display device connected via SPI."""
//|
//|     def __init__(self, spi: busio.SPI, cs: microcontroller.Pin, dc: microcontroller.Pin, rst: microcontroller.Pin) -> None:
//|         """Create a PixelCore object
//|
//|         :param busio.SPI spi: SPI bus for communication
//|         :param microcontroller.Pin cs: Chip select pin  
//|         :param microcontroller.Pin dc: Data/command pin
//|         :param microcontroller.Pin rst: Reset pin"""
//|         ...

// Function declarations
mp_obj_t pixelcore_PixelCore_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args);
STATIC mp_obj_t pixelcore_PixelCore_deinit(mp_obj_t self_in);
STATIC mp_obj_t pixelcore_PixelCore_clear(mp_obj_t self_in);
STATIC mp_obj_t pixelcore_PixelCore_update(mp_obj_t self_in);
STATIC mp_obj_t pixelcore_PixelCore_fill(size_t n_args, const mp_obj_t *args);
STATIC mp_obj_t pixelcore_PixelCore_pixel(size_t n_args, const mp_obj_t *args);
STATIC mp_obj_t pixelcore_PixelCore_hline(size_t n_args, const mp_obj_t *args);
STATIC mp_obj_t pixelcore_PixelCore_vline(size_t n_args, const mp_obj_t *args);
STATIC mp_obj_t pixelcore_PixelCore_rect(size_t n_args, const mp_obj_t *args);
STATIC mp_obj_t pixelcore_PixelCore_box(size_t n_args, const mp_obj_t *args);
STATIC mp_obj_t pixelcore_PixelCore_sprite(size_t n_args, const mp_obj_t *args);
STATIC mp_obj_t pixelcore_PixelCore_sprites(size_t n_args, const mp_obj_t *args);

STATIC mp_obj_t pixelcore_PixelCore_tilemap(size_t n_args, const mp_obj_t *args);
STATIC mp_obj_t pixelcore_PixelCore_line(size_t n_args, const mp_obj_t *args);
STATIC mp_obj_t pixelcore_PixelCore_circle(size_t n_args, const mp_obj_t *args);
STATIC mp_obj_t pixelcore_PixelCore_row_fill(size_t n_args, const mp_obj_t *args);

// Function object declarations


// Locals dictionary and type definition will be moved to the end

// Constructor implementation
mp_obj_t pixelcore_PixelCore_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_spi, ARG_cs, ARG_dc, ARG_rst };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_spi, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_cs, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_dc, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_rst, MP_ARG_REQUIRED | MP_ARG_OBJ },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get SPI bus
    busio_spi_obj_t *spi = validate_obj_is_spi_bus(args[ARG_spi].u_obj, MP_QSTR_spi);
    if (spi == NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("SPI bus not found"));
    }

    // Get pins
    const mcu_pin_obj_t *cs_pin = validate_obj_is_free_pin(args[ARG_cs].u_obj, MP_QSTR_cs);
    const mcu_pin_obj_t *dc_pin = validate_obj_is_free_pin(args[ARG_dc].u_obj, MP_QSTR_dc);
    const mcu_pin_obj_t *rst_pin = validate_obj_is_free_pin(args[ARG_rst].u_obj, MP_QSTR_rst);

    // Create display object
    pixelcore_PixelCore_obj_t *self = m_new_obj(pixelcore_PixelCore_obj_t);
    self->base.type = &pixelcore_PixelCore_type;

    // Create DigitalInOut objects
    digitalio_digitalinout_obj_t *cs = m_new_obj(digitalio_digitalinout_obj_t);
    digitalio_digitalinout_obj_t *dc = m_new_obj(digitalio_digitalinout_obj_t);
    digitalio_digitalinout_obj_t *rst = m_new_obj(digitalio_digitalinout_obj_t);

    // Initialize pins
    common_hal_digitalio_digitalinout_construct(cs, cs_pin);
    common_hal_digitalio_digitalinout_construct(dc, dc_pin);
    common_hal_digitalio_digitalinout_construct(rst, rst_pin);

    // Initialize display
    common_hal_pixelcore_PixelCore_construct(self, spi, cs, dc, rst);

    return MP_OBJ_FROM_PTR(self);
}

// Method implementations
STATIC mp_obj_t pixelcore_PixelCore_deinit(mp_obj_t self_in) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_pixelcore_PixelCore_deinit(self);
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_update(mp_obj_t self_in) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_pixelcore_PixelCore_update(self);
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_fill(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    uint16_t color = mp_obj_get_int(args[1]);
    
    // Optional start_y parameter (default: 0)
    uint16_t start_y = 0;
    if (n_args > 2) {
        start_y = mp_obj_get_int(args[2]);
    }
    
    // Optional end_y parameter (default: full height)
    uint16_t end_y = 127;  // Default display height - 1
    if (n_args > 3) {
        end_y = mp_obj_get_int(args[3]);
    }
    
    common_hal_pixelcore_PixelCore_fill(self, color, start_y, end_y);
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_pixel(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    common_hal_pixelcore_PixelCore_pixel(self, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]), mp_obj_get_int(args[3]));
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_hline(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    common_hal_pixelcore_PixelCore_hline(self, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]), mp_obj_get_int(args[3]), mp_obj_get_int(args[4]));
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_vline(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    common_hal_pixelcore_PixelCore_vline(self, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]), mp_obj_get_int(args[3]), mp_obj_get_int(args[4]));
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_rect(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    common_hal_pixelcore_PixelCore_rect(self, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]), mp_obj_get_int(args[3]), mp_obj_get_int(args[4]), mp_obj_get_int(args[5]));
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_box(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    common_hal_pixelcore_PixelCore_box(self, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]), mp_obj_get_int(args[3]), mp_obj_get_int(args[4]), mp_obj_get_int(args[5]));
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_clear(mp_obj_t self_in) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_pixelcore_PixelCore_clear(self);
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_sprite(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    // Keep sprite data reference on stack (minimal cost)
    mp_obj_t sprite_data_ref = args[1];
    
    // Direct buffer access (no validation overhead)
    mp_buffer_info_t bufinfo;
    mp_get_buffer(sprite_data_ref, &bufinfo, MP_BUFFER_READ);
    
    // Fast sprite rendering
    pixelcore_spritemap_t spritemap;
    spritemap.sprite_width = mp_obj_get_int(args[2]);    // Width of each sprite
    spritemap.sprite_height = mp_obj_get_int(args[3]);   // Height of each sprite
    spritemap.sprites_per_row = mp_obj_get_int(args[4]); // Number of sprites per row
    spritemap.data = (uint16_t *)bufinfo.buf;
    
    uint8_t sprite_index = mp_obj_get_int(args[5]);
    uint16_t x = mp_obj_get_int(args[6]);
    uint16_t y = mp_obj_get_int(args[7]);
    
    // Optional transparency parameter (default false)
    bool use_transparency = false;
    if (n_args > 8) {
        use_transparency = mp_obj_is_true(args[8]);
    }
    
    // Optional x_flip parameter (default false)
    bool x_flip = false;
    if (n_args > 9) {
        x_flip = mp_obj_is_true(args[9]);
    }
    
    // Unified sprite operation with 8-pixel processing
    common_hal_pixelcore_PixelCore_sprite(self, &spritemap, sprite_index, x, y, use_transparency, x_flip);
    
    // sprite_data_ref keeps buffer alive until here
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_line(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    common_hal_pixelcore_PixelCore_line(self, 
        mp_obj_get_int(args[1]),  // x0
        mp_obj_get_int(args[2]),  // y0
        mp_obj_get_int(args[3]),  // x1
        mp_obj_get_int(args[4]),  // y1
        mp_obj_get_int(args[5])   // color
    );
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_circle(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    common_hal_pixelcore_PixelCore_circle(self, 
        mp_obj_get_int(args[1]),  // center_x
        mp_obj_get_int(args[2]),  // center_y  
        mp_obj_get_int(args[3]),  // radius
        mp_obj_get_int(args[4])   // color
    );
    return mp_const_none;
}



STATIC mp_obj_t pixelcore_PixelCore_sprites(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    // Keep sprite data reference on stack (minimal cost)
    mp_obj_t sprite_data_ref = args[1];
    
    // Direct buffer access (no validation overhead)
    mp_buffer_info_t bufinfo;
    mp_get_buffer(sprite_data_ref, &bufinfo, MP_BUFFER_READ);
    
    // Fast sprite rendering
    pixelcore_spritemap_t spritemap;
    spritemap.sprite_width = mp_obj_get_int(args[2]);    
    spritemap.sprite_height = mp_obj_get_int(args[3]);   
    spritemap.sprites_per_row = mp_obj_get_int(args[4]); 
    spritemap.data = (uint16_t *)bufinfo.buf;
    
    // Get 2D list format: [[sprite_index, x, y], [sprite_index, x, y, use_transparency], ...]
    mp_obj_t sprites_2d_list = args[5];
    
    // Optional global transparency parameter (default false)
    bool global_transparency = false;
    if (n_args > 6) {
        global_transparency = mp_obj_is_true(args[6]);
    }
    
    // Get number of sprites
    size_t sprite_count = mp_obj_get_int(mp_obj_len(sprites_2d_list));
    
    // Process each sprite from 2D list with bounds checking
    for (size_t i = 0; i < sprite_count; i++) {
        mp_obj_t sprite_entry = mp_obj_subscr(sprites_2d_list, MP_OBJ_NEW_SMALL_INT(i), MP_OBJ_SENTINEL);
        
        // Get length of sprite entry to check format
        size_t entry_len = mp_obj_get_int(mp_obj_len(sprite_entry));
        
        // Extract [sprite_index, x, y] from sub-list
        uint8_t sprite_index = mp_obj_get_int(mp_obj_subscr(sprite_entry, MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_SENTINEL));
        int16_t x = mp_obj_get_int(mp_obj_subscr(sprite_entry, MP_OBJ_NEW_SMALL_INT(1), MP_OBJ_SENTINEL));
        int16_t y = mp_obj_get_int(mp_obj_subscr(sprite_entry, MP_OBJ_NEW_SMALL_INT(2), MP_OBJ_SENTINEL));
        
        // Check for optional per-sprite transparency (4th element)
        bool use_transparency = global_transparency;
        if (entry_len > 3) {
            use_transparency = mp_obj_is_true(mp_obj_subscr(sprite_entry, MP_OBJ_NEW_SMALL_INT(3), MP_OBJ_SENTINEL));
        }
        
        // Check for optional per-sprite x_flip (5th element)
        bool x_flip = false;
        if (entry_len > 4) {
            x_flip = mp_obj_is_true(mp_obj_subscr(sprite_entry, MP_OBJ_NEW_SMALL_INT(4), MP_OBJ_SENTINEL));
        }
        
        // Call the safe sprite function with bounds checking!
        common_hal_pixelcore_PixelCore_sprite(self, &spritemap, sprite_index, x, y, use_transparency, x_flip);
    }
    
    // sprite_data_ref keeps buffer alive until here
    return mp_const_none;
}



STATIC mp_obj_t pixelcore_PixelCore_tilemap(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    // Keep Python object references alive during operation
    mp_obj_t spritemap_obj = args[1];
    mp_obj_t tilemap_obj = args[2];
    
    // Get spritemap data from Python bytes object
    mp_buffer_info_t spritemap_bufinfo;
    mp_get_buffer_raise(spritemap_obj, &spritemap_bufinfo, MP_BUFFER_READ);
    
    // Get tilemap data from Python bytes object  
    mp_buffer_info_t tilemap_bufinfo;
    mp_get_buffer_raise(tilemap_obj, &tilemap_bufinfo, MP_BUFFER_READ);
    
    // Create spritemap structure
    pixelcore_spritemap_t spritemap;
    spritemap.sprite_width = mp_obj_get_int(args[3]);    // Width of each sprite
    spritemap.sprite_height = mp_obj_get_int(args[4]);   // Height of each sprite
    spritemap.sprites_per_row = mp_obj_get_int(args[5]); // Number of sprites per row
    spritemap.data = (uint16_t *)spritemap_bufinfo.buf;
    
    // Get tilemap parameters
    uint16_t tilemap_width = mp_obj_get_int(args[6]);
    uint16_t tilemap_height = mp_obj_get_int(args[7]);
    uint16_t tile_width = mp_obj_get_int(args[8]);
    uint16_t tile_height = mp_obj_get_int(args[9]);
    int16_t render_x = mp_obj_get_int(args[10]);
    int16_t render_y = mp_obj_get_int(args[11]);
    
    // Optional transparency parameter (default false)
    bool use_transparency = false;
    if (n_args > 12) {
        use_transparency = mp_obj_is_true(args[12]);
    }
    
    // Ensure buffers stay alive during operation by keeping references on stack
    common_hal_pixelcore_PixelCore_tilemap(self, &spritemap, (const uint8_t *)tilemap_bufinfo.buf,
                                          tilemap_width, tilemap_height, tile_width, tile_height,
                                          render_x, render_y, use_transparency);
    
    // Object references ensure buffers stay alive until function returns
    (void)spritemap_obj;  // Prevent compiler from optimizing away the reference
    (void)tilemap_obj;    // Prevent compiler from optimizing away the reference
    
    return mp_const_none;
}

STATIC mp_obj_t pixelcore_PixelCore_row_fill(size_t n_args, const mp_obj_t *args) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    // Get buffer info from colors object
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
    
    // Ensure buffer contains 16-bit values
    if (bufinfo.len % 2 != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("Color buffer must contain 16-bit values (even number of bytes)"));
    }
    
    uint16_t color_count = bufinfo.len / 2;
    const uint16_t *colors = (const uint16_t *)bufinfo.buf;
    
    // Optional start_row parameter (default: 0)
    uint16_t start_row = 0;
    if (n_args > 2) {
        start_row = mp_obj_get_int(args[2]);
    }
    
    // Optional row_count parameter (default: use all colors)
    uint16_t row_count = color_count;
    if (n_args > 3) {
        row_count = mp_obj_get_int(args[3]);
    }
    
    common_hal_pixelcore_PixelCore_row_fill(self, colors, color_count, start_row, row_count);
    return mp_const_none;
}

// Function object definitions (after ALL function implementations)
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pixelcore_PixelCore_deinit_obj, pixelcore_PixelCore_deinit);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pixelcore_PixelCore_clear_obj, pixelcore_PixelCore_clear);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pixelcore_PixelCore_update_obj, pixelcore_PixelCore_update);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_fill_obj, 2, 4, pixelcore_PixelCore_fill);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_pixel_obj, 4, 4, pixelcore_PixelCore_pixel);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_hline_obj, 5, 5, pixelcore_PixelCore_hline);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_vline_obj, 5, 5, pixelcore_PixelCore_vline);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_rect_obj, 6, 6, pixelcore_PixelCore_rect);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_box_obj, 6, 6, pixelcore_PixelCore_box);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_sprite_obj, 8, 10, pixelcore_PixelCore_sprite);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_sprites_obj, 6, 7, pixelcore_PixelCore_sprites);

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_tilemap_obj, 12, 13, pixelcore_PixelCore_tilemap);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_line_obj, 6, 6, pixelcore_PixelCore_line);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_circle_obj, 5, 5, pixelcore_PixelCore_circle);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixelcore_PixelCore_row_fill_obj, 2, 4, pixelcore_PixelCore_row_fill);




// Define locals dictionary with methods - AFTER function objects are defined
const mp_rom_map_elem_t pixelcore_PixelCore_locals_dict_table[] = {
    // Context manager methods
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&mp_identity_obj) },
    
    // Instance methods
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pixelcore_PixelCore_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&pixelcore_PixelCore_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&pixelcore_PixelCore_update_obj) },

    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&pixelcore_PixelCore_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&pixelcore_PixelCore_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_hline), MP_ROM_PTR(&pixelcore_PixelCore_hline_obj) },
    { MP_ROM_QSTR(MP_QSTR_vline), MP_ROM_PTR(&pixelcore_PixelCore_vline_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&pixelcore_PixelCore_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_box), MP_ROM_PTR(&pixelcore_PixelCore_box_obj) },
    { MP_ROM_QSTR(MP_QSTR_sprite), MP_ROM_PTR(&pixelcore_PixelCore_sprite_obj) },
    { MP_ROM_QSTR(MP_QSTR_sprites), MP_ROM_PTR(&pixelcore_PixelCore_sprites_obj) },

    { MP_ROM_QSTR(MP_QSTR_tilemap), MP_ROM_PTR(&pixelcore_PixelCore_tilemap_obj) },
    { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&pixelcore_PixelCore_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&pixelcore_PixelCore_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_row_fill), MP_ROM_PTR(&pixelcore_PixelCore_row_fill_obj) },

};
MP_DEFINE_CONST_DICT(pixelcore_PixelCore_locals_dict, pixelcore_PixelCore_locals_dict_table);

// Property access functions
STATIC void pixelcore_PixelCore_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    pixelcore_PixelCore_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (dest[0] == MP_OBJ_NULL) {
        // Load attribute (getter)
        if (attr == MP_QSTR_use_direct_dma) {
            dest[0] = mp_obj_new_bool(self->use_direct_dma);
            return;
        }
        
        // Fall back to locals dictionary for methods
        mp_obj_dict_t *locals_dict = (mp_obj_dict_t*)&pixelcore_PixelCore_locals_dict;
        mp_map_elem_t *elem = mp_map_lookup(&locals_dict->map, MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);
        if (elem != NULL) {
            // Create a bound method object so 'self' is automatically passed
            dest[0] = mp_obj_new_bound_meth(elem->value, self_in);
            return;
        }
    } else if (dest[1] != MP_OBJ_NULL) {
        // Store attribute (setter)
        if (attr == MP_QSTR_use_direct_dma) {
            self->use_direct_dma = mp_obj_is_true(dest[1]);
            dest[0] = MP_OBJ_NULL; // Success
            return;
        }
    }
}

// Define the PixelCore type - FINAL definition
MP_DEFINE_CONST_OBJ_TYPE(
    pixelcore_PixelCore_type,
    MP_QSTR_PixelCore,
    MP_TYPE_FLAG_NONE,
    make_new, pixelcore_PixelCore_make_new,
    attr, pixelcore_PixelCore_attr,
    locals_dict, &pixelcore_PixelCore_locals_dict
);
