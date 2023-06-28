

#include <TeenAstroCoordConv.hpp>


void setup()
{
  Serial.begin(57600);
}


void loop()
{
  char text[100]; 
  double angle1 = (double)(random(0, __LONG_MAX__) - __LONG_MAX__ / 2) / ((double)__LONG_MAX__) * 2 * M_PI;
  double angle2 = (double)(random(0, __LONG_MAX__) - __LONG_MAX__ / 2) / ((double)__LONG_MAX__) * M_PI;
  double angle3 = (double)(random(0, __LONG_MAX__) - __LONG_MAX__ / 2) / ((double)__LONG_MAX__) * 2 * M_PI;
  sprintf(text, "Angle1: %f\n", angle1 * 180 / M_PI);
  Serial.print(text);
  sprintf(text, "Angle2: %f\n", angle2 * 180 / M_PI);
  Serial.print(text);
  sprintf(text, "Angle3: %f\n", angle3 * 180 / M_PI);
  Serial.print(text);
  LA3::SingleRotation rots[3] = { {LA3::RotAxis::ROTAXISZ, angle1 }, {LA3::RotAxis::ROTAXISX,angle2 },{LA3::RotAxis::ROTAXISY,angle3 } };
  double m[3][3];
  LA3::getMultipleRotationMatrix(m, rots, 3);
  double e_angle1, e_angle2, e_angle3;
  LA3::getEulerRzRxRy(m, e_angle1, e_angle2, e_angle3);
  sprintf(text, "Euler Angle1: %f\n", e_angle1 * 180 / M_PI);
  Serial.print(text);
  sprintf(text, "Euler Angle2: %f\n", e_angle2 * 180 / M_PI);
  Serial.print(text);
  sprintf(text, "Euler Angle3: %f\n", e_angle3 * 180 / M_PI);
  Serial.print(text);

  double d_angle1 = angle1 - e_angle1;
  double d_angle2 = angle2 - e_angle2;
  double d_angle3 = angle3 - e_angle3;
  
  sprintf(text, "error Angle1: %f\n", d_angle1 * 180 / M_PI);
  Serial.print(text);
  sprintf(text, "error Angle2: %f\n", d_angle2 * 180 / M_PI);
  Serial.print(text);
  sprintf(text, "error Angle3: %f\n", d_angle3 * 180 / M_PI);
  Serial.print(text);
  Serial.println("*********");

  delay(1000);
}