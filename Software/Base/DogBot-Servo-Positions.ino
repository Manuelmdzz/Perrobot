#include <Servo.h>

Servo Leg1F;  //D2 - 80
Servo Leg1B;  //D3 - 100
Servo Leg2F;  //D4 - 100
Servo Leg2B;  //D5 - 80
Servo Leg3F;  //D6 - 80
Servo Leg3B;  //D7 - 100
Servo Leg4F;  //D8 - 100
Servo Leg4B;  //D9 - 80

Servo Headservo;  //D10 - 90

void setup() {
  Leg1F.attach(2);
  Leg1B.attach(3);
  Leg2F.attach(4);
  Leg2B.attach(5);
  Leg3F.attach(6);
  Leg3B.attach(7);
  Leg4F.attach(8);
  Leg4B.attach(9);
  Headservo.attach(10);

  Leg1F.write(80);
  Leg1B.write(100);
  Leg2F.write(100);
  Leg2B.write(80);
  Leg3F.write(80);
  Leg3B.write(100);
  Leg4F.write(100);
  Leg4B.write(80);

  Headservo.attach(90);
}

void loop() {
  // put your main code here, to run repeatedly:

}
