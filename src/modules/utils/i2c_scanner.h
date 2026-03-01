#ifndef I2C_SCANNER_H
#define I2C_SCANNER_H

#include <Arduino.h>
#include <Wire.h>

class I2CScanner
{
public:
    static void scan()
    {
        byte error, address;
        int nDevices;

        printf("\nScanning I2C devices...\n");

        nDevices = 0;
        for (address = 1; address < 127; address++)
        {
            Wire.beginTransmission(address);
            error = Wire.endTransmission();

            if (error == 0)
            {
                printf("I2C device found at address 0x");
                if (address < 16)
                {
                    printf("0");
                }
                printf("%02X\n", address);
                nDevices++;
            }
            else if (error == 4)
            {
                printf("Unknown error at address 0x");
                if (address < 16)
                {
                    printf("0");
                }
                printf("%02X\n", address);
            }
        }

        if (nDevices == 0)
        {
            printf("No I2C devices found\n");
        }
        else
        {
            printf("Total devices found: %d\n\n", nDevices);
        }
    }
};

#endif // I2C_SCANNER_H
