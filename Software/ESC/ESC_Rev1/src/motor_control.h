#include <Arduino.h>
#include <Wire.h>

#define AS5600_DEV_ADDRESS   0x36
#define AS5600_REG_RAW_ANGLE 0x0C

uint16_t offset = 731;
const int BLDC[3][2] = {{3,2},{5,4},{6,7}}; //Arduino Uno
//const int BLDC[3][2] = {{3,4},{5,6},{7,8}}; //Arduino Mega

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

void stop_motor() {
  analogWrite(BLDC[0][0],30);
  analogWrite(BLDC[1][0],0);
  analogWrite(BLDC[2][0],0);

  digitalWrite(BLDC[0][1],HIGH);
  digitalWrite(BLDC[1][1],HIGH);
  digitalWrite(BLDC[2][1],HIGH);
}

uint16_t calibration(void) {
  int offsetRaw[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  for (int i = 6;i > -1;i--) {
    motor_control(i % 6,30);
    delay(32000);
  }
  Serial.println("Yo");
  motor_control(5, 30);
  delay(19200);
  motor_control(0, 30);
  delay(32000);
  offsetRaw[0] = EncoderRead();
  motor_control(1, 30);
  delay(19200);
  motor_control(0, 30);
  delay(32000);
  offsetRaw[1] = EncoderRead();
  motor_control(5, 30);
  delay(19200);
  motor_control(0, 30);
  delay(32000);
  offsetRaw[2] = EncoderRead();
  motor_control(1, 30);
  delay(19200);
  motor_control(0, 30);
  delay(32000);
  offsetRaw[3] = EncoderRead();
  stop_motor();
  return (offsetRaw[0] + offsetRaw[1] + offsetRaw[2] + offsetRaw[3]) / 4;
}

/*uint8_t judge_step(uint16_t _Angle,bool _turn) {
  uint8_t _step;
  if (_turn) {
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
    _Angle += 150;
    if (_Angle <= 48 || _Angle >= 535) {
      _step = 5;
    }
    else if (_Angle >= 439 && _Angle <= 536) {
      _step = 0;
    }
    else if (_Angle >= 342 && _Angle <= 438) {
      _step = 1;
    }
    else if (_Angle >= 244 && _Angle <= 341) {
      _step = 2;
    }
    else if (_Angle >= 147 && _Angle <= 243) {
      _step = 3;
    }
    else if (_Angle >= 49 && _Angle <= 146) {
      _step = 4;
    }
  }
  return _step;
}*/