#ifndef CalibrationPoint_h
#define CalibrationPoint_h
typedef struct {
  bool state;			//состояние точки калибровки =0 - не используется/не инициализировано, =1 - используется/инициализировано	
  float refValue; 		//значение показателя (образцовое)
  float actValue;		//измеренное значение с датчика
} CalibrationPoint;
#endif