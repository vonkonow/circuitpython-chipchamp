// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 CircuitPython Contributors
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "shared-bindings/samplecore/__init__.h"
#include "shared-bindings/samplecore/SampleCore.h"
#include "ports/espressif/common-hal/samplecore/SampleCore.h"

//| """SampleCore module for ESP32
//| The `samplecore` module provides a high-performance 8-channel audio engine
//| optimized for ESP32-S2 microcontrollers. It supports 44.1kHz audio playback
//| with MIDI note control and minimal CPU overhead.
//| 
//| Key features:
//| - 8 independent audio channels
//| - 44.1kHz sample rate with 8-bit DAC output
//| - MIDI note support (0-127) with automatic pitch calculation
//| - Both one-shot samples and looping wavetables
//| - Fixed-point arithmetic for optimal performance
//| 
//| Example usage::
//| 
//|     import samplecore
//|     
//|     # Create audio engine
//|     audio = samplecore.SampleCore()
//|     
//|     # Play a sample on channel 0 at MIDI note 60 (C4)
//|     audio.play(channel=0, data=my_sample_data, midi_note=60, volume=256)
//|     
//|     # Play a looping wavetable on channel 1
//|     audio.play(channel=1, data=sine_wave, midi_note=72, volume=64, loop=True)
//|     
//|     # Control playback
//|     audio.set_volume(channel=0, volume=128)
//|     audio.stop(channel=0)
//|     
//|     # Clean up
//|     audio.deinit()
//| """

// Module-level garbage collection function
// Called by CircuitPython's supervisor to track Python objects
void samplecore_gc_collect(void) {
    // SampleCore objects are tracked by CircuitPython's GC system automatically
    // The global instance pointer is managed in common-hal and doesn't need
    // special GC handling here since it's not a Python object reference
}

static const mp_rom_map_elem_t samplecore_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_samplecore) },
    { MP_ROM_QSTR(MP_QSTR_SampleCore), MP_ROM_PTR(&samplecore_SampleCore_type) },
};

static MP_DEFINE_CONST_DICT(samplecore_module_globals, samplecore_module_globals_table);

const mp_obj_module_t samplecore_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&samplecore_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_samplecore, samplecore_module);
