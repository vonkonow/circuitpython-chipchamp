// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 CircuitPython Contributors
//
// SPDX-License-Identifier: MIT

#ifndef MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_SAMPLECORE_SAMPLECORE_H
#define MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_SAMPLECORE_SAMPLECORE_H

#include "py/obj.h"
#include "ports/espressif/common-hal/samplecore/SampleCore_types.h"
#include "driver/gptimer.h"

// Include the type definition from the shared-bindings
typedef struct samplecore_SampleCore_obj samplecore_SampleCore_obj_t;

// ISR function prototype
bool samplecore_timer_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void* user_ctx);

// Common HAL functions
void common_hal_samplecore_SampleCore_construct(samplecore_SampleCore_obj_t *self);
void common_hal_samplecore_SampleCore_deinit(samplecore_SampleCore_obj_t *self);
bool common_hal_samplecore_SampleCore_deinited(samplecore_SampleCore_obj_t *self);

void common_hal_samplecore_SampleCore_play(samplecore_SampleCore_obj_t *self,
    uint8_t channel, const int8_t* data, uint32_t length, 
    uint8_t midi_note, uint16_t volume, bool loop);

void common_hal_samplecore_SampleCore_stop_channel(samplecore_SampleCore_obj_t *self, uint8_t channel);
void common_hal_samplecore_SampleCore_stop_all(samplecore_SampleCore_obj_t *self);
void common_hal_samplecore_SampleCore_set_volume(samplecore_SampleCore_obj_t *self, uint8_t channel, uint16_t volume);

bool common_hal_samplecore_SampleCore_is_active(samplecore_SampleCore_obj_t *self, uint8_t channel);
uint8_t common_hal_samplecore_SampleCore_active_count(samplecore_SampleCore_obj_t *self);

// Reset function for CircuitPython soft reset
void samplecore_reset(void);

#endif // MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_SAMPLECORE_SAMPLECORE_H 