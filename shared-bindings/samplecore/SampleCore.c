// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 CircuitPython Contributors
//
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <string.h>

#include "py/objproperty.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "shared/runtime/context_manager_helpers.h"
#include "shared-bindings/samplecore/SampleCore.h"

#include "shared-bindings/util.h"

// Include common hal interface
#include "ports/espressif/common-hal/samplecore/SampleCore.h"

//| class SampleCore:
//|     """High-performance 8-channel audio engine for ESP32-S2.
//|     
//|     Provides hardware-accelerated audio playback with:
//|     - 8 independent channels
//|     - 44.1kHz sample rate
//|     - 8-bit DAC output on GPIO17
//|     - MIDI note control (0-127)
//|     - Minimal CPU overhead (<25%)
//|     """
//|
//|     def __init__(self) -> None:
//|         """Initialize the SampleCore audio engine.
//|         
//|         Sets up the hardware timer, DAC, and audio channels.
//|         The ISR starts disabled and will be enabled automatically
//|         when audio playback begins.
//|         """
//|         ...

mp_obj_t samplecore_SampleCore_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // No arguments expected
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    
    // Create object
    samplecore_SampleCore_obj_t *self = m_new_obj(samplecore_SampleCore_obj_t);
    self->base.type = &samplecore_SampleCore_type;
    
    // Initialize hardware
    common_hal_samplecore_SampleCore_construct(self);
    
    return MP_OBJ_FROM_PTR(self);
}

//| def deinit(self) -> None:
//|     """Deinitialize the audio engine.
//|     
//|     Stops all audio playback, disables the ISR, and releases hardware resources.
//|     """
//|     ...
static mp_obj_t samplecore_SampleCore_obj_deinit(mp_obj_t self_in) {
    samplecore_SampleCore_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_samplecore_SampleCore_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(samplecore_SampleCore_deinit_obj, samplecore_SampleCore_obj_deinit);

//| def __enter__(self) -> SampleCore:
//|     """Enter context manager."""
//|     return self
static mp_obj_t samplecore_SampleCore_obj___enter__(mp_obj_t self_in) {
    return self_in;
}
static MP_DEFINE_CONST_FUN_OBJ_1(samplecore_SampleCore___enter___obj, samplecore_SampleCore_obj___enter__);

//| def __exit__(self, exception_type: Optional[Type[BaseException]], exception_value: Optional[BaseException], traceback: Optional[TracebackType]) -> None:
//|     """Exit context manager and deinitialize."""
static mp_obj_t samplecore_SampleCore_obj___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    samplecore_SampleCore_obj_deinit(args[0]);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(samplecore_SampleCore___exit___obj, 4, 4, samplecore_SampleCore_obj___exit__);

//| def play(self, *, channel: int, data: ReadableBuffer, midi_note: int = 60, volume: int = 256, loop: bool = False) -> None:
//|     """Play audio data on the specified channel.
//|     
//|     Args:
//|         channel: Audio channel (0-7)
//|         data: Audio data as signed 8-bit samples (-127 to +127)
//|         midi_note: MIDI note number (0-127, default 60 = C4)
//|         volume: Volume level (0-511, 256 = 100%)
//|         loop: True for continuous looping (wavetables), False for one-shot (samples)
//|     
//|     The audio data should be signed 8-bit samples at 44.1kHz sample rate.
//|     MIDI note 60 (C4) plays at original sample rate. Higher notes play faster,
//|     lower notes play slower.
//|     """
//|     ...
static mp_obj_t samplecore_SampleCore_play(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_channel, ARG_data, ARG_midi_note, ARG_volume, ARG_loop };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_channel, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT },
        { MP_QSTR_data, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_OBJ },
        { MP_QSTR_midi_note, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 60} },
        { MP_QSTR_volume, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 256} },
        { MP_QSTR_loop, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    samplecore_SampleCore_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Validate channel
    int channel = args[ARG_channel].u_int;
    if (channel < 0 || channel >= SAMPLECORE_NUM_CHANNELS) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("channel must be 0-%d"), SAMPLECORE_NUM_CHANNELS - 1);
    }

    // Validate MIDI note
    int midi_note = args[ARG_midi_note].u_int;
    if (midi_note < 0 || midi_note > 127) {
        mp_raise_ValueError(MP_ERROR_TEXT("midi_note must be 0-127"));
    }

    // Validate volume
    int volume = args[ARG_volume].u_int;
    if (volume < 0 || volume > 511) {
        mp_raise_ValueError(MP_ERROR_TEXT("volume must be 0-511"));
    }

    // Get audio data buffer with minimal overhead
    mp_obj_t audio_data_ref = args[ARG_data].u_obj;
    mp_buffer_info_t bufinfo;
    mp_get_buffer(audio_data_ref, &bufinfo, MP_BUFFER_READ);
    
    if (bufinfo.len == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("data cannot be empty"));
    }

    // Call common hal implementation
    // Note: audio_data_ref keeps the buffer alive (prevents garbage collection)
    common_hal_samplecore_SampleCore_play(self, channel, (const int8_t*)bufinfo.buf, (uint32_t)bufinfo.len, 
                                         midi_note, volume, args[ARG_loop].u_bool);
    
    // audio_data_ref keeps buffer alive until here
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(samplecore_SampleCore_play_obj, 1, samplecore_SampleCore_play);

//| def stop(self, channel: Optional[int] = None) -> None:
//|     """Stop audio playback.
//|     
//|     Args:
//|         channel: Channel to stop (0-7), or None to stop all channels
//|     """
//|     ...
static mp_obj_t samplecore_SampleCore_stop(size_t n_args, const mp_obj_t *args) {
    samplecore_SampleCore_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    if (n_args == 1) {
        // Stop all channels
        common_hal_samplecore_SampleCore_stop_all(self);
    } else {
        // Stop specific channel
        int channel = mp_obj_get_int(args[1]);
        if (channel < 0 || channel >= SAMPLECORE_NUM_CHANNELS) {
            mp_raise_ValueError_varg(MP_ERROR_TEXT("channel must be 0-%d"), SAMPLECORE_NUM_CHANNELS - 1);
        }
        common_hal_samplecore_SampleCore_stop_channel(self, channel);
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(samplecore_SampleCore_stop_obj, 1, 2, samplecore_SampleCore_stop);

//| def set_volume(self, *, channel: int, volume: int) -> None:
//|     """Set the volume for a specific channel.
//|     
//|     Args:
//|         channel: Audio channel (0-7)
//|         volume: Volume level (0-511, 256 = 100%)
//|     """
//|     ...
static mp_obj_t samplecore_SampleCore_set_volume(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_channel, ARG_volume };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_channel, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT },
        { MP_QSTR_volume, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT },
    };
    samplecore_SampleCore_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int channel = args[ARG_channel].u_int;
    int volume = args[ARG_volume].u_int;
    
    if (channel < 0 || channel >= SAMPLECORE_NUM_CHANNELS) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("channel must be 0-%d"), SAMPLECORE_NUM_CHANNELS - 1);
    }
    
    if (volume < 0 || volume > 511) {
        mp_raise_ValueError(MP_ERROR_TEXT("volume must be 0-511"));
    }

    common_hal_samplecore_SampleCore_set_volume(self, channel, volume);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(samplecore_SampleCore_set_volume_obj, 1, samplecore_SampleCore_set_volume);

//| def is_active(self, channel: int) -> bool:
//|     """Check if a channel is currently playing audio.
//|     
//|     Args:
//|         channel: Audio channel (0-7)
//|     
//|     Returns:
//|         True if the channel is active, False otherwise
//|     """
//|     ...
static mp_obj_t samplecore_SampleCore_is_active(mp_obj_t self_in, mp_obj_t channel_in) {
    samplecore_SampleCore_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int channel = mp_obj_get_int(channel_in);
    
    if (channel < 0 || channel >= SAMPLECORE_NUM_CHANNELS) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("channel must be 0-%d"), SAMPLECORE_NUM_CHANNELS - 1);
    }
    
    return mp_obj_new_bool(common_hal_samplecore_SampleCore_is_active(self, channel));
}
static MP_DEFINE_CONST_FUN_OBJ_2(samplecore_SampleCore_is_active_obj, samplecore_SampleCore_is_active);

//| def active_count(self) -> int:
//|     """Get the number of currently active channels.
//|     
//|     Returns:
//|         Number of channels currently playing audio (0-8)
//|     """
//|     ...
static mp_obj_t samplecore_SampleCore_active_count(mp_obj_t self_in) {
    samplecore_SampleCore_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(common_hal_samplecore_SampleCore_active_count(self));
}
static MP_DEFINE_CONST_FUN_OBJ_1(samplecore_SampleCore_active_count_obj, samplecore_SampleCore_active_count);

// Define locals dictionary with methods
static const mp_rom_map_elem_t samplecore_SampleCore_locals_dict_table[] = {
    // Context manager protocol
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&samplecore_SampleCore___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&samplecore_SampleCore___exit___obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&samplecore_SampleCore_deinit_obj) },
    
    // Audio control methods
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&samplecore_SampleCore_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&samplecore_SampleCore_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_volume), MP_ROM_PTR(&samplecore_SampleCore_set_volume_obj) },
    
    // Status methods
    { MP_ROM_QSTR(MP_QSTR_is_active), MP_ROM_PTR(&samplecore_SampleCore_is_active_obj) },
    { MP_ROM_QSTR(MP_QSTR_active_count), MP_ROM_PTR(&samplecore_SampleCore_active_count_obj) },
};
static MP_DEFINE_CONST_DICT(samplecore_SampleCore_locals_dict, samplecore_SampleCore_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    samplecore_SampleCore_type,
    MP_QSTR_SampleCore,
    MP_TYPE_FLAG_NONE,
    make_new, samplecore_SampleCore_make_new,
    locals_dict, &samplecore_SampleCore_locals_dict
);
