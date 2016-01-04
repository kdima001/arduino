#include "Ph_control.h"

#include <SoftwareSerial.h>
#include <Wire.h>
#include <EEPROM.h>
#include <LedControl.h>
#include <Bounce2.h>
/*#include <OneWire.h>*/
/*#include <DallasTemperature.h>*/

#include "AQUA_ph.h"
#include "AQUA_temp.h"
#include "CalibrationPoint.h"

//Кнопки
Bounce btnMode = Bounce();
Bounce btnMinus = Bounce();
Bounce btnPlus = Bounce();

// Переменные пуллинга					
unsigned long curMillis;
unsigned long prevMillis, displayTime;
unsigned long btnModeTime = 0, btnMinusTime = 0, btnPlusTime = 0;

//Ошибка 
uint8_t ERROR;
//Режим работы
uint8_t MODE;

uint8_t V1, V2;

// Объекты для 1-Wire термометров
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
//OneWire oneWire1(ONE_WIRE_BUS1);
//OneWire oneWire2(ONE_WIRE_BUS2);

// Pass our oneWire reference to Dallas Temperature. 
//DallasTemperature sensors1(&oneWire1);
//DallasTemperature sensors2(&oneWire2);

//cal
CalibrationPoint CalPoint; 
//Temp
AQUA_temp objT1, objT2;
/* 
 * Now we create a new LedControl. 
 * We use pins 12,11 and 10 on the Arduino for the SPI interface
 * Pin 12 is connected to the DATA IN-pin of the first MAX7221
 * Pin 11 is connected to the CLK-pin of the first MAX7221
 * Pin 10 is connected to the LOAD(/CS)-pin of the first MAX7221 	
 * There will only be a single MAX7221 attached to the arduino 
 */
LedControl lc=LedControl(12,11,10,1);

//pH
AQUA_ph objpH1, objpH2;

int index = 0;
float pH1, pH2,
			target_pH1 = 6.50,
			target_pH2 = 7.00,
			T1 = 24,
			T2 = 24,
			curValue = 0;

//---------------------------------------------------------------------------
void SetValve(uint8_t pin, uint8_t state) {
	//если клапан на PWM выходе - включаем и переходим в удержание
	if (state && (pin == 6 || pin == 9 || pin == 10 || pin == 11 )) {
		//даем на всю железку
		analogWrite(pin, ON_VALVE_VALUE);
		delay(200);
		//переходим на пол-шишечки
		analogWrite(pin, HOLD_VALVE_VALUE);
		return;
	}		
	digitalWrite(pin, state);		
}			
//----------------------------------------------------------------
void setup()
{ Wire.begin();
  Serial.begin(57600);
	//wake up the MAX72XX from power-saving mode 
  lc.shutdown(0,false);
  //set a medium brightness for the Leds
  lc.setIntensity(0, LED_INTENSITY);
	lc.clearDisplay(0);
	
	// Start up the library
  /*sensors1.begin();
	sensors1.setResolution(TEMPERATURE_PRECISION);

	sensors2.begin();
  sensors2.setResolution(TEMPERATURE_PRECISION); 
	*/
	V1 = LOW;
	V2 = LOW;
	SetValve(VALVE1_PIN, V1);		
	SetValve(VALVE2_PIN, V2);
	
	eeprom_busy_wait();
	target_pH1 = eeprom_read_word((const uint16_t *)ADR_TARGET_PH1);
	target_pH1 = target_pH1 / 1000;
	
	eeprom_busy_wait();
	target_pH2 = eeprom_read_word((const uint16_t *)ADR_TARGET_PH2);
	target_pH2 = target_pH2 / 1000;

	objpH1.init(PH_CALIBRATE_POINTS, PH1_CALIBRATE_ADDR);
	objpH1.useADS1110(pH1_I2C_adr, &Wire);
	
	objpH2.init(PH_CALIBRATE_POINTS, PH2_CALIBRATE_ADDR);
	objpH2.useADS1110(pH1_I2C_adr, &Wire);
	
	objT1.init(T1_DQ_PIN, T_CALIBRATE_POINTS, T1_CALIBRATE_ADDR);
		
	objT2.init(T2_DQ_PIN, T_CALIBRATE_POINTS, T2_CALIBRATE_ADDR);
	
	// Setup the button
	pinMode(BUTTON_MODE_PIN, 	INPUT_PULLUP);
	btnMode.attach(BUTTON_MODE_PIN);
	btnMode.interval(5);
	
	pinMode(BUTTON_MINUS_PIN, INPUT_PULLUP);
	btnMinus.attach(BUTTON_MINUS_PIN);
	btnMinus.interval(5);
	
	pinMode(BUTTON_PLUS_PIN, 	INPUT_PULLUP);
	btnPlus.attach(BUTTON_PLUS_PIN);
	btnPlus.interval(5);
  
}
//---------------------------------------------------------------------------
//режим будет сменен
void BeforeChangeMode(uint8_t oldMode, uint8_t newMode) {
	//выход из режима - сохранение сделанных настроек
	switch (newMode){ 
		case MODE_CAL_PH1_1:
			CalPoint = objpH1.readCalibrationPoint(0);
			break;
		case MODE_CAL_PH1_2:
			CalPoint = objpH1.readCalibrationPoint(1);
			break;
		case MODE_CAL_PH2_1:
			CalPoint = objpH2.readCalibrationPoint(0);
			break;
		case MODE_CAL_PH2_2:
			CalPoint = objpH2.readCalibrationPoint(1);
			break;
		case MODE_CAL_T1_1:
			CalPoint = objT1.readCalibrationPoint(0);
			break;
		case MODE_CAL_T1_2:
			CalPoint = objT1.readCalibrationPoint(1);
			break;
		case MODE_CAL_T2_1:
			CalPoint = objT2.readCalibrationPoint(0);
			break;
		case MODE_CAL_T2_2:
			CalPoint = objT2.readCalibrationPoint(1);
			break;
	}
	
	curValue = CalPoint.refValue;
	
	switch (newMode) { 
		case MODE_SET_PH1:	
			curValue = target_pH1; 
			break;	
		case MODE_SET_PH2: 
			curValue = target_pH2; 
			break;
	}		
}
//---------------------------------------------------------------------------
void DecreaseCurValue(void) {
	switch (MODE){ 
		//для pH минимум настройки 6.00
		case MODE_SET_PH1:	
		case MODE_SET_PH2:  
		case MODE_CAL_PH1_1:
    case MODE_CAL_PH1_2:
		case MODE_CAL_PH2_1:
		case MODE_CAL_PH2_2:
			if ( curValue > 6.00 ) curValue -= 0.01;
			break;
		//Для температуры минимум 18.0
		case MODE_CAL_T1_1:
		case MODE_CAL_T1_2:
		case MODE_CAL_T2_1:
		case MODE_CAL_T2_2:
			if ( curValue > 18.00 ) curValue -= 0.1;
			break;
	}
}
//---------------------------------------------------------------------------
void IncreaseCurValue(void) {
	switch (MODE){ 
		//для pH максимум настройки 8.00
		case MODE_SET_PH1:	
		case MODE_SET_PH2:  
		case MODE_CAL_PH1_1:
    case MODE_CAL_PH1_2:
		case MODE_CAL_PH2_1:
		case MODE_CAL_PH2_2:
			if ( curValue < 8.00 ) curValue += 0.01;
			break;
		//Для температуры максимум 33.0
		case MODE_CAL_T1_1:
		case MODE_CAL_T1_2:
		case MODE_CAL_T2_1:
		case MODE_CAL_T2_2:
			if ( curValue < 33.00 ) curValue += 0.1;
			break;
	}
}
//---------------------------------------------------------------------------
//Сменили режим
void SaveSettings(void){
	switch (MODE){
		case MODE_SET_PH1:	 
			target_pH1 = curValue;
			eeprom_busy_wait();
			eeprom_write_word((uint16_t *)ADR_TARGET_PH1, (uint16_t)(target_pH1*1000)); 
			break;	
		case MODE_SET_PH2:   
			target_pH2 = curValue;
			eeprom_busy_wait();
			eeprom_write_word((uint16_t *)ADR_TARGET_PH2, (uint16_t)(target_pH2*1000));
			break;
		case MODE_CAL_PH1_1:
			//запишем калибровочную точку
			CalPoint.state = true;
			CalPoint.refValue = curValue;
			CalPoint.actValue = objpH1.getPH(1, T1); // измеряем pH без компенсации
			objpH1.calibration(0, &CalPoint);
			break;
		case MODE_CAL_PH1_2:
			CalPoint.state = true;
			CalPoint.refValue = curValue;
			CalPoint.actValue = objpH1.getPH(1, T1); // измеряем pH без компенсации
			objpH1.calibration(1, &CalPoint);
			break;
		case MODE_CAL_PH2_1:
			break;
		case MODE_CAL_PH2_2:
			break;
		case MODE_CAL_T1_1:
			break;
		case MODE_CAL_T1_2:
			break;
		case MODE_CAL_T2_1:
			break;
		case MODE_CAL_T2_2:
			break;
	}
	//Мигнем экраном 3 раза в знак сохранения параметров
	lc.setIntensity(0, 0); lc.setIntensity(1, 0);													delay(50);
	lc.setIntensity(0, LED_INTENSITY); lc.setIntensity(1, LED_INTENSITY);	delay(50);
	lc.setIntensity(0, 0); lc.setIntensity(1, 0);													delay(50);
	lc.setIntensity(0, LED_INTENSITY); lc.setIntensity(1, LED_INTENSITY);	delay(50);
	lc.setIntensity(0, 0); lc.setIntensity(1, 0);													delay(50);
	lc.setIntensity(0, LED_INTENSITY); lc.setIntensity(1, LED_INTENSITY);
}
//---------------------------------------------------------------------------
void ProcessBTN(void) {
	static uint8_t /*btnMode, btnMinus, btnPlus,*/ tmpi;
	unsigned long btnModeTimeRepeate, btnMinusTimeRepeate, btnPlusTimeRepeate;
	
	btnMode.update(); 
	btnMinus.update(); 
	btnPlus.update(); 
	
	//Отработаем кнопку режима
	//нажата кнопка
	if (btnMode.read()==BUTTON_PRESSED && btnModeTime == 0) { //только нажали (до этого был отпущен)
		//Кнопка точно нажата, до этого была отпущена
		btnModeTime = btnModeTimeRepeate = curMillis;
	} 
	
	//держится кнопка
	if (btnMode.read()==BUTTON_PRESSED && btnModeTime > 0) { 
		if ( (curMillis - btnModeTimeRepeate) > BUTTON_MODE_REPEAT_TIME ) // мыргаем первый квартетом цифр каждую секунду
			btnModeTimeRepeate = curMillis;
			lc.setIntensity(0, 0);
			delay(100);
			lc.setIntensity(0, LED_INTENSITY);			
	}
	
	//отпустили кнопку
	if ( btnMode.read()==BUTTON_RELEASED && btnModeTime > 0 ) {//отпустили батон режима
		if ( btnModeTime < WORK_MODE_TIME ) { //держали мало, переключаем режим на следующий
			//Переключаем режим на следующий
			tmpi = ( MODE >= MODE_MAX ) ? MODE_MIN : MODE+1;
			BeforeChangeMode(MODE, tmpi);
			MODE = tmpi;			
			Serial.print("MODE="); Serial.println(MODE, DEC);
		}
		if ( btnModeTime >= WORK_MODE_TIME && btnModeTime < SAVE_CALIBRATION_TIME) { // держали от 2х до 5ти - выход к рабочему режиму
			BeforeChangeMode(MODE, MODE_WORK);
			MODE = MODE_WORK;
		}
		if ( btnModeTime >= SAVE_CALIBRATION_TIME) { // держали от 2х до 5ти - выход к рабочему режиму
			SaveSettings();			
		}
		btnModeTime = 0; //обнулим таймер кнопки режима
	}
	
	//Обработаем минус
	//нажата кнопка +
	if (btnMinus.read()==BUTTON_PRESSED && btnMinusTime == 0) { //только нажали (до этого был отпущен)
		//Кнопка точно нажата, до этого была отпущена
		btnMinusTimeRepeate = btnMinusTime = curMillis;
		 //= curMillis;
		DecreaseCurValue();
	} 
	
	//держится кнопка -
	if (btnMinus.read()==BUTTON_PRESSED && btnMinusTime > 0) { 
		if ( curMillis - btnMinusTime > BUTTON_VALUE_DELAY_TO_REPEAT ) { //подождали до повтора
			if ( (curMillis - btnMinusTimeRepeate) > BUTTON_VALUE_REPEAT_TIME ) // зареган повтор
				btnMinusTimeRepeate = curMillis;
				DecreaseCurValue();
		}		
	}
	
	//Обработаем плюс
	//нажата кнопка +
	if (btnPlus.read()==BUTTON_PRESSED && btnPlusTime == 0) { //только нажали (до этого был отпущен)
		//Кнопка точно нажата, до этого была отпущена
		btnPlusTimeRepeate = btnPlusTime = curMillis;
		 //= curMillis;
		IncreaseCurValue();
	} 
	
	//держится кнопка +
	if (btnPlus.read()==BUTTON_PRESSED && btnPlusTime > 0) { 
		if ( curMillis - btnPlusTime > BUTTON_VALUE_DELAY_TO_REPEAT ) { //подождали до повтора
			if ( (curMillis - btnPlusTimeRepeate) > BUTTON_VALUE_REPEAT_TIME ) // зареган повтор
				btnPlusTimeRepeate = curMillis;
				IncreaseCurValue();
		}		
	}

}
//-------------------------------------------------------------
void Write4Char(uint8_t num, char str[]) {//num = 0 первый квартет, num = 1 второй квартет
	lc.setChar(0, 0+num*4, str[0], false);
	lc.setChar(0, 1+num*4, str[1], false);
	lc.setChar(0, 2+num*4, str[2], false);
	lc.setChar(0, 3+num*4, str[3], false);
}
//-------------------------------------------------------------
void Write3Digit(uint8_t num, char ch, float f, bool point1 = false, bool point2 = false, bool point3 = false) {//num = 0 первый квартет, num = 1 второй квартет
	static char tmps[8];
	sprintf(tmps, "%2d.%02d\0", f);
	lc.setChar(0, 0+num*4, ch, point1);
	lc.setChar(0, 1+num*4, tmps[0], point2);
	lc.setChar(0, 2+num*4, tmps[2], point3);
	lc.setChar(0, 3+num*4, tmps[3], false);
}
//-------------------------------------------------------------
void Write2Digit(uint8_t num, char str[], float f) {//num = 0 первый квартет, num = 1 второй квартет
	static char tmps[8];
	sprintf(tmps, "%1d.%01d\0", f);
	lc.setChar(0, 0+num*4, str[0], false);
	lc.setChar(0, 1+num*4, str[1], false);
	lc.setChar(0, 2+num*4, tmps[2], true);
	lc.setChar(0, 3+num*4, tmps[3], false);
}
//-------------------------------------------------------------
void Write4Digit(uint8_t num, float f) {//num = 0 первый квартет, num = 1 второй квартет
	static char tmps[8];
	sprintf(tmps, "%2d.%02d\0", f);
	lc.setChar(0, 0+num*4, tmps[0], false);
	lc.setChar(0, 1+num*4, tmps[1], false);
	lc.setChar(0, 2+num*4, tmps[2], true);
	lc.setChar(0, 3+num*4, tmps[3], false);
}
//-------------------------------------------------------------
void ProcessDisplay(void){
	static char tmps[8];
	if (displayTime >= WORK_DISPLAY_INT+WORK_DISPLAY_INT)
		displayTime = 0;
	else
		displayTime ++;
	//Вывод на экран
	//lc.clearDisplay(0);
					
	switch (MODE){
		case MODE_WORK:		// ph: -0.00 -0.00 / t: 00.00 00.00 (5 сек pH / 5 сек температура)
			if ( displayTime < WORK_DISPLAY_INT ) {
				//Выводим кислотность
				if (ERROR&ERROR_PH1) Write3Digit(0, 'E', 9.99);
				else {
					Write3Digit(0, ' ', pH1, false, true);
					//Если кислотность повышенная - отразим символ включенного клапана
					if (pH1>target_pH1) lc.setChar (0, 0, '-', false);
				}
				if (ERROR&ERROR_PH2) Write3Digit(1, 'E', 9.99);
				else {
					Write3Digit(1, ' ', pH2, false, true);
					if (pH2>target_pH2) lc.setChar (0, 4, '-', false);
				}
			} else {
				//Выводим температуру
				if (ERROR&ERROR_T1) Write4Char(0, "E---");
				else Write4Digit(0, T1);
				if (ERROR&ERROR_T2) Write4Char(1, "E---");
				else Write4Digit(1, T2);
			}
			break;
		case MODE_SET_PH1:  // c0.00 00.00
			Write4Char (0, "PH1 ");
			Write4Digit(1, target_pH1);
			break;
		case MODE_SET_PH2:	// P0.00 
			Write4Char (0, "PH2 ");
			Write4Digit(1, target_pH2);
			break;	
		case MODE_CAL_PH1_1:	// c	#	#.	#		1.	#.	#	#
			Write3Digit(0, 'c', T1, 					false, false, true);
			Write3Digit(0, '1', curValue, 	  true);
			break;
		case MODE_CAL_PH1_2:  // c	#	#.	#		2.	#.	#	#
			Write3Digit(0, 'c', T1, 					false, false, true);
			Write3Digit(0, '2', curValue, 	  true);
			break;
		case MODE_CAL_PH2_1:	// c.	#	#.	#		1.	#.	#	#
			Write3Digit(0, 'b', T2, 					true, false, true);
			Write3Digit(0, '1', curValue, 	  true, true);
			break;
		case MODE_CAL_PH2_2:  //c.	#	#.	#		2.	#.	#	#
			Write3Digit(0, 'b', T2, 					true, false, true);
			Write3Digit(0, '2', curValue, 	  true, true);
			break;
	}
	
}
//-------------------------------------------------------------
void loop()
{ char tmps[16];
	curMillis = millis();		
	ProcessBTN();
	curMillis = millis();
	ProcessDisplay();	
	curMillis = millis();
	if(curMillis - prevMillis > POOL_INT){
		prevMillis = curMillis;

		ERROR = 0;
		
		T1 = objT1.getTemp();
		if( T1>99.99 || T1<0) ERROR |= ERROR_T1;
		
		T2 = objT2.getTemp();
		if( T2>99.99 || T2<0) ERROR |= ERROR_T2;
		
		pH1 = objpH1.getPH(0, T1);
		if (pH1>9.99) ERROR |= ERROR_PH1;
		
		pH2 = objpH2.getPH(0, T2);
		if (pH2>9.99) ERROR |= ERROR_PH2;
		
		Serial.print("T1="); Serial.print(T1); Serial.print('\t');
		Serial.print("pH1="); Serial.print(pH1); Serial.print('\t');
		Serial.print("T2="); Serial.print(T2); Serial.print('\t');
		Serial.print("pH2="); Serial.print(pH2); Serial.print('\t');
		Serial.println();
		
		if (MODE ==  MODE_WORK) {
			if (ERROR&ERROR_PH1 || ERROR&ERROR_T1) { V1=LOW; SetValve(VALVE1_PIN, LOW);	}
			else {
				if (pH1> target_pH1 && V1==LOW ) { V1 = HIGH; SetValve(VALVE1_PIN, HIGH); }	//Включить клапан для понижения pH
				if (pH1<=target_pH1 && V1==HIGH) { V1 = LOW;  SetValve(VALVE1_PIN, HIGH); }	//Включить клапан для понижения pH
			}
			if (ERROR&ERROR_PH2 || ERROR&ERROR_T2) { V2=LOW; SetValve(VALVE2_PIN, LOW);	}
			else {
				if (pH2> target_pH2 && V2==LOW ) { V2 = HIGH; SetValve(VALVE2_PIN, HIGH); }	//Включить клапан для понижения pH
				if (pH2<=target_pH2 && V2==HIGH) { V2 = LOW;  SetValve(VALVE2_PIN, HIGH); }	//Включить клапан для понижения pH
			}
		}
	}
}