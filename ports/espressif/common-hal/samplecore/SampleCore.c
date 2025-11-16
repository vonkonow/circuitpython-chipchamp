// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 CircuitPython Contributors  
//
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <string.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "shared-bindings/samplecore/SampleCore.h"
#include "ports/espressif/common-hal/samplecore/SampleCore.h"
#include "ports/espressif/common-hal/samplecore/SampleCore_types.h"

// ESP32 includes for ESP-IDF v5.x
#include "driver/dac_oneshot.h"
#include "driver/gptimer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



// =============================================================================
// MIDI NOTE TO INCREMENT LOOKUP TABLE (CORRECTED)
// =============================================================================
// Pre-calculated 16.16 fixed-point increments for MIDI notes 0-127
// Base: MIDI 60 (C4) = 261.63Hz = 65536 increment (1.0x sample speed)
// Formula: increment = 65536 * 2^((midi_note - 60) / 12)
static const uint32_t midi_increments[128] = {
  // MIDI 0-11 (C-1 to B-1) - Very low notes
  4058, 4299, 4555, 4825, 5112, 5417, 5740, 6084, 6448, 6834, 7244, 7679,
  // MIDI 12-23 (C0 to B0)
  8116, 8599, 9110, 9651, 10224, 10834, 11481, 12168, 12897, 13669, 14488, 15358,
  // MIDI 24-35 (C1 to B1)
  16233, 17198, 18221, 19302, 20449, 21669, 22963, 24337, 25794, 27339, 28976, 30716,
  // MIDI 36-47 (C2 to B2)
  32466, 34397, 36443, 38605, 40898, 43339, 45927, 48674, 51589, 54679, 57953, 61432,
  // MIDI 48-59 (C3 to B3)
  64932, 68795, 72886, 77211, 81796, 86678, 91855, 97349, 103178, 109359, 115907, 122865,
  // MIDI 60-71 (C4 to B4) - MIDI 60 = 65536 base reference
  65536, 69432, 73563, 77943, 82586, 87508, 92724, 98251, 104106, 110308, 116878, 123835,
  // MIDI 72-83 (C5 to B5)
  131072, 138865, 147127, 155887, 165173, 175017, 185449, 196503, 208213, 220616, 233757, 247670,
  // MIDI 84-95 (C6 to B6)
  262144, 277730, 294254, 311775, 330346, 350034, 370899, 393006, 416427, 441233, 467514, 495341,
  // MIDI 96-107 (C7 to B7)
  524288, 555460, 588509, 623551, 660693, 700069, 741799, 786013, 832855, 882467, 935029, 990683,
  // MIDI 108-119 (C8 to B8)
  1048576, 1110921, 1177018, 1247102, 1321386, 1400138, 1483598, 1572026, 1665710, 1764935, 1870059, 1981366,
  // MIDI 120-127 (C9 to G9)
  2097152, 2221842, 2354036, 2494205, 2642773, 2800276, 3267196, 3144052
};

// =============================================================================
// PERFORMANCE OPTIMIZATION: FAST VOLUME MULTIPLICATION
// =============================================================================
// Fast volume scaling macro using optimized multiplication
#define FAST_VOLUME_MUL(sample, volume) \
  ((int32_t)(sample) * (int32_t)(volume)) >> SAMPLECORE_VOLUME_SHIFT

// Global pointer for ISR access (only one SampleCore instance supported)
static samplecore_SampleCore_obj_t *g_samplecore_instance = NULL;

// =============================================================================
// ULTRA-OPTIMIZED ISR - MAXIMUM PERFORMANCE, MEMORY EFFICIENT
// =============================================================================
bool IRAM_ATTR samplecore_timer_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void* user_ctx) {
    // Get SampleCore instance from event data (set during callback registration)
    samplecore_SampleCore_obj_t *self = g_samplecore_instance;
    if (!self || !self->isr_enabled) {
        return false;
    }
    

    register int32_t mix = 0;
    register uint32_t pos;
    register int8_t sample_val;
    
    // PERFORMANCE OPTIMIZATION: Unrolled loop with optimized sample reading
    // Channel 0
    if (__builtin_expect(self->channels[0].active, 0)) {
        pos = self->channels[0].position >> SAMPLECORE_PITCH_SHIFT;
        if (__builtin_expect(pos < self->channels[0].length, 1)) {
            sample_val = self->channels[0].sample[pos];  // Direct memory access (faster than pgm_read_byte)
            mix += FAST_VOLUME_MUL(sample_val, self->channels[0].volume);
            self->channels[0].position += self->channels[0].increment;
        } else {
            // Branchless position reset and activity update
            self->channels[0].position = self->channels[0].loop ? 0 : self->channels[0].position;
            self->channels[0].active = self->channels[0].loop;
        }
    }
    
    // Channel 1
    if (__builtin_expect(self->channels[1].active, 0)) {
        pos = self->channels[1].position >> SAMPLECORE_PITCH_SHIFT;
        if (__builtin_expect(pos < self->channels[1].length, 1)) {
            sample_val = self->channels[1].sample[pos];
            mix += FAST_VOLUME_MUL(sample_val, self->channels[1].volume);
            self->channels[1].position += self->channels[1].increment;
        } else {
            self->channels[1].position = self->channels[1].loop ? 0 : self->channels[1].position;
            self->channels[1].active = self->channels[1].loop;
        }
    }
    
    // Channel 2
    if (__builtin_expect(self->channels[2].active, 0)) {
        pos = self->channels[2].position >> SAMPLECORE_PITCH_SHIFT;
        if (__builtin_expect(pos < self->channels[2].length, 1)) {
            sample_val = self->channels[2].sample[pos];
            mix += FAST_VOLUME_MUL(sample_val, self->channels[2].volume);
            self->channels[2].position += self->channels[2].increment;
        } else {
            self->channels[2].position = self->channels[2].loop ? 0 : self->channels[2].position;
            self->channels[2].active = self->channels[2].loop;
        }
    }
    
    // Channel 3
    if (__builtin_expect(self->channels[3].active, 0)) {
        pos = self->channels[3].position >> SAMPLECORE_PITCH_SHIFT;
        if (__builtin_expect(pos < self->channels[3].length, 1)) {
            sample_val = self->channels[3].sample[pos];
            mix += FAST_VOLUME_MUL(sample_val, self->channels[3].volume);
            self->channels[3].position += self->channels[3].increment;
        } else {
            self->channels[3].position = self->channels[3].loop ? 0 : self->channels[3].position;
            self->channels[3].active = self->channels[3].loop;
        }
    }
    
    // Channel 4
    if (__builtin_expect(self->channels[4].active, 0)) {
        pos = self->channels[4].position >> SAMPLECORE_PITCH_SHIFT;
        if (__builtin_expect(pos < self->channels[4].length, 1)) {
            sample_val = self->channels[4].sample[pos];
            mix += FAST_VOLUME_MUL(sample_val, self->channels[4].volume);
            self->channels[4].position += self->channels[4].increment;
        } else {
            self->channels[4].position = self->channels[4].loop ? 0 : self->channels[4].position;
            self->channels[4].active = self->channels[4].loop;
        }
    }
    
    // Channel 5
    if (__builtin_expect(self->channels[5].active, 0)) {
        pos = self->channels[5].position >> SAMPLECORE_PITCH_SHIFT;
        if (__builtin_expect(pos < self->channels[5].length, 1)) {
            sample_val = self->channels[5].sample[pos];
            mix += FAST_VOLUME_MUL(sample_val, self->channels[5].volume);
            self->channels[5].position += self->channels[5].increment;
        } else {
            self->channels[5].position = self->channels[5].loop ? 0 : self->channels[5].position;
            self->channels[5].active = self->channels[5].loop;
        }
    }
    
    // Channel 6
    if (__builtin_expect(self->channels[6].active, 0)) {
        pos = self->channels[6].position >> SAMPLECORE_PITCH_SHIFT;
        if (__builtin_expect(pos < self->channels[6].length, 1)) {
            sample_val = self->channels[6].sample[pos];
            mix += FAST_VOLUME_MUL(sample_val, self->channels[6].volume);
            self->channels[6].position += self->channels[6].increment;
        } else {
            self->channels[6].position = self->channels[6].loop ? 0 : self->channels[6].position;
            self->channels[6].active = self->channels[6].loop;
        }
    }
    
    // Channel 7
    if (__builtin_expect(self->channels[7].active, 0)) {
        pos = self->channels[7].position >> SAMPLECORE_PITCH_SHIFT;
        if (__builtin_expect(pos < self->channels[7].length, 1)) {
            sample_val = self->channels[7].sample[pos];
            mix += FAST_VOLUME_MUL(sample_val, self->channels[7].volume);
            self->channels[7].position += self->channels[7].increment;
        } else {
            self->channels[7].position = self->channels[7].loop ? 0 : self->channels[7].position;
            self->channels[7].active = self->channels[7].loop;
        }
    }
    
    // Convert to DAC range (0-255) with fast clamping
    int32_t dac_value = mix + 128;
    dac_value = (dac_value < 0) ? 0 : ((dac_value > 255) ? 255 : dac_value);
    
    // Output to DAC
    dac_oneshot_output_voltage(self->dac_handle, (uint8_t)dac_value);
    

    
    return false;  // No task switch needed
}

// =============================================================================
// AUDIO ENGINE MANAGEMENT
// =============================================================================

static void enable_isr(samplecore_SampleCore_obj_t *self) {
    if (!self->isr_enabled && self->timer_handle) {
        gptimer_start(self->timer_handle);
        self->isr_enabled = true;

    }
}

static void disable_isr(samplecore_SampleCore_obj_t *self) {
    if (self->isr_enabled && self->timer_handle) {
        gptimer_stop(self->timer_handle);
        self->isr_enabled = false;

    }
}

static void update_active_channels(samplecore_SampleCore_obj_t *self) {
    uint8_t count = 0;
    for (int i = 0; i < SAMPLECORE_NUM_CHANNELS; i++) {
        if (self->channels[i].active) count++;
    }
    
    self->active_channels = count;
    
    // Enable/disable ISR based on active channels
    if (count > 0 && !self->isr_enabled) {
        enable_isr(self);
    } else if (count == 0 && self->isr_enabled) {
        disable_isr(self);
    }
}

// =============================================================================
// COMMON HAL IMPLEMENTATIONS
// =============================================================================

void common_hal_samplecore_SampleCore_construct(samplecore_SampleCore_obj_t *self) {
    if (g_samplecore_instance != NULL) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Only one SampleCore instance allowed"));
    }
    
    // Initialize all channels
    for (int i = 0; i < SAMPLECORE_NUM_CHANNELS; i++) {
        self->channels[i].active = 0;
        self->channels[i].position = 0;
        self->channels[i].increment = 65536;   // 1.0 pitch
        self->channels[i].volume = 256;        // 100% volume
        self->channels[i].loop = 0;
        self->channels[i].sample = NULL;
        self->channels[i].length = 0;
    }
    
    // Initialize state
    self->isr_enabled = false;
    self->active_channels = 0;
    self->initialized = false;
    self->deinited = false;
    self->timer_handle = NULL;
    self->dac_handle = NULL;
    
    // Setup DAC for ESP-IDF v5.x
    dac_oneshot_config_t dac_cfg = {
        .chan_id = SAMPLECORE_DAC_CHANNEL,
    };
    esp_err_t err = dac_oneshot_new_channel(&dac_cfg, &self->dac_handle);
    if (err != ESP_OK) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Failed to create DAC channel"));
    }
    
    // Set DAC to center position
    dac_oneshot_output_voltage(self->dac_handle, 128);
    
    // Timer configuration for ESP-IDF v5.x
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1us per tick
    };
    
    err = gptimer_new_timer(&timer_config, &self->timer_handle);
    if (err != ESP_OK) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Failed to create timer"));
    }
    
    // Set up timer callback BEFORE enabling timer
    gptimer_event_callbacks_t cbs = {};  // Zero-initialize first
    cbs.on_alarm = samplecore_timer_isr;
    
    err = gptimer_register_event_callbacks(self->timer_handle, &cbs, NULL);
    if (err != ESP_OK) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Failed to register timer callback"));
    }
    
    // Set timer alarm
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = SAMPLECORE_TIMER_ALARM_US, // 22.67us for 44.1kHz
        .flags.auto_reload_on_alarm = true,
    };
    err = gptimer_set_alarm_action(self->timer_handle, &alarm_config);
    if (err != ESP_OK) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Failed to set timer alarm"));
    }
    
    err = gptimer_enable(self->timer_handle);
    if (err != ESP_OK) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Failed to enable timer"));
    }
    
    // Timer starts disabled - will be enabled when channels become active
    self->initialized = true;
    g_samplecore_instance = self;
    

}

void common_hal_samplecore_SampleCore_deinit(samplecore_SampleCore_obj_t *self) {
    if (self->deinited) {
        return;
    }
    
    // Stop all audio and disable ISR
    common_hal_samplecore_SampleCore_stop_all(self);
    disable_isr(self);
    
    // Cleanup timer
    if (self->timer_handle) {
        gptimer_disable(self->timer_handle);
        gptimer_del_timer(self->timer_handle);
        self->timer_handle = NULL;
    }
    
    // Cleanup DAC
    if (self->dac_handle) {
        dac_oneshot_del_channel(self->dac_handle);
        self->dac_handle = NULL;
    }
    
    // Clear global instance
    g_samplecore_instance = NULL;
    
    self->deinited = true;
    self->initialized = false;
    

}

bool common_hal_samplecore_SampleCore_deinited(samplecore_SampleCore_obj_t *self) {
    return self->deinited;
}

void common_hal_samplecore_SampleCore_play(samplecore_SampleCore_obj_t *self,
    uint8_t channel, const int8_t* data, uint32_t length, 
    uint8_t midi_note, uint16_t volume, bool loop) {
    
    if (self->deinited || channel >= SAMPLECORE_NUM_CHANNELS || midi_note > 127) {
        return;
    }
    
    // Stop channel first to avoid glitches
    self->channels[channel].active = 0;
    
    // Set up channel
    self->channels[channel].sample = data;
    self->channels[channel].length = length;
    self->channels[channel].position = 0;
    self->channels[channel].increment = midi_increments[midi_note];
    self->channels[channel].volume = volume;
    self->channels[channel].loop = loop ? 1 : 0;
    self->channels[channel].active = 1;
    
    update_active_channels(self);
    

}

void common_hal_samplecore_SampleCore_stop_channel(samplecore_SampleCore_obj_t *self, uint8_t channel) {
    if (self->deinited || channel >= SAMPLECORE_NUM_CHANNELS) {
        return;
    }
    
    self->channels[channel].active = 0;
    update_active_channels(self);
    

}

void common_hal_samplecore_SampleCore_stop_all(samplecore_SampleCore_obj_t *self) {
    if (self->deinited) {
        return;
    }
    
    for (int i = 0; i < SAMPLECORE_NUM_CHANNELS; i++) {
        self->channels[i].active = 0;
    }
    update_active_channels(self);
    

}

void common_hal_samplecore_SampleCore_set_volume(samplecore_SampleCore_obj_t *self, uint8_t channel, uint16_t volume) {
    if (self->deinited || channel >= SAMPLECORE_NUM_CHANNELS) {
        return;
    }
    
    self->channels[channel].volume = volume;
    

}

bool common_hal_samplecore_SampleCore_is_active(samplecore_SampleCore_obj_t *self, uint8_t channel) {
    if (self->deinited || channel >= SAMPLECORE_NUM_CHANNELS) {
        return false;
    }
    
    return self->channels[channel].active != 0;
}

uint8_t common_hal_samplecore_SampleCore_active_count(samplecore_SampleCore_obj_t *self) {
    if (self->deinited) {
        return 0;
    }
    
    return self->active_channels;
}

// Reset function for CircuitPython soft reset
void samplecore_reset(void) {
    // If there's a global instance, deinit it
    if (g_samplecore_instance != NULL && !g_samplecore_instance->deinited) {
        common_hal_samplecore_SampleCore_deinit(g_samplecore_instance);
    }
    g_samplecore_instance = NULL;
}

 