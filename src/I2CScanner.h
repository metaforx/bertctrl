#ifndef I2CSCANNER_H
#define I2CSCANNER_H

#include "Particle.h"
#include "Wire.h"

class I2CScanner
{
public:
    I2CScanner();
    bool begin();
    void scan();
};

#endif // I2CSCANNER_H