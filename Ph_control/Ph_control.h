//Яркость экрана
#define LED_INTENSITY	15

#define MIN_PH_SCALE	6.00
#define MAX_PH_SCALE	8.00
//константы для клапанов
#define ON_VALVE_VALUE 		255	//ШИМ на включение клапана
#define HOLD_VALVE_VALUE 	200	//ШИМ на удержание клапана

//константы интервалов в мсек
#define POOL_INT						1000		//интервал опроса и регулирования
#define WORK_DISPLAY_INT		5000		//интервал смены показаний в обычном (рабочем) режиме
#define TUNE_DISPLAY_INT		500			//интервал смены показаний в режиме настройки

#define	ERROR_PH1	B00000001	// E.9.99 -0.00
#define	ERROR_PH2	B00000101	// -0.00 E.9.99

#define	ERROR_T1	B00000010 // E.--- -0.00
#define	ERROR_T2	B00001000 // -0.00 E.---

#define MODE_MIN		1
#define MODE_MAX		11

#define MODE_WORK						1

#define MODE_SET_PH1				2
#define MODE_SET_PH2				3

#define MODE_CAL_PH1_1			4
#define MODE_CAL_PH1_2			5

#define MODE_CAL_PH2_1			6
#define MODE_CAL_PH2_2			7

#define MODE_CAL_T1_1				8
#define MODE_CAL_T1_2				9

#define MODE_CAL_T2_1				10
#define MODE_CAL_T2_2				11

#define MODE_PH1_VALVE			12

#define MODE_PH1_VALVE_ON		13
#define MODE_PH1_VALVE_OFF	14

#define MODE_PH2_VALVE			15

#define MODE_PH2_VALVE_ON		16
#define MODE_PH2_VALVE_OFF	17

//Пины с кнопками
#define BUTTON_MODE_PIN		4
#define BUTTON_MINUS_PIN	5
#define BUTTON_PLUS_PIN		6

#define BUTTON_PRESSED		LOW
#define BUTTON_RELEASED		HIGH

#define BUTTON_MODE_REPEAT_TIME				1000
#define BUTTON_VALUE_DELAY_TO_REPEAT	2000
#define BUTTON_VALUE_REPEAT_TIME			300

#define WORK_MODE_TIME				2000
#define SAVE_CALIBRATION_TIME	5000

//пины к которым подключены CO2 клапана
#define VALVE1_PIN 10
#define VALVE2_PIN 11

// Data wire is plugged into port 2&3 on the Arduino
/*#define ONE_WIRE_BUS1 2 
#define ONE_WIRE_BUS2 3
#define TEMPERATURE_PRECISION 9*/

//Канал 1 (АЦП - EDO)
#define pH1_I2C_adr B1001000
//Канал 2 (АЦП - ED1)
#define pH2_I2C_adr B1001001

#define PH_CALIBRATE_POINTS    2 //number of calibrating points 
#define PH1_CALIBRATE_ADDR   0x01 //4 bytes for each calibration point = 2 bytes for reference value and 2 bytes for measured value (2 point calibration = 8 bytes)
#define PH2_CALIBRATE_ADDR   0x08 //4 bytes for each calibration point = 2 bytes for reference value and 2 bytes for measured value (2 point calibration = 8 bytes)

#define T_CALIBRATE_POINTS  2 //number of calibrating points
#define T1_DQ_PIN            2 //DQ pin
#define T1_CALIBRATE_ADDR 0x10 //4 bytes for each calibration point = 2 bytes for reference value and 2 bytes for measured value (2 point calibration = 8 bytes)

#define T2_DQ_PIN            3 //DQ pin
#define T2_CALIBRATE_ADDR 0x18 //4 bytes for each calibration point = 2 bytes for reference value and 2 bytes for measured value (2 point calibration = 8 bytes)

#define ADR_TARGET_PH1		0x20 //2 bytes to store target pH value
#define ADR_TARGET_PH2		0x22 //2 bytes to store target pH value