#include <SPI.h>
#include <Arduino.h>
#include <stdint.h>
#include "C:\Users\nkoji\Documents\GitHub\RoboCupJunior2023_Lightweight\Software\ESC\ESC_Rev2\src\motor_control.h"

#define I2C_Clock 8000000

void initialization();
void DriveMotor(int _drive,int _power);
void SD_HIGH();
uint16_t calibration();

bool stop_flag = 0; //0:回転可 1:強制停止(ブレーキ)
bool turn = 1; //1:正転(時計回り) 0:逆転(反時計回り)
uint8_t power = 0; //0から255 
uint16_t Offset = 629;//441,586,13
uint16_t shinkaku [2]= {90,90};//50 //300
uint16_t drive = 0;

void setup() {
    initialization();
    for (uint16_t i = 0;i < 360;i++) {
      pwm[i] = sin(radians(i));
    }
    SD_HIGH();
    Offset = calibration();
}

void loop() {
  while (stop_flag) {
    Stop();
  }
  
  if (turn) {
    uint16_t Angle = EncoderRead();
    Angle += 4096 * 2;
    Angle -= Offset;
    Angle %= 4096;
    drive = Angle % 585;
    drive = map(drive,0,587,0,359);
    drive += shinkaku[0];
    drive %= 360;
    DriveMotor(drive,power);
  }
  else {
    uint16_t Angle = EncoderRead();
    Angle += 4096 * 2;
    Angle -= Offset;
    Angle %= 4096;
    drive = Angle % 585;
    drive = map(drive,0,587,0,359);
    drive += 360 * 2;
    drive -= shinkaku[1];
    drive %= 360;
    DriveMotor(drive,power);
  }
}

uint16_t calibration(void) {
  uint16_t _Angle[4] = {0,0,0,0};
  for (uint8_t i = 0;i < 2;i++) {
    for (uint16_t j = 0;j < 360;j++) {
      DriveMotor(j,40);
      delayMicroseconds(10);
    }
    DriveMotor(0,40);
    delay(150 * 64);
    _Angle[i * 2] = EncoderRead();
    delay(150 * 64);
    for (uint16_t j = 359;j > 0;j--) {
      DriveMotor(j,40);
      delayMicroseconds(10);
    }
    DriveMotor(0,40);
    delay(150 * 64);
    _Angle[i * 2 + 1] = EncoderRead();
    delay(150 * 64);
  }
  uint16_t _Offset = (uint16_t)((_Angle[0] + _Angle[1] + _Angle[2] + _Angle[3]) / 4.0);
  return _Offset;
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

    //SPI
    pinMode(9,OUTPUT);
    SPCR |= bit(SPE);
    pinMode(MISO,OUTPUT);
    SPI.attachInterrupt();

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
  if (data <= 100) {
    data *= 2;
    power = data;
    if (turn != 1) {
      for (int i = 0;i < 360;i++) {
        DriveMotor(i,power);
        delayMicroseconds(5);
      }
    }
    turn = true;
  }
  else if (data >= 101 && data <= 201) {
    data = (uint8_t)data - 101;
    power = data * 2;
    if (turn != 0) {
      for(int i = 359;i >= 0;i--) {
        DriveMotor(i,power);
        delayMicroseconds(5);
      }
    }
    turn = false;
  }
  else if (data == 253) {
    Stop();
  }
}

/*ISR (SPI_STC_vect) { //まず、SPIで253から255の範囲でデーターを送る。次に0から255の範囲でスピードのデータを送る
  byte data = SPDR;
  switch (data) {
    case 255://正転(時計回り)
      if (stop_flag) {
        stop_flag = false;
      }
      data = SPDR;
      power = (uint8_t)data;
      turn = true;
      for (uint16_t i = 359;i >= 0;i--) {
        DriveMotor(i,power);
        delayMicroseconds(8);
      }
      break;
  
    case 254://逆転(反時計回り)
      if (stop_flag) {
        stop_flag = false;
      }
      data = SPDR;
      power = (uint8_t)data;
      turn = false;
      for (uint16_t i = 0;i < 360;i++) {
        DriveMotor(i,power);
        delayMicroseconds(8);
      }
      break;
  
    case 253://ブレーキ
      stop_flag = true;
      break;
  
    default:
      break;
  }
}*/

/*ISR (TIMER5_COMPA_vect) {
  TIMSK5 |= (1 << OCIE5B);
}
ISR (TIMER5_COMPB_vect) {
  TIMSK5 |= (0 << OCIE5B);
}*/
