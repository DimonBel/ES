#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>

class RgbLed {
public:
    RgbLed(uint8_t pinR, uint8_t pinG, uint8_t pinB);
    ~RgbLed();

    void begin();
    
    // Color control methods
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setRed(uint8_t brightness = 255);
    void setGreen(uint8_t brightness = 255);
    void setBlue(uint8_t brightness = 255);
    void off();
    
    // Predefined colors
    void red();
    void green();
    void blue();
    void yellow();
    void cyan();
    void magenta();
    void white();
    
    // Get current state
    void getColor(uint8_t &r, uint8_t &g, uint8_t &b) const;
    
private:
    uint8_t _pinR;
    uint8_t _pinG;
    uint8_t _pinB;
    
    uint8_t _r;
    uint8_t _g;
    uint8_t _b;
    
    void _writePins();
};

#endif // RGB_LED_H