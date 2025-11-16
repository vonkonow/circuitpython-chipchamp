#ifndef MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_PIXELCORE_PIXELCORE_H
#define MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_PIXELCORE_PIXELCORE_H

#include "py/obj.h"
#include "ports/espressif/common-hal/pixelcore/pixelcore_types.h"

// ESP32-specific functions
void esp32_pixelcore_init(pixelcore_PixelCore_obj_t *self);
void esp32_pixelcore_deinit(pixelcore_PixelCore_obj_t *self);
void esp32_pixelcore_write_data(pixelcore_PixelCore_obj_t *self, const uint8_t *data, size_t len);

// Ultra-fast sprite functions - no safety checks, maximum performance
void common_hal_pixelcore_PixelCore_sprite_fast(pixelcore_PixelCore_obj_t *self, 
                                               const pixelcore_spritemap_t *spritemap,
                                               uint8_t sprite_index, int16_t x, int16_t y, 
                                               bool use_transparency);

// Ultra-fast tilemap rendering - maximum performance with automatic culling
void common_hal_pixelcore_PixelCore_tilemap(pixelcore_PixelCore_obj_t *self, 
                                            const pixelcore_spritemap_t *spritemap,
                                            const uint8_t *tilemap_data,
                                            uint16_t tilemap_width, uint16_t tilemap_height,
                                            uint16_t tile_width, uint16_t tile_height,
                                            int16_t render_x, int16_t render_y,
                                            bool use_transparency);




// Common HAL functions
void common_hal_pixelcore_PixelCore_construct(pixelcore_PixelCore_obj_t *self,
    busio_spi_obj_t *spi,
    digitalio_digitalinout_obj_t *cs,
    digitalio_digitalinout_obj_t *dc,
    digitalio_digitalinout_obj_t *rst);

void common_hal_pixelcore_PixelCore_deinit(pixelcore_PixelCore_obj_t *self);
bool common_hal_pixelcore_PixelCore_deinited(pixelcore_PixelCore_obj_t *self);
void common_hal_pixelcore_PixelCore_update(pixelcore_PixelCore_obj_t *self);

void common_hal_pixelcore_PixelCore_fill(pixelcore_PixelCore_obj_t *self, uint16_t color, uint16_t start_y, uint16_t end_y);
void common_hal_pixelcore_PixelCore_pixel(pixelcore_PixelCore_obj_t *self, int16_t x, int16_t y, uint16_t color);
void common_hal_pixelcore_PixelCore_hline(pixelcore_PixelCore_obj_t *self, int16_t x, int16_t y, uint16_t width, uint16_t color);
void common_hal_pixelcore_PixelCore_vline(pixelcore_PixelCore_obj_t *self, int16_t x, int16_t y, uint16_t height, uint16_t color);
void common_hal_pixelcore_PixelCore_rect(pixelcore_PixelCore_obj_t *self, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color);
void common_hal_pixelcore_PixelCore_box(pixelcore_PixelCore_obj_t *self, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color);
void common_hal_pixelcore_PixelCore_clear(pixelcore_PixelCore_obj_t *self);
void common_hal_pixelcore_PixelCore_sprite(pixelcore_PixelCore_obj_t *self, const pixelcore_spritemap_t *spritemap, uint8_t sprite_index, int16_t x, int16_t y, bool use_transparency, bool x_flip);
void common_hal_pixelcore_PixelCore_row_fill(pixelcore_PixelCore_obj_t *self, const uint16_t *colors, size_t color_count, uint16_t start_row, uint16_t row_count);
void common_hal_pixelcore_PixelCore_line(pixelcore_PixelCore_obj_t *self, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void common_hal_pixelcore_PixelCore_circle(pixelcore_PixelCore_obj_t *self, int16_t x0, int16_t y0, int16_t radius, uint16_t color);

// Reset function for CircuitPython soft reset
void pixelcore_reset(void);

#endif // MICROPY_INCLUDED_ESPRESSIF_COMMON_HAL_PIXELCORE_PIXELCORE_H 