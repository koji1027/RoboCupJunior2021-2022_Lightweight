#include <SPI.h>
#include <Arduino.h>
#include <stdint.h>
#include <motor_control.h>

#define I2C_Clock 8000000

void initialization();
void DriveMotor(int _drive,int _power);
void SD_HIGH();

bool flag = 0;
bool turn = 1;
uint8_t power = 70;
uint16_t offset[2] = {289,288};
uint16_t shinkaku [2]= {170,495};
int drive = 0;

void setup() {
    initialization();
    for (int i = 0;i < 360;i++) {
      pwm[i] = sin(radians(i));
    }
    SD_HIGH();
}

void loop() {
  if (turn) {
    uint16_t Angle = EncoderRead();
    Angle = 4095 - Angle;
    Angle += 4096 * 2;
    Angle -= offset[0];
    Angle %= 4096;
    drive = Angle % 585;
    drive = map(drive,0,587,0,359);
    drive += shinkaku[0];
    drive %= 360;
    DriveMotor(drive,power);
  }
  else {
    uint16_t Angle = EncoderRead();
    Angle = 4095 - Angle;
    Angle += 4096 * 2;
    Angle -= offset[1];
    Angle %= 4096;
    drive = Angle % 585;
    drive = map(drive,0,587,0,359);
    drive += 360 * 2;
    drive -= shinkaku[1];
    drive %= 360;
    DriveMotor(drive,power);
  }
}

void initialization() {
    //pwm
    //TCCR0B = (TCCR2B & 0b11111000) | 0x01;  // 31.37255 [kHz]
    //TCCR1B = (TCCR3B & 0b11111000) | 0x01;  // 31.37255 [kHz] //Arduino Uno
    TCCR3B = (TCCR3B & 0b11111000) | 0x01;  // 31.37255 [kHz]
    TCCR4B = (TCCR4B & 0b11111000) | 0x01;  // 31.37255 [kHz] //Arduino Mega

    //I2C
    Wire.begin();
    Wire.setClock(I2C_Clock);

    Serial.begin(115200);

    //タイマー割込み
    //絶対消さない!!!!!
    TCCR5A  = 0;
    TCCR5B  = 0;
    TCCR5B |= (1 << WGM52) | (1 << CS52);  //CTCmode //prescaler to 256
    OCR5A   = 62500-1;
    OCR5B   = 6250-1;

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
    delay(1500);
    Serial.println("Start");
    TIMSK5 |= (1 << OCIE5A); //割り込みA開始
}

ISR (SPI_STC_vect) {
  byte data = SPDR;
  if (data == 255) {
    data = SPDR;
    power = data;
    turn = true;
  }
  else if (data == 254) {
    data = SPDR;
    power = data;
    turn = false;
  }
}

ISR (TIMER5_COMPA_vect) {
  turn = not(turn);
  //TIMSK5 |= (1 << OCIE5B);
}

ISR (TIMER5_COMPB_vect) {
  TIMSK5 |= (0 << OCIE5B);
}