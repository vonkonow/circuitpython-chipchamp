#ifndef MICROPY_INCLUDED_SHARED_BINDINGS_PIXELCORE_PIXELCORE_H
#define MICROPY_INCLUDED_SHARED_BINDINGS_PIXELCORE_PIXELCORE_H

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/objstr.h"

// Import other dependencies here to avoid circular includes
#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/digitalio/DigitalInOut.h"

// Type and module declarations
extern const mp_obj_type_t pixelcore_PixelCore_type;
extern const mp_obj_module_t pixelcore_module;

// Constructor prototype (using void* to avoid type conflicts)
mp_obj_t pixelcore_PixelCore_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args);

#endif // MICROPY_INCLUDED_SHARED_BINDINGS_PIXELCORE_PIXELCORE_H 