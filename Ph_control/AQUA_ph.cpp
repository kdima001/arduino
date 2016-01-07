/*
  Project: Aquarium Controller
  Library: PH
  Version: 2.1
  Author: Rastislav Birka
*/

#include <avr/eeprom.h>
#include <Arduino.h>
#include "CalibrationPoint.h"
#include "AQUA_ph.h"
#include <Streaming.h>  

/*
  Public Functions
*/

void AQUA_ph::init(uint8_t calibrate_points, uint8_t calibrate_address) {
  uint16_t value, position;
  uint8_t i;
  
  avg_empty = true;
  
  _pointCount = calibrate_points;
  _calibrateAddress = calibrate_address;
  _calData = new CalibrationPoint[_pointCount];
  _usedData = new CalibrationPoint[_pointCount];
  _const = new float[(_pointCount - 1)*2];

  position = 0;
  for(i = 0; i < _pointCount; i++) {
    eeprom_busy_wait();
    value = eeprom_read_word((const uint16_t *)(_calibrateAddress + position));
    if(value == 0xFFFF) { //address wasn't set yet
      _calData[i].state = 0;
      _calData[i].refValue = 0;
      _calData[i].actValue = 0;
      position+= 4;
    } else {
      _calData[i].state = (bool)(value & 32768);
      if(_calData[i].state == 1) {
         value-= 32768;
      }
      _calData[i].refValue = (float)value/1000.0;
      position+= 2;
      eeprom_busy_wait();
      value = eeprom_read_word((const uint16_t *)(_calibrateAddress + position));
      _calData[i].actValue = (float)value/1000.0;
      position+= 2;
    }
  }
  _setCalibrationValues();
  objADS1110 = new AQUA_ads1110;
}

void AQUA_ph::useADS1110(uint8_t adr, TwoWire *wire) {
  objADS1110->init(adr, wire);
}

/*
LMP91200
pH = 7 + (VOUT - VOCM)/alpha
alpha = -59.16mV/pH @ 25°C
*/
float AQUA_ph::getPH(float T, bool calibrate) {
	uint8_t i;
	float res, adcValue;
	adcValue = objADS1110->getValue();
	res = 7.00-adcValue/((273.15+T)*0.0001981986367);
	
	if(avg_empty) { //first meas
		avg_empty = false;
		avg_val[0] = res;		avg_val[1] = res;		avg_val[2] = res;		avg_val[3] = res;		avg_val[4] = res;
	}
		
	for(i = 0, avg = 0; i<4; i++) { //продвигаем вперед 4 правых элемента массива и подсуммируем
		avg_val[i] = avg_val[i+1];
		avg += avg_val[i];
	}
	
	avg_val[4] = res;	//пихаем значение в последний элемент
	avg += avg_val[4]; //подсуммируем
	
	res = (float)avg/5.0; //усредняем и переходим обратно во float
	
	if(calibrate) {
    if(_usedPoints == 1) {
      res+= _const[0];
    } else if(_usedPoints > 1) {
      if(res >= _usedData[_usedPoints-1].actValue) {
        res = _const[(_usedPoints-2)*2]*res + _const[(_usedPoints-2)*2 + 1];
      } else {
        for(i = 1; i < _usedPoints; i++) {
          if(res <= _usedData[i].actValue) {
            res = _const[(i-1)*2]*res + _const[(i-1)*2 + 1];
            break;
          }
        }
      }
    }
  }
	
  return (round(res*100.0))/100.0;  
}
//-------------------------------------------------------------------------------------------------------------
bool AQUA_ph::calibration(uint8_t point, CalibrationPoint *values) {
  bool res = false;

  if(point < _pointCount && point >= 0 
			&& values->refValue <= 14 && values->refValue >= 0 
			&& values->actValue <= 14 && values->actValue >= 0) {
    if(values->state != _calData[point].state 
			|| values->refValue != _calData[point].refValue 
			|| values->actValue != _calData[point].actValue) {
      _calData[point].state = values->state;
      _calData[point].refValue = values->refValue;
      _calData[point].actValue = values->actValue;
      uint16_t plusValue = 0;
      if(_calData[point].state == 1) {
        plusValue+= 32768;
      }
      uint16_t position = point*4;
      eeprom_busy_wait();
      eeprom_write_word((uint16_t *)(_calibrateAddress + position), (uint16_t)(_calData[point].refValue*1000) + plusValue);
      position+= 2;
      eeprom_busy_wait();
      eeprom_write_word((uint16_t *)(_calibrateAddress + position), (uint16_t)(_calData[point].actValue*1000));
      _setCalibrationValues();
      res = true;
    }
  }
  return res;
}

CalibrationPoint AQUA_ph::readCalibrationPoint(uint8_t point) {
  CalibrationPoint calPoint;
  if(point < _pointCount && point >= 0) {
    calPoint.state = _calData[point].state;
    calPoint.refValue = _calData[point].refValue;
    calPoint.actValue = _calData[point].actValue;
  } else {
    calPoint.state = 0;
    calPoint.refValue = 0;
    calPoint.actValue = 0;
  }
  return calPoint;
}

/*
  Private Functions
*/

/*
c1 = (ref2 - ref1)/(act2 - act1)
c2 = ref2 - c1*act2
pH = c1*SensorValue + c2
*/
void AQUA_ph::_setCalibrationValues() {
  uint8_t i,j;
  bool isCorrect;

  _usedPoints = 0;
  for(i = 0; i < _pointCount; i++) {
    if(_calData[i].state == 1 && _calData[i].actValue > 0 && _calData[i].refValue > 0) {
      if(_usedPoints > 0) {
        isCorrect = 1;
        for(j = 0; j < _usedPoints; j++) {
          if(_calData[i].actValue == _usedData[j].actValue || _calData[i].refValue == _usedData[j].refValue) {
            isCorrect = 0;
            break;
          }
        }
        if(isCorrect == 1) {
          j = _usedPoints;
          for(j; j > 0 && _usedData[j-1].actValue > _calData[i].actValue; --j) {
            _usedData[j].state = _usedData[j-1].state;
            _usedData[j].refValue = _usedData[j-1].refValue;
            _usedData[j].actValue = _usedData[j-1].actValue;
          }
          _usedData[j].state = _calData[i].state;
          _usedData[j].refValue = _calData[i].refValue;
          _usedData[j].actValue = _calData[i].actValue;
          _usedPoints++;
        }
      } else {
        _usedData[_usedPoints].state = _calData[i].state;
        _usedData[_usedPoints].refValue = _calData[i].refValue;
        _usedData[_usedPoints].actValue = _calData[i].actValue;
        _usedPoints++;
      }
    }
  }
  if(_usedPoints == 1) {
    _const[0] = (_usedData[0].refValue - _usedData[0].actValue);
  } else if(_usedPoints > 1) {
    for(i = 0; i < _usedPoints - 1; i++) {
      _const[i*2] = (_usedData[i+1].refValue - _usedData[i].refValue)/(_usedData[i+1].actValue - _usedData[i].actValue);
      _const[i*2 + 1] = _usedData[i+1].refValue - _const[i*2]*_usedData[i+1].actValue;
    }
  }
}
