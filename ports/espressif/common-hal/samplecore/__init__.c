#include "py/obj.h"
#include "py/runtime.h"
#include "shared-bindings/samplecore/__init__.h"

// This file is intentionally minimal as the implementation is in SampleCore.c 

#if CIRCUITPY_SAMPLECORE

// Initialize the module
void samplecore_module_init(void) {
    // Initialize any hardware or resources needed
    // This is called during CircuitPython startup
}

// Register module cleanup if needed
void samplecore_module_deinit(void) {
    // Cleanup any resources
}

#endif // CIRCUITPY_SAMPLECORE 