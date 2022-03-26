#include <Arduino.h>
#include <Wire.h>

#define AS5600_DEV_ADDRESS   0x36
#define AS5600_REG_RAW_ANGLE 0x0C

const int BLDC[3][2] = {{3,4},{5,6},{7,8}};
float pwm[360];

void DriveMotor(int _drive,int _power) {
  int power1 = (int)((_power / 2.0) * pwm[_drive] + (_power / 2.0));
  int power2 = (int)((_power / 2.0) * pwm[(_drive + 120) % 360] + (_power / 2.0));
  int power3 = (int)((_power / 2.0) * pwm[(_drive + 240) % 360] + (_power / 2.0));
  analogWrite(BLDC[0][0], power1);  // U_IN
  analogWrite(BLDC[1][0], power2);       // V_IN
  analogWrite(BLDC[2][0], power3);  
  //delayMicroseconds(500);
}

void SD_HIGH() {
  digitalWrite(BLDC[0][1], HIGH);  // V_SD
  digitalWrite(BLDC[1][1], HIGH);   // V_IN
  digitalWrite(BLDC[2][1], HIGH);
}

void motor_control_trapezoid(uint8_t _step, uint8_t _power) {
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