#include "AQUA_ads1110.h"

#include <Wire.h>
/*
  Project: Ph Controller
  Library: ADS1110
  Version: 1
  Author: Dmitry Kapustin 
	code based on "Aquarium Controller" project by Rastislav Birka
*/

void AQUA_ads1110::init(uint8_t adr, TwoWire *wire) {
  _adr = adr;
	_wire = wire;
	//init adc, write config register
	_wire->beginTransmission(_adr);
	_wire->write(ADS1110_OPTIONS);
	_wire->endTransmission();
}

float AQUA_ads1110::getValue() {
  byte configRegister;
	int16_t tmp;
  int16_t values[AVERAGE_SAMPLE_COUNT+AVERAGE_DISCARD_COUNT+AVERAGE_DISCARD_COUNT];
  float voltage=0;
  uint8_t i, j;
	//start i2c transmission
	_wire->beginTransmission(_adr);
	//Read AVERAGE_SAMPLE_COUNT values
  for(i = 0; i < AVERAGE_SAMPLE_COUNT; i++) {
		_wire->requestFrom((uint8_t)_adr, (uint8_t)3);
		while(_wire->available()) { // ensure all the data comes in
			values[i] = _wire->read(); // high byte * B11111111
			values[i] = values[i] << 8;
			values[i] += _wire->read();  // low byte
		  configRegister = _wire->read();
		}
  }
	//end transmission
	_wire->endTransmission();
	//sorting values
  for(i = 0; i < AVERAGE_SAMPLE_COUNT-1; i++) {
    for(j = i+1; j < AVERAGE_SAMPLE_COUNT; j++) {
      if(values[i] > values[j]) {
        tmp = values[i];
        values[i] = values[j];
        values[j] = tmp;
      }
    }
  }
	//discard lowest and highest values
  for(i = AVERAGE_DISCARD_COUNT; i < AVERAGE_SAMPLE_COUNT-AVERAGE_DISCARD_COUNT; i++) {
    voltage += values[i];
  }
	//average value
	voltage = voltage/(AVERAGE_SAMPLE_COUNT-AVERAGE_DISCARD_COUNT-AVERAGE_DISCARD_COUNT);
	voltage = (voltage * 2.048 / 32768); // ads1110 vref=2.048V, 32768-16bit adc, *1000 - mv
	return voltage;
}