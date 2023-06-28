

#include <TeenAstroCoordConv.hpp>
#include <TeenAstroCoord_EQ.hpp>
#include <TeenAstroCoord_HO.hpp>
#include <TeenAstroCoord_IN.hpp>



const double Lat[2] = { 0.88660302, -24.6274 * DEG_TO_RAD };
const double Ha[2] = { -0.69112174, 19.486*15 *DEG_TO_RAD };
const double Dec[2] = { 0.14718022, -50* DEG_TO_RAD };
const double RotE[2] = { random(-180, 180) * DEG_TO_RAD,0 };
const double T_Az[2] = { -0.902320657, 135* DEG_TO_RAD + M_PI };
const double T_Alt[2] = { 0.63775167,32.74* DEG_TO_RAD };
LA3::RefrOpt Opt = { false,10,101 };

int k;
void setup()
{
  Serial.begin(57600);
  k = 0;
}


void PrintGivenEQ(double Ha, double Dec, double RotE)
{
  char text[200];
  sprintf(text, "Given Ha: %f\n", Ha * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Given Dec: %f\n", Dec * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Given RotE: %f\n", RotE * RAD_TO_DEG);
  Serial.print(text);
}

void PrintComputedEQ(double Ha, double Dec, double RotE)
{
  char text[200];
  sprintf(text, "Computed Ha: %f\n", Ha * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Computed Dec: %f\n", Dec * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Computed RotE: %f\n", RotE * RAD_TO_DEG);
  Serial.print(text);
}


void PrintGivenHO(double  Az, double Alt, double RotH)
{
  char text[200];
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

void PrintGivenIn( double axis1, double axis2, double axis3)
{
  char text[200];
  sprintf(text, "Given axis1: %f\n", axis1 * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Given axis2: %f\n", axis2 * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Given axis3: %f\n", axis3 * RAD_TO_DEG);
  Serial.print(text);
}

void PrintComputedIN(double axis1, double axis2, double axis3)
{
  char text[200];
  sprintf(text, "Computed axis1: %f\n", axis1 * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Computed axis2: %f\n", axis2 * RAD_TO_DEG);
  Serial.print(text);
  sprintf(text, "Computed axis3: %f\n", axis3 * RAD_TO_DEG);
  Serial.print(text);
}


void loop()
{

  if (k < 2)
  {
    Coord_HO HO1 = Coord_HO(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD,true);
    Coord_EQ EQ1 = HO1.To_Coord_EQ(Lat[k]);
    Coord_IN IN1 = Coord_IN(EQ1.FrE(), EQ1.Dec(), EQ1.Ha()+M_PI);

    Coord_HO HO2 = Coord_HO(0, 45 * DEG_TO_RAD, -90 * DEG_TO_RAD,true);
    Coord_EQ EQ2 = HO2.To_Coord_EQ(Lat[k]);
    Coord_IN IN2 = Coord_IN(EQ2.FrE(), EQ2.Dec(), EQ2.Ha()+M_PI);
    CoordConv virtualEQMount;
    virtualEQMount.addReference(HO1.Az(), HO1.Alt(), IN1.Axis1(), IN1.Axis2());
    virtualEQMount.addReference(HO2.Az(), HO2.Alt(), IN2.Axis1(), IN2.Axis2());
    virtualEQMount.calculateThirdReference();

    //PrintGivenHO( T_Az[k], T_Alt[k], 0);



    Coord_IN INtest = Coord_IN(RotE[k], Dec[k], Ha[k]+ M_PI);
    Coord_HO HOtest = INtest.To_Coord_HO(virtualEQMount.T, Opt);
    Coord_EQ EQtest = INtest.To_Coord_EQ(virtualEQMount.T, Opt, Lat[k]);
    PrintGivenIn(INtest.Axis1(), INtest.Axis2(), INtest.Axis3());

    PrintComputedEQ(EQtest.Ha(), EQtest.Dec(), EQtest.FrE());
    PrintComputedHO(HOtest.Az(), HOtest.Alt(), HOtest.FrH());

    // back to instrument

    Coord_IN INcomputed = HOtest.To_Coord_IN(virtualEQMount.Tinv);
    PrintComputedIN(INcomputed.Axis1(), INcomputed.Axis2(), INcomputed.Axis3());
    k++;
    delay(1000);
  }
  delay(1000);
}