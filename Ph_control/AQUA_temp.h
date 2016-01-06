/*
  Project: Aquarium Controller
  Library: Temperature (a 4.7K resistor is necessary)
  Version: 1.0
  Author: Rastislav Birka
  Modified: Dmitry Kapustin (
*/

#ifndef AQUA_temp_h
#define AQUA_temp_h

#include "CalibrationPoint.h"
#include <DallasTemperature.h>

class AQUA_temp {
  public:
    void init(DallasTemperature* sensors, uint8_t index, uint8_t calibrate_points, uint8_t calibrate_address);
    float getTemp(bool calibrate=true);
    bool calibration(uint8_t point, CalibrationPoint *values);
    CalibrationPoint readCalibrationPoint(uint8_t point);

  private:
    uint8_t _index;
	DallasTemperature* _sensors;
    uint8_t _pointCount;
    uint8_t _calibrateAddress;
    CalibrationPoint* _calData;
    CalibrationPoint* _usedData;
    uint8_t _usedPoints;
    float* _const;

    void _setCalibrationValues();    
};
#endif
