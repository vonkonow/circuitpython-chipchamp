#include "py/obj.h"
#include "py/runtime.h"
#include "shared-bindings/pixelcore/__init__.h"

// This file is intentionally minimal as the implementation is in pixelcore.c 

#if CIRCUITPY_PIXELCORE

// Initialize the module
void pixelcore_module_init(void) {
    // Initialize any hardware or resources needed
    // This is called during CircuitPython startup
}

// Register module cleanup if needed
void pixelcore_module_deinit(void) {
    // Cleanup any resources
}

#endif // CIRCUITPY_PIXELCORE 