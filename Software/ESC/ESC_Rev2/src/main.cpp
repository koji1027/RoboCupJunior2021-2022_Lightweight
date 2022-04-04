#include <SPI.h>
#include <Arduino.h>
#include <stdint.h>
#include <motor_control.h>

#define I2C_Clock 8000000

void initialization();
void DriveMotor(int _drive,int _power);
void SD_HIGH();
uint16_t calibration();

bool stop_flag = 0; //0:回転可 1:強制停止(ブレーキ)
bool turn = 1; //1:正転(時計回り) 0:逆転(反時計回り)
uint8_t power = 50; //0から255 
uint16_t offset = 13;//441,586
uint16_t shinkaku [2]= {50,300};//170 //495
uint16_t drive = 0;

void setup() {
    initialization();
    for (uint16_t i = 0;i < 360;i++) {
      pwm[i] = sin(radians(i));
    }
    SD_HIGH();
    //offset = calibration();
}

void loop() {
  /*while (1) {
    for (int i = 0;i < 360;i++) {
      DriveMotor(i,50);
      delayMicroseconds(10);
    }
    delay(300 * 64);
    DriveMotor(0,50);
    delay(300 * 64);
    int data = EncoderRead();
    data += 4096 * 2;
    data -= offset;
    data %= 4096;
    drive = data % 585;
    drive = map(drive,0,587,0,359);
    drive += 360 * 2;
    drive -= shinkaku[0];
    drive %= 360;
    Serial.print("data : ");
    Serial.print(data);
    Serial.print(" ,drive : ");
    Serial.println(drive);
    delay(9000);
  }*/
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

uint16_t calibration(void) {
  int offsetRaw[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  for (int i = 6;i > -1;i--) {
    motor_control_trapezoid(i % 6,30);
    delay(32000);
  }
  motor_control_trapezoid(5, 30);
  delay(19200);
  motor_control_trapezoid(0, 30);
  delay(32000);
  offsetRaw[0] = EncoderRead();
  motor_control_trapezoid(1, 30);
  delay(19200);
  motor_control_trapezoid(0, 30);
  delay(32000);
  offsetRaw[1] = EncoderRead();
  motor_control_trapezoid(5, 30);
  delay(19200);
  motor_control_trapezoid(0, 30);
  delay(32000);
  offsetRaw[2] = EncoderRead();
  motor_control_trapezoid(1, 30);
  delay(19200);
  motor_control_trapezoid(0, 30);
  delay(32000);
  offsetRaw[3] = EncoderRead();
  motor_control_trapezoid(0,0);
  return (offsetRaw[0] + offsetRaw[1] + offsetRaw[2] + offsetRaw[3]) / 4;
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
    /*pinMode(9,OUTPUT);
    SPCR |= bit(SPE);
    pinMode(MISO,OUTPUT);
    SPI.attachInterrupt();*/

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