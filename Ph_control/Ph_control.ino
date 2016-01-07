#include "Ph_control.h"

#include <Wire.h>
#include <EEPROM.h>
#include <LedControl.h>
#include <Streaming.h>  
#include <avr/wdt.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "AQUA_ph.h"
#include "AQUA_temp.h"
#include "CalibrationPoint.h"

// Переменные пуллинга					
unsigned long curMillis, prevMillis;

//Ошибка 
uint8_t ERROR;
//Режим работы
uint8_t MODE;

//Массив для состояния выходов
uint8_t out[6];

//cal
CalibrationPoint calPoint;

//Temp
AQUA_temp objT1, objT2;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

/* 
 * Now we create a new LedControl. 
 * We use pins 12,11 and 10 on the Arduino for the SPI interface
 * Pin 12 is connected to the DATA IN-pin of the first MAX7221
 * Pin 11 is connected to the CLK-pin of the first MAX7221
 * Pin 10 is connected to the LOAD(/CS)-pin of the first MAX7221 	
 * There will only be a single MAX7221 attached to the arduino 
 */
LedControl lc=LedControl(12, 10, 11, 1);

//pH
AQUA_ph objpH1, objpH2;

int index = 0, tmpi;
float 		pH1 		= DEFAULT_PH, 
			pH2 		= DEFAULT_PH,
			target_pH1 	= DEFAULT_PH,
			target_pH2 	= DEFAULT_PH,
			delta_pH1 	= DEFAULT_D_PH,
			delta_pH2 	= DEFAULT_D_PH,
			
			T1 			= DEFAULT_TEMP,
			T2 			= DEFAULT_TEMP,
			target_T1 	= DEFAULT_TEMP,
			target_T2 	= DEFAULT_TEMP,
			delta_T1 	= DEFAULT_D_TEMP,
			delta_T2 	= DEFAULT_D_TEMP,
			curValue 	= 0;

//----------------------------------------------------------------
void BlinkScreen(uint8_t cnt=1) {
	for(int i=0; i<cnt; i++) {
		delay(BLINK_DELAY);
		lc.setIntensity(0, 0); 				
		delay(BLINK_DELAY);
		lc.setIntensity(0, LED_INTENSITY); 			
	}
}
//----------------------------------------------------------------
float ReadFloatVal(const uint16_t ADR, float min_val, float max_val) {
	float val;
	eeprom_busy_wait();
	val = eeprom_read_word((const uint16_t *)ADR);
	val = val / 1000;
	if ( val < min_val ) val = min_val;
	if ( val > max_val ) val = max_val;
	return val;
}
//----------------------------------------------------------------
float WriteFloatVal(const uint16_t ADR, float val) {
	eeprom_busy_wait();
	eeprom_write_word((uint16_t *)ADR, (uint16_t)(val*1000)); 
}
//----------------------------------------------------------------
void PrintCalibrationValues(CalibrationPoint* cal){
	if( cal->state) Serial << cal->actValue << '\t' << cal->refValue;
	else Serial << F("disabled");
}
//----------------------------------------------------------------
void setup() { 
	wdt_enable (WDTO_8S); 
	Wire.begin();
	Serial.begin(115200);
	//wake up the MAX72XX from power-saving mode 
	lc.shutdown(0, false);
	//set a medium brightness for the Leds
	lc.setIntensity(0, LED_INTENSITY);
	lc.clearDisplay(0);
	
	target_pH1 = ReadFloatVal(ADR_TARGET_PH1, MIN_PH_SCALE, MAX_PH_SCALE);
	
	delta_pH1 = ReadFloatVal(ADR_TARGET_D_PH1, 0, 1);
	
	target_pH2 = ReadFloatVal(ADR_TARGET_PH2, MIN_PH_SCALE, MAX_PH_SCALE);
	
	delta_pH2 = ReadFloatVal(ADR_TARGET_D_PH2, 0, 1);

	// locate devices on the bus
	Serial << F("Locating 1-wire devices...") << endl << F("Found ");
	tmpi = sensors.getDeviceCount();
	Serial << tmpi << F(" devices.") << endl;
	if (tmpi < 2) {
		Serial << F("Error - not found two 1-wire sensors.") << endl;
		ERROR |= ERROR_T1;
		ERROR |= ERROR_T2;
	} else {
		 sensors.setResolution(TEMPERATURE_PRECISION);
	}		

	objT1.init(&sensors, 0, T_CALIBRATE_POINTS, ADR_T1_CALIBRATE);
		
	objT2.init(&sensors, 1, T_CALIBRATE_POINTS, ADR_T2_CALIBRATE);
	
	objpH1.init(PH_CALIBRATE_POINTS, ADR_PH1_CALIBRATE);
	objpH1.useADS1110(pH1_I2C_adr, &Wire);
	
	objpH2.init(PH_CALIBRATE_POINTS, ADR_PH2_CALIBRATE);
	objpH2.useADS1110(pH2_I2C_adr, &Wire);
		
	// Setup the button
	pinMode(BUTTON_PIN, INPUT);
	
	pinMode(VALVE1_PIN, OUTPUT);
	pinMode(VALVE2_PIN, OUTPUT);
	
	pinMode(COOL1_PIN, OUTPUT);
	pinMode(HEAT1_PIN, OUTPUT);

	pinMode(COOL2_PIN, OUTPUT);
	pinMode(HEAT2_PIN, OUTPUT);
	
	Serial << F("Self test") << endl;
	Serial << F("Test display ");
	for (int i=0 ; i<8; i++) {
		lc.setChar(0, i, '8', true);
		Serial << '8';
		delay(100);
		wdt_reset();
	}
	Serial << endl;
	
	delay(TEST_DELAY);
	lc.clearDisplay(0);
	Serial << F("Test outputs ");
	Write4Char(G1, "0000"); Write4Char(G2, "0000"); 
	BlinkScreen(3);
	
	Serial << F(" VALVE1=ON");	writeOutput(VALVE1_PIN, HIGH);	delay(TEST_DELAY); wdt_reset(); 
	Write4Char(G2, "0001"); 
	Serial << F(" VALVE1=OFF");	writeOutput(VALVE1_PIN, LOW);	delay(TEST_DELAY); wdt_reset(); 
	Write4Char(G2, "0000"); 
	
	Serial << F(" VALVE2=ON");	writeOutput(VALVE2_PIN, HIGH);	delay(TEST_DELAY); wdt_reset();
	Write4Char(G2, "0010"); 
	Serial << F(" VALVE2=OFF");	writeOutput(VALVE2_PIN, LOW);	delay(TEST_DELAY); wdt_reset();
	Write4Char(G2, "0000"); 
	
	Serial << F(" COOL1=ON");	writeOutput(COOL1_PIN, HIGH);	delay(TEST_DELAY); wdt_reset(); 
	Write4Char(G2, "0100"); 
	Serial << F(" COOL1=OFF");	writeOutput(COOL1_PIN, LOW);	delay(TEST_DELAY); wdt_reset(); 
	Write4Char(G2, "0000"); 
	
	Serial << F(" HEAT1=ON");	writeOutput(HEAT1_PIN, HIGH);	delay(TEST_DELAY); wdt_reset(); 
	Write4Char(G2, "1000"); 
	Serial << F(" HEAT1=OFF");	writeOutput(HEAT1_PIN, LOW);	delay(TEST_DELAY); wdt_reset(); 
	Write4Char(G2, "0000"); 
	
	Serial << F(" COOL2=ON");	writeOutput(COOL2_PIN, HIGH);	delay(TEST_DELAY); wdt_reset(); 
	Write4Char(G1, "0001"); 
	Serial << F(" COOL2=OFF");	writeOutput(COOL2_PIN, LOW);	delay(TEST_DELAY); wdt_reset(); 
	Write4Char(G1, "0000"); 
	
	Serial << F(" HEAT2=ON");	writeOutput(HEAT2_PIN, HIGH);	delay(TEST_DELAY); wdt_reset(); 
	Write4Char(G1, "0010"); 
	Serial << F(" HEAT2=OFF");	writeOutput(HEAT2_PIN, LOW);	delay(TEST_DELAY); wdt_reset(); 
	Write4Char(G1, "0000"); 
	
	Serial << endl << F("Test complete.") << endl;
	lc.clearDisplay(0);
	
	Serial << F("Calibration values:") << endl;
	calPoint = objpH1.readCalibrationPoint(0);
	Serial << F("PH1 p1: "); PrintCalibrationValues(&calPoint);
	calPoint = objpH1.readCalibrationPoint(1);
	Serial << F("\tp2: "); PrintCalibrationValues(&calPoint);
	Serial << endl;
	
	calPoint = objpH2.readCalibrationPoint(0);
	Serial << F("PH2 p1: "); PrintCalibrationValues(&calPoint);
	calPoint = objpH2.readCalibrationPoint(1);
	Serial << F("\t p2: "); PrintCalibrationValues(&calPoint);
    Serial << endl;

	calPoint = objT1.readCalibrationPoint(0);
	Serial << F("T1 p1: "); PrintCalibrationValues(&calPoint);
	calPoint = objT1.readCalibrationPoint(1);
	Serial << F("\t p2: "); PrintCalibrationValues(&calPoint);
  Serial << endl;	
	
	calPoint = objT2.readCalibrationPoint(0);
	Serial << F("T2 p1: "); PrintCalibrationValues(&calPoint);
	calPoint = objT2.readCalibrationPoint(1);
	Serial << F("\t p2: "); PrintCalibrationValues(&calPoint);
  Serial << endl;	
	
	uint16_t val;
	
	/*Serial << "RAW CAL:" << '\t';
	eeprom_busy_wait();
	val = eeprom_read_word((const uint16_t *)1); 
	Serial << val << '\t';
	val = eeprom_read_word((const uint16_t *)3); 
	Serial << val << '\t';
	
	val = eeprom_read_word((const uint16_t *)5); 
	Serial << val << '\t';
	val = eeprom_read_word((const uint16_t *)7); 
	Serial << val << '\t';
	
	Serial << endl;
	
	val = eeprom_read_word((const uint16_t *)ADR_PH2_CALIBRATE+0); 
	Serial << val << '\t';
	val = eeprom_read_word((const uint16_t *)ADR_PH2_CALIBRATE+2); 
	Serial << val << '\t';
	
	val = eeprom_read_word((const uint16_t *)ADR_PH2_CALIBRATE+4); 
	Serial << val << '\t';
	val = eeprom_read_word((const uint16_t *)ADR_PH2_CALIBRATE+6); 
	Serial << val << '\t';
	*/
	
	MODE = MODE_WORK;
	
	calPoint.state = true;
}
//---------------------------------------------------------------------------
//режим будет сменен
void BeforeChangeMode(uint8_t oldMode, uint8_t newMode) {
	//выход из режима - сохранение сделанных настроек
	switch (newMode){ 
		case MODE_CAL_PH1_1:
			calPoint = objpH1.readCalibrationPoint(0);
			break;
		case MODE_CAL_PH1_2:
			calPoint = objpH1.readCalibrationPoint(1);
			break;
		case MODE_CAL_PH2_1:
			calPoint = objpH2.readCalibrationPoint(0);
			break;
		case MODE_CAL_PH2_2:
			calPoint = objpH2.readCalibrationPoint(1);
			break;
		case MODE_CAL_T1_1:
			calPoint = objT1.readCalibrationPoint(0);
			break;
		case MODE_CAL_T1_2:
			calPoint = objT1.readCalibrationPoint(1);
			break;
		case MODE_CAL_T2_1:
			calPoint = objT2.readCalibrationPoint(0);
			break;
		case MODE_CAL_T2_2:
			calPoint = objT2.readCalibrationPoint(1);
			break;
	}
	
	curValue = calPoint.refValue;
	
	switch (newMode) { 
		case MODE_SET_PH1:	
			curValue = target_pH1; 
			break;	
		case MODE_SET_D_PH1: 
			curValue = delta_pH1; 
			break;
		case MODE_SET_PH2:	
			curValue = target_pH2; 
			break;	
		case MODE_SET_D_PH2: 
			curValue = delta_pH2; 
			break;
	}		
}
//---------------------------------------------------------------------------
void DecreaseCurValue(void) {
	switch (MODE){ 
		case MODE_SET_PH1:	
		case MODE_SET_PH2:  
		case MODE_CAL_PH1_1:
		case MODE_CAL_PH1_2:
		case MODE_CAL_PH2_1:
		case MODE_CAL_PH2_2:
			if ( curValue > MIN_PH_SCALE ) curValue -= 0.01;
			Serial << F("Decrease pH curval, ") << curValue << endl;
			break;
			
		case MODE_SET_D_PH1:
		case MODE_SET_D_PH2:
			if ( curValue > 0.01 ) curValue -= 0.01;
			break;
			
		//Для температуры минимум 18.0
		case MODE_CAL_T1_1:
		case MODE_CAL_T1_2:
		case MODE_CAL_T2_1:
		case MODE_CAL_T2_2:
			if ( curValue > MIN_T_SCALE ) curValue -= 0.1;
			Serial << F("Decrease T curval, ") << curValue << endl;
			break;
	}
}
//---------------------------------------------------------------------------
void IncreaseCurValue(void) {
	switch (MODE){ 
		case MODE_SET_PH1:	
		case MODE_SET_PH2:  
		case MODE_CAL_PH1_1:
		case MODE_CAL_PH1_2:
		case MODE_CAL_PH2_1:
		case MODE_CAL_PH2_2:
			if ( curValue < MAX_PH_SCALE ) curValue += 0.01;
			Serial << F("Increase pH curval, ") << curValue  << endl;
			break;
			
		case MODE_SET_D_PH1:
		case MODE_SET_D_PH2:
			if ( curValue < 1 ) curValue += 0.01;
			break;
			
		//Для температуры максимум 33.0
		case MODE_CAL_T1_1:
		case MODE_CAL_T1_2:
		case MODE_CAL_T2_1:
		case MODE_CAL_T2_2:
			if ( curValue < MAX_T_SCALE ) curValue += 0.1;
			Serial << F("Increase T curval, ") << curValue << endl;
			break;
	}
}
//---------------------------------------------------------------------------
//Сменили режим
void SaveSettings(void){
	
	calPoint.refValue = curValue;
	calPoint.state = true;
	
	switch (MODE){
	
		case MODE_SET_PH1:	
			target_pH1 = curValue;
			WriteFloatVal(ADR_TARGET_PH1, target_pH1);
			break;
		case MODE_SET_D_PH1:	
			delta_pH1 = curValue;
			WriteFloatVal(ADR_TARGET_D_PH1, delta_pH1);
			break;	
			
		case MODE_SET_PH2:	
			target_pH2 = curValue;
			WriteFloatVal(ADR_TARGET_PH2, target_pH2);
			break;
		case MODE_SET_D_PH2:	
			delta_pH2 = curValue;
			WriteFloatVal(ADR_TARGET_D_PH2, delta_pH2);
			break;	
		
		case MODE_SET_T1:	
			target_T1 = curValue;
			WriteFloatVal(ADR_TARGET_T1, target_T1);
			break;
		case MODE_SET_D_T1:	
			delta_T1 = curValue;
			WriteFloatVal(ADR_TARGET_D_T1, delta_T1);
			break;	
			
		case MODE_SET_T2:	
			target_T2 = curValue;
			WriteFloatVal(ADR_TARGET_T2, target_T2);
			break;
		case MODE_SET_D_T2:	
			delta_T2 = curValue;
			WriteFloatVal(ADR_TARGET_D_T2, delta_T2);
			break;	
			
		case MODE_CAL_PH1_1:
			//запишем калибровочную точку
			calPoint.actValue = objpH1.getPH(T1, false); // измеряем pH без компенсации
			objpH1.calibration(0, &calPoint);
			break;
		case MODE_CAL_PH1_2:
			calPoint.actValue = objpH1.getPH(T1, false); // измеряем pH без компенсации
			objpH1.calibration(1, &calPoint);
			break;
		case MODE_CAL_PH2_1:
			calPoint.actValue = objpH2.getPH(T2, false); // измеряем pH без компенсации
			objpH2.calibration(0, &calPoint);
			break;
		case MODE_CAL_PH2_2:
			calPoint.actValue = objpH2.getPH(T2, false); // измеряем pH без компенсации
			objpH2.calibration(1, &calPoint);
			break;
			
		case MODE_CAL_T1_1:
			calPoint.actValue = objT1.getTemp(false); 
			objT1.calibration(0, &calPoint);
			break;
		case MODE_CAL_T1_2:
			calPoint.actValue = objT1.getTemp(false); 
			objT1.calibration(1, &calPoint);
			break;		
		case MODE_CAL_T2_1:
			calPoint.actValue = objT2.getTemp(false); 
			objT2.calibration(0, &calPoint);
			break;
		case MODE_CAL_T2_2:
			calPoint.actValue = objT2.getTemp(false); 
			objT2.calibration(1, &calPoint);
			break;
	}
	Serial << F("Save seting") << endl;
	//Мигнем экраном 3 раза в знак сохранения параметров
	BlinkScreen(3);	
}
//---------------------------------------------------------------------------
unsigned long btnModeTime = 0, btnMinusTime = 0, btnPlusTime = 0;
unsigned long btnModeTimeRepeate = 0, btnMinusTimeRepeate = 0, btnPlusTimeRepeate = 0;
unsigned long btnOnTimer = 0;
void ProcessBTN(bool p_pool = false) {
	static uint8_t btnModeState, btnMinusState, btnPlusState, tmpi;
	static uint16_t	btn;
	
	btn = analogRead(BUTTON_PIN); 
	delay(1); 
	btn = analogRead(BUTTON_PIN); 
	delay(1); 
	btn = analogRead(BUTTON_PIN);
	
	btnModeState  = 0; 
	btnMinusState = 0; 
	btnPlusState  = 0; 
	
	if ( btn<10 ) { //0 
		btnModeState  = 1;
		btnOnTimer = curMillis;
	} else 	if ( btn > 490 && btn < 530 ) { //510
				btnPlusState  = 1;
				btnOnTimer = curMillis;
			} else 	if ( btn > 660 && btn < 700 ) { //680
						btnMinusState  = 1;
						btnOnTimer = curMillis;
					}
			
	if (!p_pool) { 
		//Отработаем кнопку режима
		//нажата кнопка
		if (btnModeState && btnModeTime == 0) { //только нажали (до этого был отпущен)
			//Кнопка точно нажата, до этого была отпущена
			btnModeTime = btnModeTimeRepeate = curMillis;
		} 	
	
		//отпустили кнопку
		if (!btnModeState && btnModeTime > 0 ) {//отпустили батон режима
			if ( curMillis - btnModeTime < WORK_MODE_TIME ) { //держали мало, переключаем режим на следующий
				//Переключаем режим на следующий
				tmpi = ( MODE >= MODE_MAX ) ? MODE_MIN : MODE+1;
				BeforeChangeMode(MODE, tmpi);
				MODE = tmpi;			
				Serial << F("MODE=") << MODE << endl;
			}
			if ( curMillis - btnModeTime >= WORK_MODE_TIME && curMillis - btnModeTime < SAVE_CALIBRATION_TIME) { // держали от 2х до 5ти - выход к рабочему режиму
				BeforeChangeMode(MODE, MODE_WORK);
				MODE = MODE_WORK;
			}
			if ( curMillis - btnModeTime >= SAVE_CALIBRATION_TIME) { // держали более 5ти - сохранить параметр
				SaveSettings();			
			}
			btnModeTime = 0; //обнулим таймер кнопки режима
		}
	
		//Обработаем минус
		//нажата кнопка -
		if (btnMinusState && btnMinusTime == 0) { //только нажали (до этого был отпущен)
			//Кнопка точно нажата, до этого была отпущена
			btnMinusTimeRepeate = btnMinusTime = curMillis;
			//= curMillis;
			DecreaseCurValue();
		} 
	
		//Обработаем плюс
		//нажата кнопка +
		if (btnPlusState && btnPlusTime == 0) { //только нажали (до этого был отпущен)
			//Кнопка точно нажата, до этого была отпущена
			btnPlusTimeRepeate = btnPlusTime = curMillis;
			//= curMillis;
			IncreaseCurValue();
		} 
		
		//держится кнопка -
		if (btnMinusState && btnMinusTime > 0) { 
			if ( curMillis - btnMinusTime > BUTTON_VALUE_DELAY_TO_REPEAT ) { //подождали до повтора
				if ( curMillis - btnMinusTimeRepeate > BUTTON_VALUE_REPEAT_TIME ) // зареган повтор
					btnMinusTimeRepeate = curMillis;
					DecreaseCurValue();
			}		
		}
		
		//держится кнопка +
		if (btnPlusState && btnPlusTime > 0) { 
			if ( curMillis - btnPlusTime > BUTTON_VALUE_DELAY_TO_REPEAT ) { //подождали до повтора
				if ( curMillis - btnPlusTimeRepeate > BUTTON_VALUE_REPEAT_TIME ) // зареган повтор
					btnPlusTimeRepeate = curMillis;
					IncreaseCurValue();
			}		
		}
		
		//Отпустили кнопку
		if ( !btnMinusState && btnMinusTime > 0 )
			btnMinusTime = 0;
		if ( !btnPlusState && btnPlusTime > 0 )
			btnPlusTime = 0;
		if ( !btnModeState && btnModeTime > 0 )
			btnModeTime = 0;
		
		if (   btnPlusTime==0 
			&& btnMinusTime==0 
			&& btnModeTime==0 
			&& MODE != MODE_WORK 
			&& curMillis - btnOnTimer > TUNE_TIMEOUT 
			&& btnOnTimer > 0 ) {
				btnOnTimer = 0;
				MODE = MODE_WORK;
		}			
	} else {	//Пуллинг, ежесекундный вызов
		//держится кнопка
		if (btnModeState && btnModeTime > 0) { 
			//Мигнем экраном - кнопка зажата
			BlinkScreen();
			/*if ( curMillis - btnModeTimeRepeate > BUTTON_MODE_REPEAT_TIME + SAVE_CALIBRATION_TIME ) {
				//Переключаем режим на следующий
				tmpi = ( MODE >= MODE_MAX ) ? MODE_MIN : MODE+1;
				BeforeChangeMode(MODE, tmpi);
				MODE = tmpi;			
				Serial << F("MODE(pooling)=") << MODE << endl;				
			}*/
		}		
	}
}
//-------------------------------------------------------------
void Write4Char(uint8_t num, char str[]) {//num = 0 первый квартет, num = 1 второй квартет
	lc.setChar(0, 0+num*4, str[3], false); delay(DISP_DELAY);
	lc.setChar(0, 1+num*4, str[2], false); delay(DISP_DELAY);
	lc.setChar(0, 2+num*4, str[1], false); delay(DISP_DELAY);
	lc.setChar(0, 3+num*4, str[0], false); delay(DISP_DELAY);
}
//-------------------------------------------------------------
char tmps[9];
void Write3Digit_1_2(uint8_t num, char ch, float f, bool point1 = false) {
	dtostrf(f, 4, 2, tmps); //#.##
	lc.setChar(0, 3+num*4, ch, point1);		delay(DISP_DELAY);
	lc.setChar(0, 2+num*4, tmps[0], true);	delay(DISP_DELAY);
	lc.setChar(0, 1+num*4, tmps[2], false);	delay(DISP_DELAY);
	lc.setChar(0, 0+num*4, tmps[3], false);	delay(DISP_DELAY);
}
//-------------------------------------------------------------
void Write3Digit_2_1(uint8_t num, char ch, float f, bool point1 = false) {
	dtostrf(f, 4, 1, tmps); //##.#
	lc.setChar(0, 3+num*4, ch, point1);		delay(DISP_DELAY);
	lc.setChar(0, 2+num*4, tmps[0], false);	delay(DISP_DELAY);
	lc.setChar(0, 1+num*4, tmps[1], true);	delay(DISP_DELAY);
	lc.setChar(0, 0+num*4, tmps[3], false);	delay(DISP_DELAY);
}
//-------------------------------------------------------------
void Write4Digit(uint8_t num, float f) {//num = 0 первый квартет, num = 1 второй квартет
	dtostrf(f, 5, 2, tmps);
	lc.setChar(0, 3+num*4, tmps[0], false);	delay(DISP_DELAY);
	lc.setChar(0, 2+num*4, tmps[1], true);	delay(DISP_DELAY);
	lc.setChar(0, 1+num*4, tmps[3], false);	delay(DISP_DELAY);
	lc.setChar(0, 0+num*4, tmps[4], false);	delay(DISP_DELAY);
}
//-------------------------------------------------------------
void Write4Char4Digit(char str[], float f){
	Write4Char (G1, str);
	Write4Digit(G2, f);
}
//-------------------------------------------------------------
void Write2Char4Digit(char str[], float f1, float f2){
	//Write2Digit(G1, str, f1);
	dtostrf(f1, 2, 0, tmps); //##
	lc.setChar(0, 7, str[0], false);
	lc.setChar(0, 6, str[1], true);
	lc.setChar(0, 5, tmps[0], false);
	lc.setChar(0, 4, tmps[1], false);
	Write4Digit(G2, f2);
}
//-------------------------------------------------------------
unsigned long displayTime = 0;
void ProcessDisplay(void){
	if (curMillis - displayTime >= WORK_DISPLAY_INT + WORK_DISPLAY_INT)
		displayTime = curMillis;
					
	switch (MODE){
		case MODE_WORK:		// ph: -0.00 -0.00 / t: 00.00 00.00 (5 сек pH / 5 сек температура)
			if ( curMillis - displayTime < WORK_DISPLAY_INT ) {
				//Выводим кислотность
				if (ERROR&ERROR_PH1) Write4Char(G1, " -Ph");
				else 	if ( readOutput(VALVE1_PIN)) 
								Write3Digit_1_2(G1, '_', pH1, false);
							else 
								Write3Digit_1_2(G1, ' ', pH1, false);

				if (ERROR&ERROR_PH2) Write4Char(G2, " -Ph");
				else 	if ( readOutput(VALVE2_PIN) ) 
								Write3Digit_1_2(G2, '_', pH2, false);
							else 
								Write3Digit_1_2(G2, ' ', pH2, false);					
			} else {
				//Выводим температуру
				if (ERROR&ERROR_T1) Write4Char(G1, "--c ");
				else if ( readOutput(COOL1_PIN) ) Write3Digit_2_1(G1, '_', T1, false);
						else if ( readOutput(HEAT1_PIN) ) Write3Digit_2_1(G1, '.', T1, false);
								else Write3Digit_2_1(G1, ' ', T1, false);
								
				if (ERROR&ERROR_T2) Write4Char(G2, "--c ");
				else if ( readOutput(COOL1_PIN) ) Write3Digit_2_1(G2, '_', T2, false);
						else if ( readOutput(HEAT1_PIN) ) Write3Digit_2_1(G2, '.', T2, false);
								else Write3Digit_2_1(G2, ' ', T2, false);
			}
			break;
			
		case MODE_SET_PH1:		Write4Char4Digit("PH1 ", curValue);	break;
		case MODE_SET_D_PH1:	Write4Char4Digit("dPH1", curValue);	break;	
		case MODE_SET_PH2:		Write4Char4Digit("PH2 ", curValue);	break;	
		case MODE_SET_D_PH2:	Write4Char4Digit("dPH2", curValue);	break;	
		
		case MODE_SET_T1:			Write4Char4Digit(" c1 ", curValue);	break;
		case MODE_SET_D_T1:		Write4Char4Digit("dc1 ", curValue);	break;	
		case MODE_SET_T2:			Write4Char4Digit(" c2 ", curValue);	break;	
		case MODE_SET_D_T2:		Write4Char4Digit("dc2 ", curValue);	break;	
			
		case MODE_CAL_PH1_1:	Write2Char4Digit("11", T1, curValue);	break;
		case MODE_CAL_PH1_2:  Write2Char4Digit("12", T1, curValue);	break;
		case MODE_CAL_PH2_1:	Write2Char4Digit("21", T2, curValue);	break;
		case MODE_CAL_PH2_2:  Write2Char4Digit("22", T2, curValue);	break;
		
		case MODE_CAL_T1_1:		Write4Char4Digit("c 11", curValue);	break;
		case MODE_CAL_T1_2:		Write4Char4Digit("c 12", curValue);	break;
		case MODE_CAL_T2_1:		Write4Char4Digit("c 21", curValue);	break;
		case MODE_CAL_T2_2:		Write4Char4Digit("c 22", curValue);	break;
	}	
}
//-------------------------------------------------------------
void writeOutput(uint8_t pin, bool state){
	out[pin-4]=state;
	digitalWrite(pin, state);
}
//-------------------------------------------------------------
bool readOutput(uint8_t pin){
	return out[pin-4];	
}
//-------------------------------------------------------------
void Regulator(float val, float target_val, float delta, uint8_t dec_pin, uint8_t inc_pin) {
	if (val > target_val+delta && !readOutput(dec_pin) /*&& dec_pin*/)  
		writeOutput(dec_pin, HIGH);
	if (val <= target_val && readOutput(dec_pin)/* && dec_pin*/) 
		writeOutput(dec_pin, LOW);	
		
	if (val < target_val-delta && readOutput(inc_pin)==LOW && inc_pin)  
		writeOutput(inc_pin, HIGH);					
	if (val >= target_val	&&	readOutput(inc_pin)==HIGH && inc_pin) 
		writeOutput(inc_pin, LOW);	
}
//-------------------------------------------------------------
void loop() { 
	wdt_reset(); // не забываем сбросить смотрящую собаку
	
	curMillis = millis();		

	ProcessBTN();
	ProcessDisplay();	
	
	if(curMillis - prevMillis > POOL_INT){
		prevMillis = curMillis;
		
		ProcessBTN(true);
		
		ERROR = 0;
		
		sensors.requestTemperatures();
		
		T1 = objT1.getTemp(1);
		if( T1>99 || T1<10 || T1 == DEVICE_DISCONNECTED ) { 
			ERROR |= ERROR_T1;
			T1 = DEFAULT_TEMP;
		}
		
		T2 = objT2.getTemp(1);
		if( T2>99 || T2<10 || T2 == DEVICE_DISCONNECTED ) { 
			ERROR |= ERROR_T2;
			T2 = DEFAULT_TEMP;
		}
		
		pH1 = objpH1.getPH(T1);
		if (pH1>9.99 || pH1<0) ERROR |= ERROR_PH1;
		
		pH2 = objpH2.getPH(T2);
		if (pH2>9.99 || pH2<0) ERROR |= ERROR_PH2;
		
		Serial << F("MODE=")	<< MODE << F("\t\t");
		//Вывод в виде: [чистое значение, без калибровки]=>[значение с учетом калибровки]
		Serial << F("T1=")  	<< objT1.getTemp(false) << "=>" << T1 << F("\t");
		Serial << F("pH1=") 	<< objpH1.getPH(T1, false) << "=>" << pH1 << F("\t\t");
		Serial << F("T2=")  	<< objT2.getTemp(false) << "=>" << T2 << F("\t");
		Serial << F("pH2=") 	<< objpH2.getPH(T2, false) << "=>" << pH2;
		Serial << endl;
		
		if (MODE == MODE_WORK) {
			if (ERROR&ERROR_PH1 ) //Ошибка температуры отключит компенсацию по температуре и уменьшит точность определения РH
				writeOutput(VALVE1_PIN, LOW);	
			else 
				Regulator(pH1, target_pH1, delta_pH1, VALVE1_PIN, 0);
			
			if (ERROR&ERROR_PH2 ) 
				writeOutput(VALVE2_PIN, LOW);	
			else 
				Regulator(pH2, target_pH2, delta_pH2, VALVE2_PIN, 0);
			
			if (ERROR&ERROR_T1) { 
				writeOutput(COOL1_PIN, LOW);
				writeOutput(HEAT1_PIN, LOW);				
			} else Regulator(T1, target_T1, delta_T1, COOL1_PIN, HEAT1_PIN);
			
			if (ERROR&ERROR_T2) { 
				writeOutput(COOL2_PIN, LOW);
				writeOutput(HEAT2_PIN, LOW);				
			} else Regulator(T2, target_T2, delta_T2, COOL2_PIN, HEAT2_PIN);
		}
	}
}
