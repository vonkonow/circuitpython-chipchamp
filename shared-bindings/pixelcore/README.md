# PixelCore Display Driver

The `pixelcore` module provides a high-performance display driver optimized for ESP32-S2 microcontrollers. It is specifically designed for ST7789-based displays and offers hardware-accelerated graphics operations through the ESP32-S2's SPI interface.

## Hardware Requirements

- ESP32-S2 microcontroller
- ST7789-based display (160x128 resolution supported)
- SPI interface with the following pins:
  - MOSI (Data out)
  - SCK (Clock)
  - CS (Chip Select)
  - DC (Data/Command)
  - RST (Reset)

## Basic Usage

```python
import board
import busio
import digitalio
from pixelcore import PixelCore

# Initialize SPI
spi = busio.SPI(clock=board.IO36, MOSI=board.IO35)

# Initialize control pins
cs = digitalio.DigitalInOut(board.IO5)
dc = digitalio.DigitalInOut(board.IO4)
rst = digitalio.DigitalInOut(board.IO3)

# Create display instance
display = PixelCore(spi, cs, dc, rst)

# Basic drawing operations
display.fill(0x0000)  # Clear display to black
display.pixel(80, 64, 0xFFFF)  # Draw a white pixel at center
display.update()  # Update display
```

## Color Format

Colors are specified in RGB565 format (16-bit), but with bytes swapped for ST7789 compatibility:
- First byte (LSB):
  - Green (lower 3 bits): bits 0-2
  - Blue (5 bits): bits 3-7
- Second byte (MSB):
  - Red (5 bits): bits 3-7
  - Green (upper 3 bits): bits 0-2

To create a color value:
```python
def rgb565(r, g, b):
    """Convert RGB888 to RGB565 with correct byte order.
    
    Args:
        r (int): Red value (0-255)
        g (int): Green value (0-255)
        b (int): Blue value (0-255)
    
    Returns:
        int: 16-bit color value with bytes swapped for ST7789
    """
    rgb = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    return ((rgb & 0xFF) << 8) | (rgb >> 8)  # Swap bytes

# Common colors (pre-swapped)
BLACK = 0x0000   # 0b0000000000000000
WHITE = 0xFFFF   # 0b1111111111111111
RED = 0x00F8     # 0b0000000011111000
GREEN = 0xE007   # 0b1110000000000111
BLUE = 0x1F00    # 0b0001111100000000
YELLOW = 0xE0FF  # 0b1110000011111111
MAGENTA = 0x1FF8 # 0b0001111111111000
CYAN = 0xFF07    # 0b1111111100000111
```

## Drawing Functions

### Basic Display Operations

- `clear() -> None`
  - Clear display (fill with black)

- `update() -> None`
  - Update display with frame buffer contents
  - Must be called after drawing operations

- `fill(color: int, start_y: int = 0, end_y: int = 127) -> None`
  - Fill display or a vertical range with a color
  - `color`: 16-bit RGB565 format (byte-swapped)
  - `start_y`: Starting Y coordinate (default: 0, top of display)
  - `end_y`: Ending Y coordinate (default: 127, bottom of display)
  - Perfect for sky/ground effects and background layers

- `deinit() -> None`
  - Release hardware resources

### Basic Drawing Primitives

- `pixel(x: int, y: int, color: int) -> None`
  - Draw a single pixel
  - Color is 16-bit RGB565 format (byte-swapped)
  - Includes bounds checking

- `hline(x: int, y: int, width: int, color: int) -> None`
  - Draw horizontal line
  - Starting at (x,y) with given width
  - Includes bounds checking

- `vline(x: int, y: int, height: int, color: int) -> None`
  - Draw vertical line
  - Starting at (x,y) with given height
  - Includes bounds checking

- `line(x0: int, y0: int, x1: int, y1: int, color: int) -> None`
  - Draw line between two points using Bresenham's algorithm
  - From (x0,y0) to (x1,y1)
  - Includes bounds checking

- `rect(x: int, y: int, width: int, height: int, color: int) -> None`
  - Draw rectangle outline
  - Top-left at (x,y) with given width and height
  - Includes bounds checking

- `box(x: int, y: int, width: int, height: int, color: int) -> None`
  - Draw filled rectangle
  - Top-left at (x,y) with given width and height
  - Includes bounds checking

- `circle(center_x: int, center_y: int, radius: int, color: int) -> None`
  - Draw circle outline using optimized algorithm
  - Center at (center_x, center_y) with given radius
  - Includes bounds checking

### Sprite Rendering Functions

#### Single Sprite Functions

- `sprite(spritemap_data: bytes, sprite_width: int, sprite_height: int, sprites_per_row: int, sprite_index: int, x: int, y: int, use_transparency: bool = False) -> None`
  - **Safe sprite rendering with bounds checking and transparency support**
  - `spritemap_data`: Bytes object containing RGB565 pixel data (byte-swapped)
  - `sprite_width`, `sprite_height`: Dimensions of each sprite in pixels
  - `sprites_per_row`: Number of sprites per row in the spritemap
  - `sprite_index`: Index of sprite to draw (0-based)
  - `x`, `y`: Position to draw sprite
  - `use_transparency`: Optional transparency support - color 0x0000 treated as transparent (default: False)
  - Includes bounds checking and clipping
  - Recommended for most use cases

#### Batch Sprite Functions

- `sprites(spritemap_data: bytes, sprite_width: int, sprite_height: int, sprites_per_row: int, sprite_list: list, use_transparency: bool = False) -> None`
  - **Safe batch sprite rendering with bounds checking and transparency support**
  - Same spritemap parameters as single sprite functions
  - `sprite_list`: 2D list of sprites to draw
    - Format: `[[sprite_index, x, y], [sprite_index, x, y], ...]`
    - Or with per-sprite transparency: `[[sprite_index, x, y, use_transparency], [sprite_index, x, y, use_transparency], ...]`
  - `use_transparency`: Global transparency setting (default: False) - overridden by per-sprite transparency values
  - Each sprite includes bounds checking and clipping
  - Recommended for batch sprite operations

### Properties

- `use_direct_dma: bool` (read/write)
  - **DMA memory allocation status and control**
  - **Getter**: Returns `True` if display buffer is allocated in DMA-capable memory, `False` otherwise
  - **Setter**: Allows manual control of DMA usage flag (for debugging/testing)
  - When `True`: Display updates can use ESP32-S2 DMA for maximum performance
  - When `False`: Display updates use regular memory transfers (slower)
  - Automatically set during initialization based on memory allocation success
  - Memory allocation strategy:
    1. **32-byte aligned DMA memory** (best performance)
    2. **Regular DMA memory** (good performance) 
    3. **Internal memory fallback** (basic performance, `use_direct_dma = False`)

### Advanced Tilemap Functions

- `tilemap(spritemap_data: bytes, sprite_width: int, sprite_height: int, sprites_per_row: int, tilemap_data: bytes, tilemap_width: int, tilemap_height: int, tile_width: int, tile_height: int, render_x: int, render_y: int, use_transparency: bool = False) -> None`
  - **Ultra-optimized tilemap renderer with automatic culling**
  - `spritemap_data`: Bytes object containing sprite data
  - `sprite_width`, `sprite_height`: Dimensions of each sprite
  - `sprites_per_row`: Number of sprites per row in spritemap
  - `tilemap_data`: Bytes object containing tile indices (8-bit values)
  - `tilemap_width`, `tilemap_height`: Dimensions of tilemap in tiles
  - `tile_width`, `tile_height`: Size of each tile in sprites (usually 1x1)
  - `render_x`, `render_y`: Screen position to render tilemap
  - `use_transparency`: If True, color 0x0000 is treated as transparent
  - Automatically culls off-screen tiles for maximum performance
  - Uses vectorized operations and batch processing
  - Most efficient for rendering large tilemaps

## Function Performance Hierarchy

**From Fastest to Safest:**

1. **`tilemap()`** - Fastest (culling + vectorization + specialized loops)
2. **`sprites()`** - Medium speed (bounds checking + clipping)
3. **`sprite()`** - Medium speed (bounds checking + clipping)

## Example: Basic Drawing

```python
# Import helper function for colors
def rgb565(r, g, b):
    rgb = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    return ((rgb & 0xFF) << 8) | (rgb >> 8)

# Pre-defined colors (byte-swapped)
BLACK = 0x0000
WHITE = 0xFFFF
RED = 0x00F8
GREEN = 0xE007
BLUE = 0x1F00

# Draw various shapes
display.fill(BLACK)  # Clear to black

# Draw some lines
display.line(0, 0, 159, 127, WHITE)  # Diagonal white line
display.hline(30, 64, 100, RED)      # Horizontal red line
display.vline(80, 20, 88, GREEN)     # Vertical green line

# Draw rectangles
display.rect(10, 10, 140, 108, BLUE)  # Blue rectangle outline
display.box(50, 40, 60, 48, YELLOW)   # Yellow filled rectangle

# Draw circles
display.circle(80, 64, 30, MAGENTA)   # Purple circle outline

display.update()  # Show the results
```

## Example: Partial Fill Effects

```python
# Sky and ground background
display.fill(0x87EF, 0, 64)    # Sky blue for top half (y=0 to y=64)
display.fill(0xE607, 64, 127)  # Ground brown for bottom half (y=64 to y=127)

# Horizon effects with multiple layers
display.fill(0x001F, 0, 70)    # Deep blue sky (top)
display.fill(0x0A5F, 70, 80)   # Lighter blue (horizon)
display.fill(0x07E0, 80, 85)   # Green horizon line 
display.fill(0xFFE0, 85, 127)  # Yellow ground (bottom)

display.update()
```

## Example: Sprite Usage

```python
# Create a simple 8x8 sprite (red square)
sprite_data = bytearray(8 * 8 * 2)  # 8x8 pixels, 2 bytes each
red_color = 0x00F8  # Red in byte-swapped RGB565

for i in range(0, len(sprite_data), 2):
    sprite_data[i] = red_color & 0xFF
    sprite_data[i+1] = red_color >> 8

# Draw single sprite
display.sprite(sprite_data, 8, 8, 1, 0, 50, 50)  # Safe with bounds checking

# Batch sprite drawing (safe with bounds checking)
sprite_list = [
    [0, 10, 10],    # sprite 0 at (10,10)
    [0, 20, 20],    # sprite 0 at (20,20)
    [0, 30, 30]     # sprite 0 at (30,30)
]
display.sprites(sprite_data, 8, 8, 1, sprite_list)  # Safe batch rendering

display.update()
```

## Performance Optimizations

The driver includes several optimizations:
1. **Direct memory access** for pixel operations
2. **Efficient algorithms** using Bresenham's line drawing
3. **Optimized filled shapes** using memcpy operations
4. **Hardware-accelerated SPI** transfers
5. **Automatic bounds checking** with clipping
6. **Vectorized operations** for tilemap rendering
7. **Automatic culling** of off-screen tiles
8. **Batch processing** for sprite operations
9. **Range-based filling** for efficient partial screen updates

## Best Practices

### When to Use Each Function

- **Use `sprite()`** for single sprites when you need bounds checking
- **Use `sprites()`** for batch drawing when you need safety
- **Use `tilemap()`** for large scrolling backgrounds and level rendering
- **Use `fill()` with ranges** for efficient background layers and sky/ground effects

### Memory Management

- The driver allocates a frame buffer of 40KB (160x128x2 bytes)
- Sprite data should be pre-calculated and stored efficiently
- Use `bytearray` for mutable sprite data, `bytes` for immutable data
- Consider using transparency to reduce visual complexity

### Error Prevention

- Always call `update()` after drawing operations
- Use bounds-checked functions (`sprite()`, `sprites()`) during development
- Remember that coordinates are zero-based (0,0 to 159,127)
