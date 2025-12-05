#if !defined(KEYBOARD_H)
#define KEYBOARD_H

#include <Arduino.h>
#include <queue>
#include <pico/stdlib.h>
#include <pico/platform.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>

class KeyBoard
{
protected:
    uint8_t _board;
public:
    KeyBoard() : _board(0) {}
    virtual ~KeyBoard() {}

    virtual bool wait_any_key() = 0;
    virtual bool fetch_key(uint8_t &c) = 0;
    virtual bool exists() = 0;

    static const int I2C_ADDR = 0x08;
    enum kbd_type_t { unknown = -1, faces = 0, cardputer = 1, ble = 2, picocalc = 3, };

    virtual inline kbd_type_t keyboard_type() = 0;
};

#define I2C_KBD_MOD i2c1
#define I2C_KBD_SDA 6
#define I2C_KBD_SCL 7

#define I2C_KBD_SPEED  50000 // if dual i2c, then the speed of keyboard i2c should be 10khz

#define I2C_KBD_ADDR 0x1F

class PicoCalcKeyBoard : public KeyBoard
{
public:
    PicoCalcKeyBoard();
    ~PicoCalcKeyBoard() {}

    bool wait_any_key() override;
    bool fetch_key(uint8_t &c) override;
    bool exists() override { return true; }

    inline kbd_type_t keyboard_type() override { return picocalc; }
protected:
    int _write_i2c_kbd(); 
    int _read_i2c_kbd(); 
    uint8_t _i2c_inited;
    uint8_t _keycheck;
};
#endif