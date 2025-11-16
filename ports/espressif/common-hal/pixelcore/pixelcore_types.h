#ifndef MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_PIXELCORE_PIXELCORE_TYPES_H
#define MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_PIXELCORE_PIXELCORE_TYPES_H

#include "py/obj.h"
#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/digitalio/DigitalInOut.h"
#include "driver/spi_master.h"

// Sprite map structure
typedef struct {
    uint16_t sprite_width;
    uint16_t sprite_height;
    uint16_t sprites_per_row;
    uint16_t *data;
} pixelcore_spritemap_t;

// Main display object structure
typedef struct {
    mp_obj_base_t base;
    busio_spi_obj_t *spi;
    digitalio_digitalinout_obj_t *cs;
    digitalio_digitalinout_obj_t *dc;
    digitalio_digitalinout_obj_t *rst;
    uint16_t *buffer;
    uint16_t width;          // Display width
    uint16_t height;         // Display height
    uint32_t spi_freq;       // SPI frequency
    bool deinited;
    
    // DMA optimization fields
    bool use_direct_dma;           // Use direct ESP-IDF SPI for maximum performance
    spi_device_handle_t spi_device; // Direct SPI device handle for DMA transfers
    
    // Stored SPI configuration (captured during construction before spi.deinit())
    spi_host_device_t stored_host_id;  // SPI host ID (SPI2_HOST, SPI3_HOST, etc.)
    int stored_mosi_pin;               // MOSI pin number
    int stored_clk_pin;                // Clock pin number
} pixelcore_PixelCore_obj_t;

#endif // MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_PIXELCORE_PIXELCORE_TYPES_H 