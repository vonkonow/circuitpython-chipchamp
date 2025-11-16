// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 CircuitPython Contributors
//
// SPDX-License-Identifier: MIT

#ifndef MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_SAMPLECORE_SAMPLECORE_TYPES_H
#define MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_SAMPLECORE_SAMPLECORE_TYPES_H

#include "py/obj.h"
#include "driver/gptimer.h"
#include "driver/dac_oneshot.h"

// Audio configuration constants
#define SAMPLECORE_SAMPLE_RATE 44100
#define SAMPLECORE_NUM_CHANNELS 8
#define SAMPLECORE_PITCH_SHIFT 16    // 16.16 fixed point
#define SAMPLECORE_VOLUME_SHIFT 8    // 8.8 fixed point
#define SAMPLECORE_DAC_CHANNEL DAC_CHANNEL_1  // GPIO17
#define SAMPLECORE_TIMER_ALARM_US (1000000 / SAMPLECORE_SAMPLE_RATE)

// Audio channel structure (optimized for performance)
typedef struct {
    const int8_t* sample;      // Pointer to sample data
    uint32_t position;         // 16.16 fixed point position
    uint32_t increment;        // 16.16 fixed point increment (pitch)
    uint32_t length;           // Sample length in samples
    uint16_t volume;           // Volume level (0-511)
    uint8_t active;            // Channel active flag
    uint8_t loop;              // Loop flag
} samplecore_channel_t;

// Main SampleCore object structure
typedef struct samplecore_SampleCore_obj {
    mp_obj_base_t base;
    
    // Audio channels - cache aligned for optimal memory access
    samplecore_channel_t channels[SAMPLECORE_NUM_CHANNELS] __attribute__((aligned(32)));
    
    // ISR state
    volatile bool isr_enabled;
    volatile uint8_t active_channels;
    
    // Hardware state
    bool initialized;
    bool deinited;
    gptimer_handle_t timer_handle;  // ESP-IDF v5.x timer handle
    dac_oneshot_handle_t dac_handle; // ESP-IDF v5.x DAC handle
} samplecore_SampleCore_obj_t;

#endif // MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_SAMPLECORE_SAMPLECORE_TYPES_H 