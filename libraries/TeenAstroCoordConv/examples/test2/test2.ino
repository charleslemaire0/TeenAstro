

#include <TeenAstroCoordConv.hpp>
#include <TeenAstroCoord_EQ.hpp>
#include <TeenAstroCoord_HO.hpp>
#include <TeenAstroCoord_IN.hpp>



const double Lat[2] = { 0.88660302, -24.6274 * DEG_TO_RAD };
const double Ha[2] = { -0.69112174, 19.486*15 *DEG_TO_RAD };
const double Dec[2] = { 0.14718022, -50* DEG_TO_RAD };
const double RotE[2] = { random(-180, 180) * DEG_TO_RAD,0 };
const double T_Az[2] = { -0.902320657+M_PI, 135* DEG_TO_RAD };
const double T_Alt[2] = { 0.63775167,32.74* DEG_TO_RAD };
LA3::RefrOpt Opt = { false,10,101 };

int k;
void setup()
{
  Serial.begin(57600);
  k = 0;
}


void PrintGivenHO(double Lat, double  Az, double Alt, double RotH)
{
  char text[200];
  sprintf(text, "Given Lat: %f\n", Lat * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Given Az: %f\n", Az * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Given Alt: %f\n", Alt * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Given RotH: %f\n", RotH * RAD_TO_DEG);
  Serial.print(text);
}

void PrintComputedHO(double Az, double Alt, double RotH)
{
  char text[200];
  sprintf(text, "Computed Az: %f\n", Az * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Computed Alt: %f\n", Alt * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Computed RotH: %f\n", RotH * RAD_TO_DEG);
  Serial.print(text);
}

void loop()
{

  if (k < 2)
  {
    Coord_HO HO1 = Coord_HO(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD,true);
    Coord_EQ EQ1 = HO1.To_Coord_EQ(Lat[k]);
    Coord_IN IN1 = Coord_IN(EQ1.FrE(), EQ1.Dec(), EQ1.Ha());

    Coord_HO HO2 = Coord_HO(0, 45 * DEG_TO_RAD, 270 * DEG_TO_RAD,true);
    Coord_EQ EQ2 = HO2.To_Coord_EQ(Lat[k]);
    Coord_IN IN2 = Coord_IN(EQ2.FrE(), EQ2.Dec(), EQ2.Ha());
    CoordConv virtualEQMount;
    virtualEQMount.addReference(HO1.Az(), HO1.Alt(), IN1.Axis1(), IN1.Axis2());
    virtualEQMount.addReference(HO2.Az(), HO2.Alt(), IN2.Axis1(), IN2.Axis2());
    virtualEQMount.calculateThirdReference();

    PrintGivenHO(Lat[k], T_Az[k], T_Alt[k], 0);

    Coord_IN INtest = Coord_IN(RotE[k], Dec[k], Ha[k]);
    Coord_HO HOtest = INtest.To_Coord_HO(virtualEQMount.T, Opt);

    PrintComputedHO(HOtest.Az(), HOtest.Alt(), HOtest.FrH());
    k++;
    delay(1000);
  }
  delay(1000);
}