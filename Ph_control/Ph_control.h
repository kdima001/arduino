//Экран
#define LED_INTENSITY	15
#define BLINK_DELAY		100
#define DISP_DELAY		1
#define G1 				1 //group 1
#define G2				0 //group 2

#define MIN_PH_SCALE	5
#define MAX_PH_SCALE	9

#define MIN_T_SCALE		18
#define MAX_T_SCALE		33

//константы для клапанов
#define ON_VALVE_VALUE 		255	//ШИМ на включение клапана
#define HOLD_VALVE_VALUE 	200	//ШИМ на удержание клапана

//константы интервалов в мсек
#define POOL_INT				1000		//интервал опроса и регулирования
#define WORK_DISPLAY_INT		3000	    //интервал смены показаний в обычном (рабочем) режиме
#define TUNE_DISPLAY_INT		5000		//интервал смены показаний в режиме настройки СЕКУНДЫ !!!
#define TUNE_TIMEOUT			10000		//интервал до перехода к рабочему режиму в отсутвии нажатий кнопок

#define	ERROR_PH1	B00000001	// E.9.99 -0.00
#define	ERROR_PH2	B00000100	// -0.00 E.9.99

#define	ERROR_T1	B00000010 // E.--- -0.00
#define	ERROR_T2	B00001000 // -0.00 E.---

#define MODE_MIN			1
#define MODE_MAX			17

#define MODE_WORK			1

#define MODE_SET_PH1		2
#define MODE_SET_D_PH1		3
#define MODE_SET_T1			4
#define MODE_SET_D_T1		5

#define MODE_SET_PH2		6
#define MODE_SET_D_PH2		7
#define MODE_SET_T2			8
#define MODE_SET_D_T2		9

#define MODE_CAL_PH1_1		10
#define MODE_CAL_PH1_2		11
#define MODE_CAL_PH2_1		12
#define MODE_CAL_PH2_2		13

#define MODE_CAL_T1_1		14
#define MODE_CAL_T1_2		15
#define MODE_CAL_T2_1		16
#define MODE_CAL_T2_2		17

//Кнопки
#define BUTTON_PIN		A7

#define BUTTON_MODE_REPEAT_TIME			1000
#define BUTTON_VALUE_DELAY_TO_REPEAT	2000
#define BUTTON_VALUE_REPEAT_TIME		500

#define WORK_MODE_TIME				2000
#define SAVE_CALIBRATION_TIME		5000

//пины к которым подключены CO2 клапана
#define VALVE1_PIN 4
#define VALVE2_PIN 5

//пины к которым нагреватели и охладители
#define COOL1_PIN 6
#define HEAT1_PIN 9

#define COOL2_PIN 7
#define HEAT2_PIN 8

//PH датчики
//Канал 1 (АЦП - EDO)
#define pH1_I2C_adr B1001000
//Канал 2 (АЦП - ED1)
#define pH2_I2C_adr B1001001

#define PH_CALIBRATE_POINTS    	2 //number of calibrating points 
#define PH1_CALIBRATE_ADDR   	0x01 //4 bytes for each calibration point = 2 bytes for reference value and 2 bytes for measured value (2 point calibration = 8 bytes)
#define PH2_CALIBRATE_ADDR   	0x08 //4 bytes for each calibration point = 2 bytes for reference value and 2 bytes for measured value (2 point calibration = 8 bytes)

#define ADR_TARGET_PH1			0x20 //2 bytes to store target pH value
#define ADR_TARGET_D_PH1		0x22 //2 bytes to store target delta pH value

#define ADR_TARGET_PH2			0x24
#define ADR_TARGET_D_PH2		0x26

#define ADR_TARGET_T1			0x28
#define ADR_TARGET_D_T1			0x2A

#define ADR_TARGET_T2			0x2C
#define ADR_TARGET_D_T2			0x2E

#define DEFAULT_PH				7
#define DEFAULT_D_PH			0.4

#define DEFAULT_TEMP			23
#define DEFAULT_D_TEMP			1

//Датчики температуры
#define ONE_WIRE_BUS 			2
#define TEMPERATURE_PRECISION 	9

#define T_CALIBRATE_POINTS  	2 //number of calibrating points
#define T1_CALIBRATE_ADDR 		0x10 //4 bytes for each calibration point = 2 bytes for reference value and 2 bytes for measured value (2 point calibration = 8 bytes)
#define T2_CALIBRATE_ADDR 		0x18 //4 bytes for each calibration point = 2 bytes for reference value and 2 bytes for measured value (2 point calibration = 8 bytes)

#define TEST_DELAY		500