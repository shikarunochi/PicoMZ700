#ifndef MZ700LGX_H_
#define MZ700LGX_H_

#include <LovyanGFX.hpp> // Include LovyanGFX library

#define EXT_SPI_SCLK 10
#define EXT_SPI_MOSI 11
#define EXT_SPI_MISO 12
#define EXT_SPI_CS 13
#define EXT_SPI_DC 14
#define EXT_SPI_RST 15

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9488 _panel_instance; // ILI9488 panel
    lgfx::Bus_SPI _bus_instance; // SPI bus instance
  public:
    LGFX(void)
    {
      { // バス制御の設定を行います。
        auto cfg = _bus_instance.config();
        cfg.spi_host = 1; // SPI1 for pico/pico2
        cfg.spi_mode = 0; // SPI mode 0
        cfg.freq_write = 50000000; // SPI write frequency
        cfg.freq_read = 25000000; // SPI read frequency

        cfg.pin_sclk = EXT_SPI_SCLK; // SCLK pin
        cfg.pin_mosi = EXT_SPI_MOSI; // MOSI pin
        cfg.pin_miso = EXT_SPI_MISO; // MISO pin
        cfg.pin_dc   = EXT_SPI_DC; // DC pin

        _bus_instance.config(cfg); // Apply bus configuration
        _panel_instance.setBus(&_bus_instance); // Set bus to panel instance
      }

      { // 表示パネル制御の設定を行います。
        auto cfg = _panel_instance.config();
        cfg.pin_cs = EXT_SPI_CS; // CS pin
        cfg.pin_rst = EXT_SPI_RST; // RST pin
        cfg.pin_busy = -1; // No busy pin

        cfg.memory_width = 320; // Screen width
        cfg.memory_height = 480; // Screen height
        cfg.panel_width = 320; // Panel width
        cfg.panel_height = 320; // Panel height

        cfg.offset_x = 0; // X offset
        cfg.offset_y = 0; // Y offset
        cfg.offset_rotation = 6; // Rotation offset

        cfg.dummy_read_pixel = 8; // Dummy read pixel
        cfg.dummy_read_bits = 1; // Dummy read bits

        cfg.readable = true; // Enable readable mode

        cfg.invert = true; // inversion
        cfg.rgb_order = false; // RGB order

        // cfg.bus_shared = true; // Bus shared

        _panel_instance.config(cfg); // Apply panel configuration
      }
      setPanel(&_panel_instance); // 使用するパネルをセットします。
    }
};
extern LGFX m5lcd;

extern int lcdMode;
extern int lcdRotate;
#endif 
