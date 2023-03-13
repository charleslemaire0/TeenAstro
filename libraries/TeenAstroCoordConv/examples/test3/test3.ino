#include <TeenAstroCoordConv.hpp>


void setup()
{
  Serial.begin(57600);
}

#define DEG2RAD   M_PI/180.
#define RAD2DEG   180./M_PI

void loop()
{

  char text[100];
 //value in rad
  double Lat[1] = {0.88660302};
  double Ha[1] = { -0.69112174 };
  double Dec[1] = {0.14718022 };
  double RotE[1] = {0 };
  double T_Az[1] = { -0.902320657 };
  double T_Alt[1] = {0.63775167 };

  for (int k = 0 ; k < 1; k++)
  {
    sprintf(text, "Ha: %f\n", Ha[k] * RAD2DEG);
    Serial.print(text);
    sprintf(text, "Dec: %f\n", Dec[k] * RAD2DEG);
    Serial.print(text);
    sprintf(text, "Lat: %f\n", Lat[k] * RAD2DEG);
    Serial.print(text);


    LA3::SingleRotation rots[4] = {
      {LA3::RotAxis::ROTAXISX,RotE[k] },
      {LA3::RotAxis::ROTAXISY,Dec[k] },
      {LA3::RotAxis::ROTAXISZ,Ha[k] },
      {LA3::RotAxis::ROTAXISY, (M_PI_2 - Lat[k]) }
    };
 

    double m2[3][3];
    LA3::getMultipleRotationMatrix(m2, rots, 4);
    sprintf(text, " %f, %f, %f\n", m2[0][0],m2[1][0],m2[2][0]);
    Serial.print(text);
    sprintf(text, " %f, %f, %f\n", m2[0][1], m2[1][1], m2[2][1]);
    Serial.print(text);
    sprintf(text, " %f, %f, %f\n", m2[0][2], m2[1][2], m2[2][2]);
    Serial.print(text);


    double Az, Alt, RotH;
    LA3::getEulerRxRyRz(m2, RotH, Alt, Az);
    Az *= RAD2DEG;
    Alt *= RAD2DEG;
    RotH *= RAD2DEG;

    sprintf(text, "Az: %f\n", Az);
    Serial.print(text);
    sprintf(text, "Alt: %f\n", Alt);
    Serial.print(text);
    sprintf(text, "RotH: %f\n", RotH);
    Serial.print(text);

    double d_Az = T_Az[k] * RAD2DEG - Az;
    double d_Alt = T_Alt[k] * RAD2DEG - Alt;


    sprintf(text, "error Az: %f\n", d_Az);
    Serial.print(text);
    sprintf(text, "error Alt: %f\n", d_Alt);
    Serial.print(text);

    Serial.println("*********");
  }
  delay(1000);
}