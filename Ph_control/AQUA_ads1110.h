#ifndef AQUA_ADS1110_H
#define AQUA_ADS1110_H

#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

#include <Wire.h>

/*
  Project: Ph Controller
  Library: ADS1110
  Version: 1
  Author: Dmitry Kapustin 
	code based on "Aquarium Controller" project by Rastislav Birka
*/



/*  set register:  STBY   0  0    SC    DR1  DR0 PGA1  PGA0
          default   1     0  0    0      1    1    0     0      0x8C
          i want    1     0  0    0      1    1    0     0
                    ign   0  0   con      15SPS      GAIN 1
    STBY, only for single mode to start conversion
    SC    1= single , 0=continuous
    DR1:0    datarate 00 = 240sps, 12 bit      -2048 to  2047
                      01 =  60sps, 14          -8192 to  9191
                      10 =  30sps, 15         -16384 to 16383
                      11 =  15sps, 16         -32768 to 32767 
    PGA1:0  Gain      00 = *1, 01 = *2, 10 = *4, 11 = *8 
*/
#define ADS1110_OPTIONS				B10001100
//averaging count (maximum 255)
#define AVERAGE_SAMPLE_COUNT	10
//discard samle count, totaly read sample AVERAGE_DISCARD_COUNT+AVERAGE_SAMPLE_COUNT+AVERAGE_DISCARD_COUNT
//reading time = (AVERAGE_SAMPLE_COUNT+2*AVERAGE_DISCARD_COUNT) / 15frame/sec = 8 sec.
//(maximum 255), recomended AVERAGE_SAMPLE_COUNT/10
#define	AVERAGE_DISCARD_COUNT	1

class AQUA_ads1110 {
  public:
    void init(uint8_t, TwoWire*);
    float getValue();

  private:
		TwoWire *_wire;
    uint8_t _adr;
};
#endif
