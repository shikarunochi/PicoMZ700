// Keyboard class

#include <Keyboard.h>

PicoCalcKeyBoard::PicoCalcKeyBoard()
    : KeyBoard(), _i2c_inited(1), _keycheck(0) 
{
    gpio_set_function(I2C_KBD_SCL, GPIO_FUNC_I2C);
    gpio_set_function(I2C_KBD_SDA, GPIO_FUNC_I2C);
    i2c_init(I2C_KBD_MOD, I2C_KBD_SPEED);
    gpio_pull_up(I2C_KBD_SCL);
    gpio_pull_up(I2C_KBD_SDA);
}

bool 
PicoCalcKeyBoard::wait_any_key()
{
    bool r = false;
    int k = _read_i2c_kbd();
    if (k > 0)
    {
        r = true;
    }
    return r;
}

bool 
PicoCalcKeyBoard::fetch_key(uint8_t &c)
{
    int k = _read_i2c_kbd();
    bool r = false;
    if (k > 0)
    {
        c = (uint8_t)k;
        r = true;
    }
    return r;
}

int 
PicoCalcKeyBoard::_write_i2c_kbd() 
{
    int retval;
    unsigned char msg[2];
    msg[0] = 0x09;

    if(_i2c_inited == 0) return -1;

    retval = i2c_write_timeout_us(I2C_KBD_MOD, I2C_KBD_ADDR, msg, 1, false, 500000);
    if ( retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT) 
    {
        Serial1.printf( "i2c write error\r\n");
        return -1;
    }
    return 0;
}

int 
PicoCalcKeyBoard::_read_i2c_kbd() 
{
    int retval;
    static int ctrlheld = 0;
    uint16_t buff = 0;
    unsigned char msg[2];
    int c  = -1;
    msg[0] = 0x09;

    if(_i2c_inited == 0) return -1;

    if(_keycheck == 0)
    {
        retval = _write_i2c_kbd();
        _keycheck = 1;
        return retval;
    }
    else 
    {
        retval = i2c_read_timeout_us(I2C_KBD_MOD, I2C_KBD_ADDR, (unsigned char *) &buff, 2, false, 500000);
        if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT) 
        {
            Serial1.printf("i2c read error read\n");
            return -1;
        }
        _keycheck = 0;
    }
    if(buff != 0) 
    {
        if (buff == 0xA503) ctrlheld = 0;
        else if (buff == 0xA502) 
        {
            ctrlheld = 1;
        }
        else if((buff & 0xff) == 1) 
        {//pressed
            c = buff >> 8;
            int realc = -1;
            switch (c) 
            {
            case 0xA1:
            case 0xA2:
            case 0xA3:
            case 0xA4:
            case 0xA5:
                realc = -1;//skip shift alt ctrl keys
                break;
            default:
                realc = c;
                break;
            }
            c = realc;
            if(c >= 'a' && c <= 'z' && ctrlheld) c = c - 'a' + 1;
        }
        return c;
    }
    return -1;
}