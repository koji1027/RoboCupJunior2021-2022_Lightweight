//使うマイコン(Uno,Mega)によってレジスタ(initialize関数内)と
//ピン(motor_control.h内)の設定を変えること
//Unoで使う場合delay・delayMicrosecondsは使えない
//プログラムをいじるときはなるべく変更箇所を消さずにコメントアウトする。

//I2Cでエラーが出る場合はI2C_Clockの値を4000000まで下げてみる
#include <SPI.h>
#include <Arduino.h>
#include <stdint.h>
#include <motor_control.h>

#define I2C_Clock 4000000

uint8_t power = 50;
bool turn = 1; //true(1) 正転 / false(0) 逆転
int drive = 0;
int val1,val2 = -1;
bool flag1,flag2 = 0;
unsigned long period = 0;

void stop_motor();
uint8_t judge_step(uint16_t _Angle,bool _turn);
void get_speed();
void initialization();
uint16_t EncoderRead();
uint16_t calibration();
void motor_control(uint8_t _step,uint8_t _power);

void setup() {
  // put your setup code here, to run once:
  initialization();
  delay(32000);
  offset = calibration();
  //pinMode(9,OUTPUT);
  //digitalWrite(9,LOW);
}

void loop() {
  //digitalWrite(9,HIGH);
  int Angle = EncoderRead();
  Angle -= offset;
  Angle += 4096 * 2;
  Angle %= 4096;
  Angle = 4095 - Angle;
  drive = 0;
  if (turn) {
    Angle += 4096 * 2;
    Angle += 10;
    Angle %= 4096;
    drive = (int)((float)Angle / 97.5) % 6;
    drive += 2;
    drive %= 6;
  }
  else {
    Angle += 4096 * 2;
    Angle -= 50;
    Angle %= 4096;
    drive = (int)((float)Angle / 97.5) % 6;
    drive += 5;
    drive %= 6;
  }
  motor_control(drive,power);
}

void initialization() {
  //pwm
  TCCR0B = (TCCR0B & 0b11111000) | 0x01;  // 31.37255 [kHz]
  TCCR2B = (TCCR2B & 0b11111000) | 0x01;  // 31.37255 [kHz] //Arduino Uno
  //TCCR2B = (TCCR2B & 0b11111000) | 0x01;  // 31.37255 [kHz]
  //TCCR3B = (TCCR3B & 0b11111000) | 0x01;  // 31.37255 [kHz]
  //TCCR4B = (TCCR4B & 0b11111000) | 0x01;  // 31.37255 [kHz] //Arduino Mega

  //I2C
  Wire.begin();
  Wire.setClock(I2C_Clock);

  //タイマー割込み
  //絶対消さない!!!!!
  /*TCCR5A  = 0;
  TCCR5B  = 0;
  TCCR5B |= (1 << WGM52) | (1 << CS52);  //CTCmode //prescaler to 256
  OCR5A   = 62500-1;
  OCR5B   = 3125-1;*/

  //Serial
  Serial.begin(115200);
  
  //SPI
  SPCR |= bit(SPE);
  pinMode(MISO,OUTPUT);
  SPI.attachInterrupt();

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
  delay(64000);
  Serial.println("Start");
  //TIMSK5 |= (1 << OCIE5A);
}

ISR (SPI_STC_vect) {
  byte data = SPDR;
  if (data <= 100) {
    data *= 2;
    power = data;
    if (turn != 1) {
      for (int i = 0;i < 6;i++) {
        motor_control(i,0);
        delayMicroseconds(2000);
      }
    }
    turn = 1;
  }
  else if (data >= 101 && data <= 201) {
    data = (uint8_t)data - 101;
    power = data * 2;
    if (turn != 0) {
      for(int i = 5;i >= 0;i--) {
        motor_control(i,0);
        delayMicroseconds(2000);
      }
    }
    turn = 0;
  }
}

/*ISR (TIMER5_COMPA_vect) {
  //TIMSK5 |= (1 << OCIE5B);
}

ISR (TIMER5_COMPB_vect) {
  flag2 = 1;
  TIMSK5 |= (0 << OCIE5B);
}*/