

#include <TeenAstroCoordConv.hpp>
CoordConv virtualMount;
CoordConv Misalignment;
CoordConv Misalignment2;
#define ErrorSync 3 // in degree
#define ErrorPol 3  // in degree
#define HA1 0.0
#define DEC1 0.0

#define HA2 0.0
#define DEC2 70.0
#define HA3 6.0
#define DEC3 0.0

void setup()
{
  Serial.begin(57600);
}

class coord
{
public:
  double ra;
  double dec;
  coord addError(coord error)
  {
    return coord(ra + error.ra, dec + error.dec);
  } 
  coord subError(coord error)
  {
    return coord(ra - error.ra, dec - error.dec);
  }
  coord delta_to(coord value)
  {
    return coord(ra - value.ra, dec - value.dec);
  }
  coord(double rain, double decin)
  {
    ra = rain;
    dec = decin;
  }
  coord()
  {
    ra = 0;
    dec = 0;
  }
};

void loop()
{
  coord stars[3] = { coord(HA1 * 15, DEC1), coord(HA2 * 15, DEC2), coord(HA3 * 15, DEC3) };
  char text[100];
  virtualMount.addReferenceDeg(0, 90, 0, 90 - ErrorPol);
  virtualMount.addReferenceDeg(90, 0, 90, 0);
  virtualMount.calculateThirdReference();

  coord instruments1[3]; //instrument without sync Error
  coord instruments2[3]; //instrument with sync Error
  coord SyncError(ErrorSync, ErrorSync);

  for (int k = 0; k < 3; k++)
  {
    instruments2[k] = instruments1[k].addError(SyncError);
    virtualMount.toAxisDeg(instruments2[k].ra, instruments2[k].dec, stars[k].ra, stars[k].dec);
  }

  // First scenario
  // with these instrument observations the alignment is computed
  coord control;
  coord Pointing_Err_scenario1;
  //compute the alignment with these observations
  Misalignment.addReferenceDeg(stars[0].ra, stars[0].dec, instruments2[0].ra, instruments2[0].dec);
  Misalignment.addReferenceDeg(stars[1].ra, stars[1].dec, instruments2[1].ra, instruments2[1].dec);
  Misalignment.calculateThirdReference();
  // the pointing error is computed on a thrird star
  Misalignment.toReferenceDeg(control.ra, control.dec, instruments2[2].ra, instruments2[2].dec);
  Pointing_Err_scenario1 = stars[2].delta_to(control);

  Serial.print("pointing error without sync at the begining of the alignment\n");
  sprintf(text, "error Ra: %f\n", Pointing_Err_scenario1.ra);
  Serial.print(text);
  sprintf(text, "error Dec: %f\n", Pointing_Err_scenario1.dec);
  Serial.print(text);

  // Second scenario
  // a simple alignment is made on the first star
  // we compute the observed sync error
  coord First_Obs_Sync_Err_scenario2;
  First_Obs_Sync_Err_scenario2 = instruments2[0].delta_to(stars[0]);
  Serial.println(First_Obs_Sync_Err_scenario2.ra);
  Serial.println(First_Obs_Sync_Err_scenario2.dec);
  // the sync error is substracted to all observations.
  coord instruments3[3]; //instrument with approximated corrected sync Error
  for (int k = 0; k < 3; k++)
  {
    instruments3[k] = instruments2[k].subError(First_Obs_Sync_Err_scenario2);
  }
  //compute the alignment with these observations

  Misalignment.reset();
  Misalignment.addReferenceDeg(stars[0].ra, stars[0].dec, instruments3[0].ra, instruments3[0].dec);
  Misalignment.addReferenceDeg(stars[1].ra, stars[1].dec, instruments3[1].ra, instruments3[1].dec);
  Misalignment.calculateThirdReference();
  //the pointing error is computed on a thrird star
  Misalignment.toReferenceDeg(control.ra, control.dec, instruments3[2].ra, instruments3[2].dec);

  coord Pointing_Err_scenario2 = stars[2].delta_to(control);
  Serial.print("pointing error with sync at the begining of the alignment\n");
  sprintf(text, "error Ra: %f\n", Pointing_Err_scenario2.ra);
  Serial.print(text);
  sprintf(text, "error Dec: %f\n", Pointing_Err_scenario2.dec);
  Serial.print(text);


  delay(1000);
}