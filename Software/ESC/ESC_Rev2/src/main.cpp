#include <SPI.h>
#include <Arduino.h>
#include <stdint.h>
#include <motor_control.h>

#define I2C_Clock 8000000

void initialization();
void DriveMotor(int _drive,int _power);
void SD_HIGH();
void calibration();

bool stop_flag = 0; //0:回転可 1:強制停止
bool turn = 1;
uint8_t power = 0;
uint16_t offset = 206;
uint16_t shinkaku [2]= {50,300};//170 //495
uint16_t drive = 0;

void setup() {
    initialization();
    for (uint16_t i = 0;i < 360;i++) {
      pwm[i] = sin(radians(i));
    }
    SD_HIGH();
    calibration();
}

void loop() {
  if (stop_flag) {
    Stop();
  }
  else {
    if (turn) {
      uint16_t Angle = EncoderRead();
      Angle += 4096 * 2;
      Angle -= offset;
      Angle %= 4096;
      drive = Angle % 585;
      drive = map(drive,0,587,0,359);
      drive += 360 * 2;
      drive -= shinkaku[0];
      drive %= 360;
      DriveMotor(drive,power);
    }
    else {
      uint16_t Angle = EncoderRead();
      Angle += 4096 * 2;
      Angle -= offset;
      Angle %= 4096;
      drive = Angle % 585;
      drive = map(drive,0,587,0,359);
      drive += 360 * 2;
      drive -= shinkaku[1];
      drive %= 360;
      DriveMotor(drive,power);
    }
  }
}

void calibration() {
  uint16_t _Angle[4] = {0,0,0,0};
  for (uint8_t i = 0;i < 4;i++) {
    for (uint16_t j = 0;j < 360;j++) {
      DriveMotor(j,50);
      delayMicroseconds(10);
    }
    DriveMotor(0,50);
    delay(300 * 64);
    _Angle[i] = EncoderRead();
    delay(500 * 64);
  }
  offset = (uint16_t)((_Angle[0] + _Angle[1] + _Angle[2] + _Angle[3]) / 4.0);
}

void initialization() {
    //pwm
    TCCR0B = (TCCR0B & 0b11111000) | 0x01;  // 31.37255 [kHz]
    TCCR2B = (TCCR2B & 0b11111000) | 0x01;  // 31.37255 [kHz] //Arduino Uno
    /*TCCR2B = (TCCR2B & 0b11111000) | 0x01;  // 31.37255 [kHz]
    TCCR3B = (TCCR3B & 0b11111000) | 0x01;  // 31.37255 [kHz]
    TCCR4B = (TCCR4B & 0b11111000) | 0x01;*/  // 31.37255 [kHz] //Arduino Mega

    //I2C
    Wire.begin();
    Wire.setClock(I2C_Clock);

    Serial.begin(115200);

    //タイマー割込み
    //絶対消さない!!!!!
    /*TCCR5A  = 0;
    TCCR5B  = 0;
    TCCR5B |= (1 << WGM52) | (1 << CS52);  //CTCmode //prescaler to 256
    OCR5A   = 62500-1;
    OCR5B   = 6250-1;*/

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
    delay(300 * 64);
    //TIMSK5 |= (1 << OCIE5A); //割り込みA開始
}

ISR (SPI_STC_vect) {
  byte data = SPDR;
  switch (data) {
    case 255:
      if (stop_flag) {
        stop_flag = false;
      }
      data = SPDR;
      power = data;
      turn = true;
      for (uint16_t i = 359;i >= 0;i--) {
        DriveMotor(i,power);
        delayMicroseconds(8);
      }
      break;
  
    case 254:
      if (stop_flag) {
        stop_flag = false;
      }
      data = SPDR;
      power = data;
      turn = false;
      for (uint16_t i = 0;i < 360;i++) {
        DriveMotor(i,power);
        delayMicroseconds(8);
      }
      break;
  
    case 253:
      stop_flag = true;
      break;
  
    default:
      break;
  }
}

/*ISR (TIMER5_COMPA_vect) {
  TIMSK5 |= (1 << OCIE5B);
}

ISR (TIMER5_COMPB_vect) {
  TIMSK5 |= (0 << OCIE5B);
}*/