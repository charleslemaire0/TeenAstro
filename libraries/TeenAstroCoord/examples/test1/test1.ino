#include <TeenAstroCoord_EQ.hpp>
#include <TeenAstroCoord_HO.hpp>

int k = 0;


const double Lat[1] = { 0.88660302 };
const double Ha[1] = { -0.69112174 };
const double Dec[1] = { 0.14718022 };
const double RotE[1] = { random(-180, 180)*DEG_TO_RAD };
const double T_Az[1] = { -0.902320657 };
const double T_Alt[1] = { 0.63775167 };


void setup()
{
    Serial.begin(57600);
}

#define DEG2RAD   M_PI/180.
#define RAD2DEG   180./M_PI


void PrintGivenLat(double Lat)
{
  char text[200];
  sprintf(text, "Given Lat: %f\n", Lat * RAD2DEG);
  Serial.print(text);
}


void PrintGivenEQ( Coord_EQ *EQ)
{
  char text[200];
  sprintf(text, "Given Ha: %f\n", EQ->Ha() * RAD2DEG);
  Serial.print(text);
  sprintf(text, "Given Dec: %f\n", EQ->Dec() * RAD2DEG);
  Serial.print(text);
  sprintf(text, "Given FrE: %f\n", EQ->FrE() * RAD2DEG);
  Serial.print(text);
}

void PrintComputedEQ(Coord_EQ* EQ)
{
  char text[200];
  sprintf(text, "Computed Ha: %f\n", EQ->Ha() * RAD2DEG);
  Serial.print(text);
  sprintf(text, "Computed Dec: %f\n", EQ->Dec() * RAD2DEG);
  Serial.print(text);
  sprintf(text, "Computed FrE: %f\n", EQ->FrE() * RAD2DEG);
  Serial.print(text);
}

void PrintGivenHO( Coord_HO* HO)
{
  char text[200];
  sprintf(text, "Given Az: %f\n", HO->Az() * RAD2DEG);
  Serial.print(text);
  sprintf(text, "Given Alt: %f\n", HO->Alt() * RAD2DEG);
  Serial.print(text);
  sprintf(text, "Given FrH: %f\n", HO->FrH() * RAD2DEG);
  Serial.print(text);
}

void PrintComputedHO(Coord_HO* HO)
{
  char text[200];
  sprintf(text, "Computed Az: %f\n", HO->Az() * RAD2DEG);
  Serial.print(text);
  sprintf(text, "Computed Alt: %f\n", HO->Alt() * RAD2DEG);
  Serial.print(text);
  sprintf(text, "Computed RotH: %f\n", HO->FrH() * RAD2DEG);
  Serial.print(text);
}




Coord_HO HADEC2ALTAZ_T(const double& Lat, Coord_EQ EQ1, Coord_HO HO1)
{
  char text[200];
  LA3::RefrOpt Opt = { false,10,101 };
  Serial.println("---------------------------------------");
  Serial.println("-----SKY TO HORIZONTAL TOPOCENTRIC-----");
  Serial.println("---------------------------------------");
  PrintGivenEQ(&EQ1);

  Coord_HO HO2 = EQ1.To_Coord_HO(Lat, Opt);
  PrintComputedHO(&HO2);
 
  double d_Az = HO1.Az() - HO2.Az();
  double d_Alt = HO1.Alt() - HO2.Alt();

  sprintf(text, "error Az: %f\n", d_Az * RAD2DEG);
  Serial.print(text);
  sprintf(text, "error Alt: %f\n", d_Alt * RAD2DEG);
  Serial.print(text);
  return HO2;
}

Coord_HO HADEC2ALTAZ_A(const double& Lat, Coord_EQ EQ1, Coord_HO HO1)
{
  char text[200];
  LA3::RefrOpt Opt = { true,10,101 };
  Serial.println("---------------------------------------");
  Serial.println("-----SKY TO HORIZONTAL APPARENT--------");
  Serial.println("---------------------------------------");
  PrintGivenEQ(&EQ1);
  Coord_HO HO3 = HO1.ToApparent(Opt);
  Coord_HO HO2 = EQ1.To_Coord_HO(Lat, Opt);
  PrintComputedHO(&HO2);

  double d_Az = HO3.Az() - HO2.Az();
  double d_Alt = HO3.Alt() - HO2.Alt();

  sprintf(text, "error Az: %f\n", d_Az * RAD2DEG);
  Serial.print(text);
  sprintf(text, "error Alt: %f\n", d_Alt * RAD2DEG);
  Serial.print(text);
  return HO2;
}



Coord_EQ ALTAZ_T2HADEC(const double& Lat, Coord_HO HO1, Coord_EQ EQ1)
{
  char text[200];
  LA3::RefrOpt Opt = { false,10,101 };

  Serial.println("---------------------------------------");
  Serial.println("-----HORIZONTAL TOPOCENTRIC TO SKY-----");
  Serial.println("---------------------------------------");
  PrintGivenHO(&HO1);
  Coord_EQ EQ2 = HO1.To_Coord_EQ(Lat);

  PrintComputedEQ(&EQ2);
  
  double d_Ha = EQ1.Ha() - EQ2.Ha();
  double d_Dec = EQ1.Dec() - EQ2.Dec();

  sprintf(text, "error Ha: %f\n", d_Ha * RAD2DEG);
  Serial.print(text);
  sprintf(text, "error Dec: %f\n", d_Dec * RAD2DEG);
  Serial.print(text);
  return EQ2;
}

Coord_EQ ALTAZ_A2HADEC(const double& Lat, Coord_HO HO1, Coord_EQ EQ1)
{
  char text[200];
  LA3::RefrOpt Opt = { true,10,101 };

  Serial.println("---------------------------------------");
  Serial.println("------HORIZONTAL APPARENT TO SKY-------");
  Serial.println("---------------------------------------");
  PrintGivenHO(&HO1);
  Coord_HO HO2 = HO1.ToTopocentric(Opt); 
  Coord_EQ EQ2 = HO2.To_Coord_EQ(Lat);

  PrintComputedEQ(&EQ2);

  double d_Ha = EQ1.Ha() - EQ2.Ha();
  double d_Dec = EQ1.Dec() - EQ2.Dec();

  sprintf(text, "error Ha: %f\n", d_Ha * RAD2DEG);
  Serial.print(text);
  sprintf(text, "error Dec: %f\n", d_Dec * RAD2DEG);
  Serial.print(text);
  return EQ2;
}


void loop()
{

 //value in rad
 
  if (k < 1)
  {
    Coord_EQ givenEQ(RotE[k], Dec[k], Ha[k]);
    Coord_HO givenHO(RotE[k], T_Alt[k], T_Az[k],false);

    Coord_HO computed_HO = HADEC2ALTAZ_T(Lat[k], givenEQ, givenHO);
    Coord_EQ EQ_B1 = ALTAZ_T2HADEC(Lat[k], computed_HO, givenEQ);
    computed_HO = HADEC2ALTAZ_A( Lat[k], givenEQ, givenHO);
    ALTAZ_A2HADEC(Lat[k], computed_HO, givenEQ);
    k++;
  }
  //LA3::RefrOpt Opt = { true,10,101 };
  //static Coord_HO HO(0, 1.0*M_PI/180., 0, false);
  //PrintGivenHO(&HO);
  //HO = HO.ToApparent(Opt);
  //PrintComputedHO(&HO);
  //HO = HO.ToTopocentric(Opt);
  delay(10000);
}