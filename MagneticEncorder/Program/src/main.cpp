#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>

#define AS5600_DEV_ADDRESS   0x36
#define AS5600_REG_RAW_ANGLE 0x0C
#define I2C_Clock 8000000

const int BLDC[3][2] = {{3,4},{5,6},{7,8}};
const uint16_t offset = 419;

uint8_t power = 50;
uint8_t turn = 0; //0 正転 / 1 逆転

void stop_motor();
uint16_t adjust_angle(uint16_t _Angle);
uint8_t judge_step(uint16_t _Angle,uint8_t _turn);
void initialization();
uint16_t EncoderRead();
//uint16_t ElectricAngle(uint16_t RawAngle);
uint16_t calibration();
void motor_control(uint8_t _step,uint8_t _power);

void setup() {
  // put your setup code here, to run once:
  initialization();
  //offset = calibration();
  delay(500);
  for (uint8_t i = 6;i > 0;i--) {
    motor_control(i % 6,30);
    delay(100);
  }
}

void loop() {
  uint16_t Angle = EncoderRead();
  Angle = adjust_angle(Angle);
  uint8_t step = judge_step(Angle,turn);
  motor_control(step,power);
}

void stop_motor() {
  analogWrite(BLDC[0][0],0);
  analogWrite(BLDC[1][0],0);
  analogWrite(BLDC[2][0],0);

  digitalWrite(BLDC[0][1],HIGH);
  digitalWrite(BLDC[1][1],HIGH);
  digitalWrite(BLDC[2][1],HIGH);
}

uint16_t adjust_angle(uint16_t _Angle) {
  _Angle -= offset;
  _Angle += 4096 * 2;
  _Angle %= 4096;
  _Angle %= 585;
  return _Angle;
}

uint8_t judge_step(uint16_t _Angle,uint8_t _turn) {
  uint8_t _step;
  if (_turn == 0) {
    if (_Angle <= 48 || _Angle >= 535) {
      _step = 2;
    }
    else if (_Angle >= 439 && _Angle <= 536) {
      _step = 3;
    }
    else if (_Angle >= 342 && _Angle <= 438) {
      _step = 4;
    }
    else if (_Angle >= 244 && _Angle <= 341) {
      _step = 5;
    }
    else if (_Angle >= 147 && _Angle <= 243) {
      _step = 0;
    }
    else if (_Angle >= 49 && _Angle <= 146) {
      _step = 1;
    }
  }
  else {
    if (_Angle <= 48 || _Angle >= 535) {
      _step = 4;
    }
    else if (_Angle >= 439 && _Angle <= 536) {
      _step = 5;
    }
    else if (_Angle >= 342 && _Angle <= 438) {
      _step = 0;
    }
    else if (_Angle >= 244 && _Angle <= 341) {
      _step = 1;
    }
    else if (_Angle >= 147 && _Angle <= 243) {
      _step = 2;
    }
    else if (_Angle >= 49 && _Angle <= 146) {
      _step = 3;
    }
  }
  return _step;
}

void initialization() {
  //pwm
  TCCR2B = (TCCR2B & 0b11111000) | 0x01;  // 31.37255 [kHz]
  TCCR3B = (TCCR3B & 0b11111000) | 0x01;  // 31.37255 [kHz]
  TCCR4B = (TCCR4B & 0b11111000) | 0x01;  // 31.37255 [kHz]

  //I2C
  Wire.begin();
  Wire.setClock(I2C_Clock);

  //タイマー割込み
  //絶対消さない!!!!!
  /*TCCR1A  = 0;
  TCCR1B  = 0;
  TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);  //CTCmode //prescaler to 256
  OCR1A   = 20000-1;
  TIMSK1 |= (1 << OCIE1A);*/

  //Serial
  Serial.begin(115200);
  
  //BLDC pin
  for (uint8_t i = 0;i < 3;i++) {
    for (uint8_t j = 0;j < 2;j++) {
      pinMode(BLDC[i][j],OUTPUT);
    }
  }
  for (uint8_t i = 0;i < 3;i++) {
    analogWrite(BLDC[i][0],0);
    digitalWrite(BLDC[i][1],HIGH);
  }
  stop_motor();
  delay(3000);
}

uint16_t EncoderRead() {
  Wire.beginTransmission(AS5600_DEV_ADDRESS);
  Wire.write(AS5600_REG_RAW_ANGLE);
  Wire.endTransmission(false);
  Wire.requestFrom(AS5600_DEV_ADDRESS,2);
  uint16_t RawAngle = 0;
  RawAngle = ((uint16_t)Wire.read() << 8) & 0x0F00;
  RawAngle |= (uint16_t)Wire.read();
  if (RawAngle > 4096) {
    RawAngle = EncoderRead();
  }
  return RawAngle;
}

/*uint16_t ElectricAngle(uint16_t RawAngle) { //0 ~ 4095
  uint16_t elecAngle = RawAngle % 585;
  elecAngle = map(elecAngle,0,584,0,359);
  return elecAngle;
}*/

uint16_t calibration(void) {
  int offsetRaw[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  /*for (uint8_t i = 6;i > 0;i--) {
    motor_control(i % 6,30);
    delay(100);
  }*/
  motor_control(1, 30);
  delay(50);
  motor_control(0, 30);
  delay(150);
  offsetRaw[0] = EncoderRead();
  motor_control(5, 30);
  delay(50);
  motor_control(0, 30);
  delay(150);
  offsetRaw[1] = EncoderRead();
  motor_control(1, 30);
  delay(50);
  motor_control(0, 30);
  delay(150);
  offsetRaw[2] = EncoderRead();
  motor_control(5, 30);
  delay(50);
  motor_control(0, 30);
  delay(150);
  offsetRaw[3] = EncoderRead();
  stop_motor();
  Serial.print(offsetRaw[0]);
  Serial.print(", ");
  Serial.print(offsetRaw[1]);
  Serial.print(", ");
  Serial.print(offsetRaw[2]);
  Serial.print(", ");
  Serial.print(offsetRaw[3]);
  Serial.println("");
  return (offsetRaw[0] + offsetRaw[1] + offsetRaw[2] + offsetRaw[3]) / 4;
}

void motor_control(uint8_t _step, uint8_t _power) {
  switch (_step) {
    case 0:  // U→V
      analogWrite(BLDC[0][0], _power);  // U_IN
      analogWrite(BLDC[1][0], 0);       // V_IN
      analogWrite(BLDC[2][0], 0);       // W_IN

      digitalWrite(BLDC[0][1], HIGH);  // V_SD
      digitalWrite(BLDC[1][1], HIGH);   // V_IN
      digitalWrite(BLDC[2][1], LOW);  // W_IN
      break;

    case 1:  // W→V
      analogWrite(BLDC[0][0], 0);
      analogWrite(BLDC[1][0], 0);
      analogWrite(BLDC[2][0], _power);

      digitalWrite(BLDC[0][1], LOW);
      digitalWrite(BLDC[1][1], HIGH);
      digitalWrite(BLDC[2][1], HIGH);
      break;

    case 2:  // W→U
      analogWrite(BLDC[0][0], 0);
      analogWrite(BLDC[1][0], 0);
      analogWrite(BLDC[2][0], _power);

      digitalWrite(BLDC[0][1], HIGH);
      digitalWrite(BLDC[1][1], LOW);
      digitalWrite(BLDC[2][1], HIGH);
      break;

    case 3:  // V→U
      analogWrite(BLDC[0][0], 0);
      analogWrite(BLDC[1][0], _power);
      analogWrite(BLDC[2][0], 0);

      digitalWrite(BLDC[0][1], HIGH);
      digitalWrite(BLDC[1][1], HIGH);
      digitalWrite(BLDC[2][1], LOW);
      break;

    case 4:  // V→W
      analogWrite(BLDC[0][0], 0);
      analogWrite(BLDC[1][0], _power);
      analogWrite(BLDC[2][0], 0);

      digitalWrite(BLDC[0][1], LOW);
      digitalWrite(BLDC[1][1], HIGH);
      digitalWrite(BLDC[2][1], HIGH);
      break;

    case 5:  // U→W
      analogWrite(BLDC[0][0], _power);
      analogWrite(BLDC[1][0], 0);
      analogWrite(BLDC[2][0], 0);

      digitalWrite(BLDC[0][1], HIGH);
      digitalWrite(BLDC[1][1], LOW);
      digitalWrite(BLDC[2][1], HIGH);
      break;
  }
}

ISR (TIMER1_COMPA_vect) {
  
}