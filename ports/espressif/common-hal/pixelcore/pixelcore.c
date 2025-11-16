// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 Your Name
//
// SPDX-License-Identifier: MIT

#include "py/obj.h"
#include "py/runtime.h"
#include "py/qstr.h"
#include "py/objtype.h"
#include "py/mphal.h"

// Include shared bindings
#include "shared-bindings/busio/SPI.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/digitalio/DigitalInOut.h"
#include "shared-bindings/digitalio/DriveMode.h"
#include "shared-bindings/pixelcore/pixelcore.h"

// Include local implementations
#include "common-hal/digitalio/DigitalInOut.h"
#include "ports/espressif/common-hal/pixelcore/pixelcore.h"
#include "ports/espressif/common-hal/pixelcore/pixelcore_types.h"

#include "shared/runtime/context_manager_helpers.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "soc/soc.h"
#include "esp_private/spi_common_internal.h"

#include <string.h>

// Note: We use CircuitPython's SPI interface directly rather than accessing internal handles

#define TAG "pixelcore"

#ifndef STATIC
#define STATIC static
#endif

// Display Configuration
#define DISPLAY_WIDTH 160
#define DISPLAY_HEIGHT 128
#define BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * 2)

// ST7789 Commands (only used commands included)
#define ST7789_SWRESET 0x01
#define ST7789_SLPOUT 0x11
#define ST7789_NORON 0x13
#define ST7789_DISPON 0x29
#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C
#define ST7789_COLMOD 0x3A
#define ST7789_MADCTL 0x36
#define ST7789_INVCTR 0xB4

// Forward declarations of helper functions
STATIC void write_command(pixelcore_PixelCore_obj_t *self, uint8_t cmd);
STATIC void write_data(pixelcore_PixelCore_obj_t *self, const uint8_t *data, size_t len);
STATIC void write_data_byte(pixelcore_PixelCore_obj_t *self, uint8_t data);
STATIC void set_window(pixelcore_PixelCore_obj_t *self, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
STATIC void init_display(pixelcore_PixelCore_obj_t *self);

// Forward declarations of ESP32-specific functions
void esp32_pixelcore_write_data_fast(pixelcore_PixelCore_obj_t *self, const uint8_t *data, size_t len);
void esp32_pixelcore_write_data_ultra_fast(pixelcore_PixelCore_obj_t *self, const uint8_t *data, size_t len);

// Forward declarations of sprite functions
void common_hal_pixelcore_PixelCore_sprite(pixelcore_PixelCore_obj_t *self, const pixelcore_spritemap_t *spritemap,
                                  uint8_t sprite_index, int16_t x, int16_t y, bool use_transparency, bool x_flip);

// Ultra-fast sprite functions with no safety checks - use with caution!
// These functions assume:
// - Sprite is fully within display bounds
// - All pointers are valid
// - Sprite dimensions are correct

// Ultra-fast sprite rendering without transparency (fastest)
STATIC void sprite_fast_opaque(pixelcore_PixelCore_obj_t *self, const pixelcore_spritemap_t *spritemap,
                               uint8_t sprite_index, int16_t x, int16_t y) {
    // Calculate sprite position in spritemap (no bounds checking)
    uint16_t sprite_x = (sprite_index % spritemap->sprites_per_row) * spritemap->sprite_width;
    uint16_t sprite_y = (sprite_index / spritemap->sprites_per_row) * spritemap->sprite_height;
    
    // Direct pointer arithmetic - no safety checks
    const uint16_t *sprite_data = &spritemap->data[sprite_y * spritemap->sprite_width * spritemap->sprites_per_row + sprite_x];
    uint16_t *dest_row = &self->buffer[y * self->width + x];
    
    // Simple memcpy loop - maximum performance
    uint16_t sprite_stride = spritemap->sprite_width * spritemap->sprites_per_row;
    uint16_t copy_bytes = spritemap->sprite_width * sizeof(uint16_t);
    
    for (uint16_t row = 0; row < spritemap->sprite_height; row++) {
        memcpy(dest_row, sprite_data, copy_bytes);
        dest_row += self->width;
        sprite_data += sprite_stride;
    }
}

// Ultra-fast sprite rendering with transparency (still very fast)
STATIC void sprite_fast_transparent(pixelcore_PixelCore_obj_t *self, const pixelcore_spritemap_t *spritemap,
                                    uint8_t sprite_index, int16_t x, int16_t y) {
    // Calculate sprite position in spritemap (no bounds checking)
    uint16_t sprite_x = (sprite_index % spritemap->sprites_per_row) * spritemap->sprite_width;
    uint16_t sprite_y = (sprite_index / spritemap->sprites_per_row) * spritemap->sprite_height;
    
    // Direct pointer arithmetic - no safety checks
    const uint16_t *sprite_data = &spritemap->data[sprite_y * spritemap->sprite_width * spritemap->sprites_per_row + sprite_x];
    uint16_t *dest_row = &self->buffer[y * self->width + x];
    
    uint16_t sprite_stride = spritemap->sprite_width * spritemap->sprites_per_row;
    uint16_t sprite_width = spritemap->sprite_width;
    uint16_t sprite_height = spritemap->sprite_height;
    
    // Optimized transparency processing - 8 pixels at a time when possible
    for (uint16_t row = 0; row < sprite_height; row++) {
        uint16_t col = 0;
        
        // Process 8 pixels at a time for maximum throughput
        for (; col < (sprite_width & ~7); col += 8) {
            uint16_t p0 = sprite_data[col];     if (p0) dest_row[col] = p0;
            uint16_t p1 = sprite_data[col + 1]; if (p1) dest_row[col + 1] = p1;
            uint16_t p2 = sprite_data[col + 2]; if (p2) dest_row[col + 2] = p2;
            uint16_t p3 = sprite_data[col + 3]; if (p3) dest_row[col + 3] = p3;
            uint16_t p4 = sprite_data[col + 4]; if (p4) dest_row[col + 4] = p4;
            uint16_t p5 = sprite_data[col + 5]; if (p5) dest_row[col + 5] = p5;
            uint16_t p6 = sprite_data[col + 6]; if (p6) dest_row[col + 6] = p6;
            uint16_t p7 = sprite_data[col + 7]; if (p7) dest_row[col + 7] = p7;
        }
        
        // Handle remaining pixels
        for (; col < sprite_width; col++) {
            uint16_t pixel = sprite_data[col];
            if (pixel) dest_row[col] = pixel;
        }
        
        dest_row += self->width;
        sprite_data += sprite_stride;
    }
}

// Public fast sprite function - chooses appropriate fast path
void common_hal_pixelcore_PixelCore_sprite_fast(pixelcore_PixelCore_obj_t *self, const pixelcore_spritemap_t *spritemap,
                                               uint8_t sprite_index, int16_t x, int16_t y, bool use_transparency) {
    // WARNING: This function performs NO safety checks!
    // Caller must ensure:
    // - All pointers are valid
    // - Sprite is fully within display bounds
    // - sprite_index is valid
    
    if (use_transparency) {
        sprite_fast_transparent(self, spritemap, sprite_index, x, y);
    } else {
        sprite_fast_opaque(self, spritemap, sprite_index, x, y);
    }
}


// Helper function implementations
STATIC void write_command(pixelcore_PixelCore_obj_t *self, uint8_t cmd) {
    common_hal_digitalio_digitalinout_set_value(self->dc, false);  // Command mode
    common_hal_digitalio_digitalinout_set_value(self->cs, false);  // Select display
    common_hal_busio_spi_write(self->spi, &cmd, 1);
    common_hal_digitalio_digitalinout_set_value(self->cs, true);   // Deselect display
}

STATIC void write_data(pixelcore_PixelCore_obj_t *self, const uint8_t *data, size_t len) {
    common_hal_digitalio_digitalinout_set_value(self->dc, true);   // Data mode
    common_hal_digitalio_digitalinout_set_value(self->cs, false);  // Select display
    common_hal_busio_spi_write(self->spi, data, len);
    common_hal_digitalio_digitalinout_set_value(self->cs, true);   // Deselect display
}

STATIC void write_data_byte(pixelcore_PixelCore_obj_t *self, uint8_t data) {
    write_data(self, &data, 1);
}

STATIC void set_window(pixelcore_PixelCore_obj_t *self, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Adjust coordinates to account for display boundaries
    x0++;
    x1++;
    y0=y0+2;
    y1=y1+2;

    write_command(self, ST7789_CASET);
    uint8_t caset[4] = {
        (x0 >> 8) & 0xFF,
        x0 & 0xFF,
        (x1 >> 8) & 0xFF,
        x1 & 0xFF
    };
    write_data(self, caset, 4);

    write_command(self, ST7789_RASET);
    uint8_t raset[4] = {
        (y0 >> 8) & 0xFF,
        y0 & 0xFF,
        (y1 >> 8) & 0xFF,
        y1 & 0xFF
    };
    write_data(self, raset, 4);
}

STATIC void init_display(pixelcore_PixelCore_obj_t *self) {
    // Hardware reset
    common_hal_digitalio_digitalinout_set_value(self->rst, true);
    mp_hal_delay_ms(10);
    common_hal_digitalio_digitalinout_set_value(self->rst, false);
    mp_hal_delay_ms(10);
    common_hal_digitalio_digitalinout_set_value(self->rst, true);
    mp_hal_delay_ms(100);

    // Software reset
    write_command(self, ST7789_SWRESET);
    mp_hal_delay_ms(150);

    // Sleep out
    write_command(self, ST7789_SLPOUT);
    mp_hal_delay_ms(255);

    // Display Inversion On
    write_command(self, ST7789_INVCTR);
    write_data_byte(self, 0x07); 
    
    // Memory Data Access Control
    write_command(self, ST7789_MADCTL);
    write_data_byte(self, 0x60);  // MX=1, MY=1, RGB mode
    
    // Interface Pixel Format
    write_command(self, ST7789_COLMOD);
    write_data_byte(self, 0x05);  // 16-bit color
    
    // Normal Display Mode On
    write_command(self, ST7789_NORON);
    mp_hal_delay_ms(10);
    
    // Display On
    write_command(self, ST7789_DISPON);
    mp_hal_delay_ms(100);
}

// Function to initialize ESP32-specific hardware
void esp32_pixelcore_init(pixelcore_PixelCore_obj_t *self) {
    // Note: ESP_LOG output not visible in CircuitPython REPL
    // SPI2_HOST preference should avoid DAC conflicts automatically
    
    // ESP32-S2 has limited DMA-capable internal RAM (~320KB total, but most is used by system)
    // A 40KB buffer is significant
    self->buffer = NULL;
    self->use_direct_dma = false;
    
    // Try multiple DMA allocation strategies for ESP32-S2
    self->buffer = NULL;
    
    // Strategy 1: Try 32-byte aligned DMA allocation
    self->buffer = heap_caps_aligned_alloc(32, BUFFER_SIZE, MALLOC_CAP_DMA);
    if (self->buffer && (uintptr_t)self->buffer >= 0x3FFB0000 && (uintptr_t)self->buffer < 0x40000000) {
        self->use_direct_dma = true;
    } else {
        if (self->buffer) {
            heap_caps_free(self->buffer);
            self->buffer = NULL;
        }
        
        // Strategy 2: Try regular DMA allocation
        self->buffer = heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_DMA);
        if (self->buffer && (uintptr_t)self->buffer >= 0x3FFB0000 && (uintptr_t)self->buffer < 0x40000000) {
            self->use_direct_dma = true;
        } else {
            if (self->buffer) {
                heap_caps_free(self->buffer);
                self->buffer = NULL;
            }
            
            // Strategy 3: Fall back to internal memory
            self->buffer = heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_INTERNAL);
            if (self->buffer) {
                self->use_direct_dma = false;
            } else {
                mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Failed to allocate display buffer"));
            }
        }
    }
    
    // Initialize to safe state
    self->spi_device = NULL;

    // Clear buffer
    memset(self->buffer, 0, BUFFER_SIZE);
    

}

// Function to deinitialize ESP32-specific hardware
void esp32_pixelcore_deinit(pixelcore_PixelCore_obj_t *self) {
    // Clean up our direct SPI device if we created one (currently we don't)
    if (self->spi_device != NULL) {
        spi_bus_remove_device(self->spi_device);
        self->spi_device = NULL;
    }
    
    // Free frame buffer - heap_caps_free works for all heap_caps_malloc allocations
    if (self->buffer != NULL) {
        heap_caps_free(self->buffer);
        self->buffer = NULL;
    }
    
    self->use_direct_dma = false;
}



// Common HAL functions
void common_hal_pixelcore_PixelCore_construct(pixelcore_PixelCore_obj_t *self,
    busio_spi_obj_t *spi,
    digitalio_digitalinout_obj_t *cs,
    digitalio_digitalinout_obj_t *dc,
    digitalio_digitalinout_obj_t *rst) {
    
    self->spi = spi;
    self->cs = cs;
    self->dc = dc;
    self->rst = rst;
    self->width = DISPLAY_WIDTH;
    self->height = DISPLAY_HEIGHT;
    self->spi_freq = 80000000; // 80MHz for better performance
    self->buffer = NULL;  // Will be allocated in esp32_pixelcore_init
    self->use_direct_dma = false;  // Will be enabled if DMA buffer allocation succeeds
    self->spi_device = NULL;
    
    // Store SPI configuration NOW while SPI object is still valid
    // This information will be used later if Python calls spi.deinit()
    self->stored_host_id = spi->host_id;
    self->stored_mosi_pin = common_hal_mcu_pin_number(spi->MOSI);
    self->stored_clk_pin = common_hal_mcu_pin_number(spi->clock);
    
    // Note: ESP_LOG statements are not visible in CircuitPython REPL
    // Performance testing will show if SPI2_HOST fix is working

    // Initialize pins with explicit configuration
    
    // Configure CS pin
    common_hal_digitalio_digitalinout_switch_to_output(self->cs, true, DRIVE_MODE_PUSH_PULL);
    common_hal_digitalio_digitalinout_set_value(self->cs, true); // Deselect display
    
    // Configure DC pin
    common_hal_digitalio_digitalinout_switch_to_output(self->dc, false, DRIVE_MODE_PUSH_PULL);
    
    // Configure RST pin
    common_hal_digitalio_digitalinout_switch_to_output(self->rst, true, DRIVE_MODE_PUSH_PULL);

    // Initialize ESP32-specific hardware
    esp32_pixelcore_init(self);

    // Initialize display
    init_display(self);

    // Clear display
    common_hal_pixelcore_PixelCore_fill(self, 0xFFFF, 0, self->height - 1); // Fill with white to test
    common_hal_pixelcore_PixelCore_update(self);
}

void common_hal_pixelcore_PixelCore_deinit(pixelcore_PixelCore_obj_t *self) {
    esp32_pixelcore_deinit(self);
}

void common_hal_pixelcore_PixelCore_update(pixelcore_PixelCore_obj_t *self) {
    if (self == NULL || self->buffer == NULL) {
        return;
    }
    
    // Set window and send buffer data - always use simple SPI path
    set_window(self, 0, 0, self->width - 1, self->height - 1);
    write_command(self, ST7789_RAMWR);
    write_data(self, (uint8_t *)self->buffer, BUFFER_SIZE);
}





void common_hal_pixelcore_PixelCore_fill(pixelcore_PixelCore_obj_t *self, uint16_t color, uint16_t start_y, uint16_t end_y) {
    if (self->buffer == NULL) {
        return;
    }

    // Clamp start_y and end_y to valid range
    if (start_y >= self->height) return;
    if (end_y >= self->height) end_y = self->height - 1;
    if (start_y > end_y) return;

    // Calculate row range
    uint16_t *start_row = &self->buffer[start_y * self->width];
    uint16_t *end_row = &self->buffer[(end_y + 1) * self->width];
    size_t row_size = self->width * sizeof(uint16_t);

    // Use memset for solid colors if possible
    if (color == 0 || color == 0xFFFF) {
        uint8_t fill_byte = color == 0 ? 0 : 0xFF;
        for (uint16_t *row = start_row; row < end_row; row += self->width) {
            memset(row, fill_byte, row_size);
        }
        return;
    }

    // Otherwise fill with 16-bit color value
    for (uint16_t *row = start_row; row < end_row; row += self->width) {
        uint16_t *buf = row;
        for (uint16_t x = 0; x < self->width; x++) {
            *buf++ = color;
        }
    }
}

void common_hal_pixelcore_PixelCore_pixel(pixelcore_PixelCore_obj_t *self, int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
        return;
    }
    self->buffer[y * DISPLAY_WIDTH + x] = color;
}

void common_hal_pixelcore_PixelCore_hline(pixelcore_PixelCore_obj_t *self, int16_t x, int16_t y, uint16_t width, uint16_t color) {
    if (y < 0 || y >= DISPLAY_HEIGHT) {
        return;
    }
    for (int16_t i = 0; i < width; i++) {
        if (x + i >= 0 && x + i < DISPLAY_WIDTH) {
            self->buffer[y * DISPLAY_WIDTH + x + i] = color;
        }
    }
}

void common_hal_pixelcore_PixelCore_vline(pixelcore_PixelCore_obj_t *self, int16_t x, int16_t y, uint16_t height, uint16_t color) {
    if (x < 0 || x >= DISPLAY_WIDTH) {
        return;
    }
    for (int16_t i = 0; i < height; i++) {
        if (y + i >= 0 && y + i < DISPLAY_HEIGHT) {
            self->buffer[(y + i) * DISPLAY_WIDTH + x] = color;
        }
    }
}

void common_hal_pixelcore_PixelCore_rect(pixelcore_PixelCore_obj_t *self, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color) {
    common_hal_pixelcore_PixelCore_hline(self, x, y, width, color);
    if (height > 1) {
        common_hal_pixelcore_PixelCore_hline(self, x, y + height - 1, width, color);
        common_hal_pixelcore_PixelCore_vline(self, x, y + 1, height - 2, color);
        common_hal_pixelcore_PixelCore_vline(self, x + width - 1, y + 1, height - 2, color);
    }
}

void common_hal_pixelcore_PixelCore_box(pixelcore_PixelCore_obj_t *self, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color) {
    // Clip rectangle to display bounds
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (x + width > self->width) {
        width = self->width - x;
    }
    if (y + height > self->height) {
        height = self->height - y;
    }
    if (width <= 0 || height <= 0) {
        return;
    }
    
    // Direct memory write with stride
    uint16_t stride = self->width;
    uint16_t *row = &self->buffer[y * stride + x];
    
    // Optimize for small rectangles
    if (width < 8) {
        while (height--) {
            uint16_t *buf = row;
            uint16_t w = width;
            while (w--) {
                *buf++ = color;
            }
            row += stride;
        }
        return;
    }
    
    // For larger rectangles, fill first row and use memcpy
    uint16_t *buf = row;
    uint16_t w = width;
    while (w--) {
        *buf++ = color;
    }
    
    // Copy first row to remaining rows
    while (--height) {
        row += stride;
        memcpy(row, row - stride, width * sizeof(uint16_t));
    }
}

// Sprite functions
void common_hal_pixelcore_PixelCore_sprite(pixelcore_PixelCore_obj_t *self, const pixelcore_spritemap_t *spritemap,
                                  uint8_t sprite_index, int16_t x, int16_t y, bool use_transparency, bool x_flip) {
    if (self->buffer == NULL || spritemap == NULL || spritemap->data == NULL) {
        return;
    }

    // Check if sprite is fully within bounds - if so, use fast path
    if (x >= 0 && y >= 0 && 
        x + spritemap->sprite_width <= self->width && 
        y + spritemap->sprite_height <= self->height) {
        // Use fast path - no bounds checking needed
        common_hal_pixelcore_PixelCore_sprite_fast(self, spritemap, sprite_index, x, y, use_transparency);
        return;
    }

    // Slow path: Handle clipping for sprites at edges
    uint16_t sprite_x = (sprite_index % spritemap->sprites_per_row) * spritemap->sprite_width;
    uint16_t sprite_y = (sprite_index / spritemap->sprites_per_row) * spritemap->sprite_height;
    const uint16_t *sprite_data = &spritemap->data[sprite_y * spritemap->sprite_width * spritemap->sprites_per_row + sprite_x];

    int16_t draw_width = spritemap->sprite_width;
    int16_t draw_height = spritemap->sprite_height;
    
    if (x < 0) { draw_width += x; sprite_data -= x; x = 0; }
    if (y < 0) { draw_height += y; sprite_data -= y * spritemap->sprite_width * spritemap->sprites_per_row; y = 0; }
    if (x + draw_width > self->width) draw_width = self->width - x;
    if (y + draw_height > self->height) draw_height = self->height - y;
    if (draw_width <= 0 || draw_height <= 0) return;

    uint16_t *dest_row = &self->buffer[y * self->width + x];
    const uint16_t *src_row = sprite_data;
    uint16_t sprite_stride = spritemap->sprite_width * spritemap->sprites_per_row;
    
    if (!use_transparency) {
        if (x_flip) {
            // Horizontal flip: copy pixels in reverse order
            for (int16_t row = 0; row < draw_height; row++) {
                for (int16_t col = 0; col < draw_width; col++) {
                    dest_row[col] = src_row[draw_width - 1 - col];
                }
                dest_row += self->width;
                src_row += sprite_stride;
            }
        } else {
            for (int16_t row = 0; row < draw_height; row++) {
                memcpy(dest_row, src_row, draw_width * sizeof(uint16_t));
                dest_row += self->width; src_row += sprite_stride;
            }
        }
    } else {
        if (x_flip) {
            // Horizontal flip with transparency
            for (int16_t row = 0; row < draw_height; row++) {
                for (int16_t col = 0; col < draw_width; col++) {
                    uint16_t pixel = src_row[draw_width - 1 - col];
                    if (pixel) dest_row[col] = pixel;
                }
                dest_row += self->width;
                src_row += sprite_stride;
            }
        } else {
            for (int16_t row = 0; row < draw_height; row++) {
                for (int16_t col = 0; col < draw_width; col++) {
                    uint16_t pixel = src_row[col];
                    if (pixel) dest_row[col] = pixel;
                }
                dest_row += self->width; src_row += sprite_stride;
            }
        }
    }
}

void common_hal_pixelcore_PixelCore_clear(pixelcore_PixelCore_obj_t *self) {
    if (self->buffer == NULL) {
        return;
    }
    
    // Use memset to quickly set all bytes to 0
    memset(self->buffer, 0, BUFFER_SIZE);
}

void common_hal_pixelcore_PixelCore_line(pixelcore_PixelCore_obj_t *self, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    // Bresenham's line algorithm
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = (dx > dy ? dx : -dy) / 2;
    int16_t e2;

    while (true) {
        common_hal_pixelcore_PixelCore_pixel(self, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

void common_hal_pixelcore_PixelCore_circle(pixelcore_PixelCore_obj_t *self, int16_t x0, int16_t y0, int16_t radius, uint16_t color) {
    // Bresenham's circle algorithm
    int16_t x = radius;
    int16_t y = 0;
    int16_t err = 0;

    while (x >= y) {
        common_hal_pixelcore_PixelCore_pixel(self, x0 + x, y0 + y, color);
        common_hal_pixelcore_PixelCore_pixel(self, x0 + y, y0 + x, color);
        common_hal_pixelcore_PixelCore_pixel(self, x0 - y, y0 + x, color);
        common_hal_pixelcore_PixelCore_pixel(self, x0 - x, y0 + y, color);
        common_hal_pixelcore_PixelCore_pixel(self, x0 - x, y0 - y, color);
        common_hal_pixelcore_PixelCore_pixel(self, x0 - y, y0 - x, color);
        common_hal_pixelcore_PixelCore_pixel(self, x0 + y, y0 - x, color);
        common_hal_pixelcore_PixelCore_pixel(self, x0 + x, y0 - y, color);

        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

// Ultra-fast tilemap rendering function
void common_hal_pixelcore_PixelCore_tilemap(pixelcore_PixelCore_obj_t *self, 
                                            const pixelcore_spritemap_t *spritemap,
                                            const uint8_t *tilemap_data,
                                            uint16_t tilemap_width, uint16_t tilemap_height,
                                            uint16_t tile_width, uint16_t tile_height,
                                            int16_t render_x, int16_t render_y,
                                            bool use_transparency) {
    
    // Early exit if invalid parameters
    if (!self->buffer || !spritemap || !spritemap->data || !tilemap_data) {
        return;
    }
    
    // Calculate tile pixel dimensions
    uint16_t tile_pixel_width = tile_width * spritemap->sprite_width;
    uint16_t tile_pixel_height = tile_height * spritemap->sprite_height;
    
    // Calculate visible tile range with culling
    int16_t start_tile_x = 0;
    int16_t start_tile_y = 0;
    int16_t end_tile_x = tilemap_width;
    int16_t end_tile_y = tilemap_height;
    
    // Cull tiles outside display bounds (critical optimization!)
    if (render_x < 0) {
        start_tile_x = (-render_x) / tile_pixel_width;
        if (start_tile_x >= tilemap_width) return;
    }
    if (render_y < 0) {
        start_tile_y = (-render_y) / tile_pixel_height;
        if (start_tile_y >= tilemap_height) return;
    }
    if (render_x >= self->width) return;
    if (render_y >= self->height) return;
    
    // Limit end tiles to display bounds
    int16_t max_x = self->width - render_x;
    int16_t max_y = self->height - render_y;
    end_tile_x = (max_x + tile_pixel_width - 1) / tile_pixel_width;
    end_tile_y = (max_y + tile_pixel_height - 1) / tile_pixel_height;
    
    if (end_tile_x > tilemap_width) end_tile_x = tilemap_width;
    if (end_tile_y > tilemap_height) end_tile_y = tilemap_height;
    
    // Pre-calculate sprite dimensions and stride for performance
    uint16_t sprite_width = spritemap->sprite_width;
    uint16_t sprite_height = spritemap->sprite_height;
    uint16_t sprite_stride = sprite_width * spritemap->sprites_per_row;
    uint16_t display_width = self->width;
    
    // Main tile rendering loop - optimized for cache efficiency
    for (int16_t tile_y = start_tile_y; tile_y < end_tile_y; tile_y++) {
        for (int16_t tile_x = start_tile_x; tile_x < end_tile_x; tile_x++) {
            
            // Get tile index from tilemap (8-bit lookup)
            uint8_t tile_index = tilemap_data[tile_y * tilemap_width + tile_x];
            
            // Skip empty tiles (index 0 = transparent tile)
            if (tile_index == 0 && use_transparency) {
                continue;
            }
            
            // Calculate tile render position
            int16_t tile_render_x = render_x + tile_x * tile_pixel_width;
            int16_t tile_render_y = render_y + tile_y * tile_pixel_height;
            
            // Render each sprite within this tile
            for (uint16_t sprite_y = 0; sprite_y < tile_height; sprite_y++) {
                for (uint16_t sprite_x = 0; sprite_x < tile_width; sprite_x++) {
                    
                    // Calculate final sprite position
                    int16_t final_x = tile_render_x + sprite_x * sprite_width;
                    int16_t final_y = tile_render_y + sprite_y * sprite_height;
                    
                    // Bounds check for this sprite
                    if (final_x >= self->width || final_y >= self->height ||
                        final_x + sprite_width <= 0 || final_y + sprite_height <= 0) {
                        continue;
                    }
                    
                    // Calculate sprite position in spritemap
                    uint16_t sprite_map_x = (tile_index % spritemap->sprites_per_row) * sprite_width;
                    uint16_t sprite_map_y = (tile_index / spritemap->sprites_per_row) * sprite_height;
                    
                    // Get source sprite data pointer
                    const uint16_t *sprite_data = &spritemap->data[sprite_map_y * sprite_stride + sprite_map_x];
                    
                    // Calculate clipping for this sprite
                    int16_t clip_left = (final_x < 0) ? -final_x : 0;
                    int16_t clip_top = (final_y < 0) ? -final_y : 0;
                    int16_t clip_right = (final_x + sprite_width > self->width) ? 
                                        (final_x + sprite_width - self->width) : 0;
                    int16_t clip_bottom = (final_y + sprite_height > self->height) ? 
                                         (final_y + sprite_height - self->height) : 0;
                    
                    int16_t draw_width = sprite_width - clip_left - clip_right;
                    int16_t draw_height = sprite_height - clip_top - clip_bottom;
                    
                    if (draw_width <= 0 || draw_height <= 0) continue;
                    
                    // Adjust pointers for clipping
                    const uint16_t *src_row = sprite_data + clip_top * sprite_stride + clip_left;
                    uint16_t *dest_row = &self->buffer[(final_y + clip_top) * display_width + (final_x + clip_left)];
                    
                    // Ultra-fast sprite copy
                    if (!use_transparency) {
                        // Fastest path: pure memcpy
                        for (int16_t row = 0; row < draw_height; row++) {
                            memcpy(dest_row, src_row, draw_width * sizeof(uint16_t));
                            dest_row += display_width;
                            src_row += sprite_stride;
                        }
                    } else {
                        // Fast transparency path: 8 pixels at a time
                        for (int16_t row = 0; row < draw_height; row++) {
                            int16_t col = 0;
                            
                            // Process 8 pixels at once for maximum throughput
                            for (; col < (draw_width & ~7); col += 8) {
                                uint16_t p0 = src_row[col];     if (p0) dest_row[col] = p0;
                                uint16_t p1 = src_row[col + 1]; if (p1) dest_row[col + 1] = p1;
                                uint16_t p2 = src_row[col + 2]; if (p2) dest_row[col + 2] = p2;
                                uint16_t p3 = src_row[col + 3]; if (p3) dest_row[col + 3] = p3;
                                uint16_t p4 = src_row[col + 4]; if (p4) dest_row[col + 4] = p4;
                                uint16_t p5 = src_row[col + 5]; if (p5) dest_row[col + 5] = p5;
                                uint16_t p6 = src_row[col + 6]; if (p6) dest_row[col + 6] = p6;
                                uint16_t p7 = src_row[col + 7]; if (p7) dest_row[col + 7] = p7;
                            }
                            
                            // Handle remaining pixels
                            for (; col < draw_width; col++) {
                                uint16_t pixel = src_row[col];
                                if (pixel) dest_row[col] = pixel;
                            }
                            
                            dest_row += display_width;
                            src_row += sprite_stride;
                        }
                    }
                }
            }
        }
    }
}



bool common_hal_pixelcore_PixelCore_deinited(pixelcore_PixelCore_obj_t *self) {
    return self->deinited;
}

// Row fill function - fills multiple rows with different colors
void common_hal_pixelcore_PixelCore_row_fill(pixelcore_PixelCore_obj_t *self, const uint16_t *colors, size_t color_count, uint16_t start_row, uint16_t row_count) {
    if (self->buffer == NULL || colors == NULL || color_count == 0) {
        return;
    }
    
    // Clamp parameters to valid range
    if (start_row >= self->height) return;
    if (start_row + row_count > self->height) {
        row_count = self->height - start_row;
    }
    if (row_count == 0) return;
    
    // Fill rows with colors
    uint16_t *row_ptr = &self->buffer[start_row * self->width];
    for (uint16_t row = 0; row < row_count; row++) {
        uint16_t color = colors[row % color_count]; // Cycle through colors if needed
        for (uint16_t x = 0; x < self->width; x++) {
            row_ptr[x] = color;
        }
        row_ptr += self->width;
    }
}

// Reset function for CircuitPython soft reset
void pixelcore_reset(void) {
    // No global instance tracking - individual objects handle their own cleanup
    // during deinit() calls from Python garbage collection
}

