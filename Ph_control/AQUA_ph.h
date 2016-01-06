/*
  Project: Aquarium Controller
  Library: PH
  Version: 2.1
  Author: Rastislav Birka
*/

#ifndef AQUA_ph_h
#define AQUA_ph_h

#include "CalibrationPoint.h"
#include "AQUA_ads1110.h"

class AQUA_ph {
  public:
    void init(uint8_t calibrate_points, uint8_t calibrate_address);
    //void useInternalADC();
    //void useADC141S626(uint8_t _voutPin, uint8_t _misoPin = 50, uint8_t _mosiPin = 51, uint8_t _sclkPin = 52, uint8_t _ssPin = 53);
    void useADS1110(uint8_t adr, TwoWire *wire);
    float getPH(float T, bool calibrate=true);
    bool calibration(uint8_t point, CalibrationPoint *values);
    CalibrationPoint readCalibrationPoint(uint8_t point);

  private:
    //uint8_t _adc;
    //uint8_t _voutPin, _vocmPin;
    uint8_t _pointCount;
    uint8_t _calibrateAddress;
    CalibrationPoint* _calData;
    CalibrationPoint* _usedData;
    uint8_t _usedPoints;
    float* _const;
    float _alpha, _constPerUnit;
    //AQUA_adc141s626 *objADC141S626;
    AQUA_ads1110 *objADS1110;

    void _setCalibrationValues();
    //float _readInternalADC();
};
#endif
