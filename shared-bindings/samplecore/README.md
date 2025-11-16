# SampleCore - High-Performance Audio Engine

The `samplecore` module provides a high-performance 8-channel audio engine optimized for ESP32-S2 microcontrollers. It delivers hardware-accelerated audio playback with minimal CPU overhead and precise timing.

## Features

- **8 Independent Channels**: Play different audio simultaneously
- **44.1kHz Sample Rate**: Professional audio quality
- **8-bit DAC Output**: Hardware DAC on GPIO17 (ESP32-S2)
- **MIDI Note Control**: Automatic pitch calculation (0-127)
- **Flexible Audio Data**: Supports both wavetables and samples of any length
- **Low CPU Overhead**: Optimized ISR with fixed-point arithmetic
- **Unified API**: Single `play()` method for all audio types
- **Auto ISR Management**: Enables/disables automatically based on usage
- **Context Manager Support**: Automatic resource cleanup
- **Buffer Safety**: Advanced memory management prevents crashes

## Audio Data Types

SampleCore supports two main types of audio data:

### 1. **Wavetables** - Synthesized Waveforms
- **Purpose**: Continuous tones, musical synthesis
- **Length**: Typically 100-256 samples (one waveform cycle)
- **Usage**: Loop continuously for sustained notes
- **Examples**: Sine waves, square waves, sawtooth waves

### 2. **Samples** - Recorded Audio
- **Purpose**: Sound effects, drums, musical instruments
- **Length**: Any length (from short clicks to long recordings)
- **Usage**: One-shot playback or pitched musical phrases
- **Examples**: Drum hits, violin recordings, sound effects

## Hardware Requirements

- ESP32-S2 microcontroller (ESP32-S2 Mini recommended)
- Audio output connected to GPIO17 (built-in DAC)
- Optional: External amplifier for better sound quality

## Quick Start Examples

### Wavetable Synthesis
```python
import samplecore
import math

# Generate a 168-sample sine wave wavetable (optimized for C4 reference pitch)
def generate_sine_wave(length=168):
    samples = []
    for i in range(length):
        # Generate sine wave with full amplitude (-127 to +127)
        sample = int(127 * math.sin(2 * math.pi * i / length))
        samples.append(sample)
    return bytes(samples)

# Create audio engine
audio = samplecore.SampleCore()

# Generate wavetable
sine_wave = generate_sine_wave()

# Play continuous sine wave at C4 (reference pitch)
audio.play(channel=0, data=sine_wave, midi_note=60, volume=256, loop=True)

# Clean up when done
audio.deinit()
```

### Sample Playback
```python
import samplecore

# Load a drum sample (any length)
kick_sample = load_audio_file("kick.raw")  # Your audio loading function

# Create audio engine
audio = samplecore.SampleCore()

# Play drum sample at original pitch (C4 reference)
audio.play(channel=0, data=kick_sample, midi_note=60, volume=256, loop=False)

# Play same sample pitched up (one octave higher)
audio.play(channel=1, data=kick_sample, midi_note=72, volume=256, loop=False)

# Play violin sample as melody
violin_sample = load_audio_file("violin_c4.raw")  # Recorded at C4
audio.play(channel=2, data=violin_sample, midi_note=67, volume=200, loop=False)  # Play at G4

audio.deinit()
```

## Pitch Reference System

SampleCore uses **MIDI note 60 (C4, 261.6Hz)** as the pitch reference:

### For Wavetables:
- **Optimal Length**: 168 samples for perfect C4 tuning
- **Tuning**: 168 samples = 168.5 samples at C4, so wavetables play at correct pitch
- **Best Practice**: Use 168 samples for synthesis tuned to C4 reference
- **Usage**: Continuous synthesis, electronic music

### For Samples:
- **Reference Pitch**: C4 (261.6Hz) = original playback speed
- **Any Length**: From short (100 samples) to long (100,000+ samples)
- **Pitch Control**: MIDI notes transpose the sample up/down
- **Usage**: Realistic instruments, sound effects, drums

### MIDI Note Effects:
- **MIDI 60 (C4)**: Original sample speed (reference pitch)
- **MIDI 72 (C5)**: 2x speed = one octave higher
- **MIDI 48 (C3)**: 0.5x speed = one octave lower
- **MIDI 69 (A4)**: 1.68x speed = perfect fifth + minor third higher

## API Reference

### SampleCore Class

#### Constructor

```python
audio = samplecore.SampleCore()
```

Creates a new SampleCore instance. Only one instance is allowed per program.

#### Methods

##### `play(*, channel, data, midi_note=60, volume=256, loop=False)`

Play audio data on the specified channel.

**Parameters:**
- `channel` (int): Audio channel (0-7)
- `data` (bytes): Audio data as signed 8-bit samples (-127 to +127)
- `midi_note` (int): MIDI note number (0-127, default 60 = C4)
- `volume` (int): Volume level (0-511, 256 = 100%)
- `loop` (bool): True for continuous looping, False for one-shot playback

##### `stop(channel=None)`

Stop audio playback.

**Parameters:**
- `channel` (int, optional): Channel to stop (0-7), or None to stop all channels

##### `set_volume(*, channel, volume)`

Set the volume for a specific channel.

**Parameters:**
- `channel` (int): Audio channel (0-7)
- `volume` (int): Volume level (0-511, 256 = 100%)

##### `is_active(channel)`

Check if a channel is currently playing audio.

**Returns:** `bool` - True if active, False otherwise

##### `active_count()`

Get the number of currently active channels.

**Returns:** `int` - Number of active channels (0-8)

##### `deinit()`

Deinitialize the audio engine and release hardware resources.

### Context Manager Support

SampleCore supports the context manager protocol for automatic cleanup:

```python
with samplecore.SampleCore() as audio:
    audio.play(channel=0, data=my_data, midi_note=60)
    # Automatically cleaned up when exiting the block
```

## Audio Data Format

### Sample Rate
All audio data should be at 44.1kHz sample rate. The engine does not perform sample rate conversion.

### Data Format
Audio data must be provided as signed 8-bit samples in a `bytes` object:
- **Range**: -127 to +127
- **Format**: Signed 8-bit integer
- **Encoding**: Raw bytes (no header)

## MIDI Note Reference

MIDI notes control playback pitch with **C4 (261.6Hz)** as the reference:

| MIDI | Note | Frequency | Pitch Ratio | Use Case |
|------|------|-----------|-------------|----------|
| 48   | C3   | 130.8 Hz  | 0.5x        | Bass sounds |
| 60   | C4   | 261.6 Hz  | 1.0x        | **Reference pitch** |
| 69   | A4   | 440.0 Hz  | 1.68x       | Concert pitch |
| 72   | C5   | 523.3 Hz  | 2.0x        | One octave higher |
| 84   | C6   | 1047 Hz   | 4.0x        | Two octaves higher |

## Technical Details

### Audio Engine Architecture
- **Sample Rate**: 44.1kHz (22.67μs timer intervals)
- **Channels**: 8 independent channels with individual volume control
- **Pitch Reference**: MIDI 60 (C4, 261.6Hz) = 1.0x playback speed
- **ISR Optimization**: Unrolled loops and fixed-point arithmetic
- **Memory Management**: Python object references kept alive during playback
- **Hardware**: ESP32-S2 DAC on GPIO17

### Volume Control
- **Range**: 0-511 (9-bit resolution)
- **Linear Scale**: 256 = 100% volume
- **Mixing**: All channels mixed before DAC output
- **Per-Channel**: Individual volume control per channel

The SampleCore module provides professional-quality audio capabilities for both electronic synthesis and realistic sample playback, maintaining the clean, Pythonic API that CircuitPython users expect.
