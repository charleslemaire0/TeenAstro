#include "Global.h"

/*
 * Add a star in EQ coordinates to the sky model
 *
 */
void PointingModel::addStar(EqCoords *sP)
{
  Steps steps;
  Axes sky, instr;

  // computed axes positions
  mount.mP->eqToAxes(sP, &sky, mount.mP->GetPierSide());

  // actual axes positions
  steps.steps1 = motorA1.getCurrentPos();
  steps.steps2 = motorA2.getCurrentPos();
  mount.mP->stepsToAxes(&steps, &instr);

  alignment.addReferenceDeg(sky.axis1, sky.axis2, instr.axis1, instr.axis2);

  if (alignment.getRefs() == 2)
  {
    alignment.calculateThirdReference();
    if (alignment.isReady())
    {
      motorA1.setTargetPos(motorA1.getCurrentPos());
      motorA2.setTargetPos(motorA2.getCurrentPos());
    }
  }
}

void PointingModel::reset(void)
{
  alignment.reset();
}

void PointingModel::init(bool reset)
{
  float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;

  byte TvalidFromEEPROM = XEEPROM.read(getMountAddress(EE_Tvalid));

  if (TvalidFromEEPROM == 1 && reset)
  {
    XEEPROM.write(getMountAddress(EE_Tvalid), 0);
  }
  if (TvalidFromEEPROM == 1 && !reset)
  {
    t11 = XEEPROM.readFloat(getMountAddress(EE_T11));
    t12 = XEEPROM.readFloat(getMountAddress(EE_T12));
    t13 = XEEPROM.readFloat(getMountAddress(EE_T13));
    t21 = XEEPROM.readFloat(getMountAddress(EE_T21));
    t22 = XEEPROM.readFloat(getMountAddress(EE_T22));
    t23 = XEEPROM.readFloat(getMountAddress(EE_T23));
    t31 = XEEPROM.readFloat(getMountAddress(EE_T31));
    t32 = XEEPROM.readFloat(getMountAddress(EE_T32));
    t33 = XEEPROM.readFloat(getMountAddress(EE_T33));
    alignment.setT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
    alignment.setTinvFromT();
  }
  else
  {
    alignment.clean();
    motorA1.setTargetPos(motorA1.getCurrentPos());
    motorA2.setTargetPos(motorA2.getCurrentPos());
  }
}


void PointingModel::save(void)
{
  // and store our corrections
  float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
  XEEPROM.write(getMountAddress(EE_Tvalid), mount.mP->pm.alignment.isReady());
  if (mount.mP->pm.alignment.isReady())
  {
    mount.mP->pm.alignment.getT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
  }
  XEEPROM.writeFloat(getMountAddress(EE_T11), t11);
  XEEPROM.writeFloat(getMountAddress(EE_T12), t12);
  XEEPROM.writeFloat(getMountAddress(EE_T13), t13);
  XEEPROM.writeFloat(getMountAddress(EE_T21), t21);
  XEEPROM.writeFloat(getMountAddress(EE_T22), t22);
  XEEPROM.writeFloat(getMountAddress(EE_T23), t23);
  XEEPROM.writeFloat(getMountAddress(EE_T31), t31);
  XEEPROM.writeFloat(getMountAddress(EE_T32), t32);
  XEEPROM.writeFloat(getMountAddress(EE_T33), t33);
}

// get instrument coordinates from sky 
void PointingModel::instr(Axes *sP, Axes *iP)
{
  if (alignment.isReady())
    alignment.toInstrumentDeg(iP->axis1, iP->axis2, sP->axis1, sP->axis2);
  else
  {
    iP->axis1 = sP->axis1;
    iP->axis2 = sP->axis2;
  }
}

// get sky coordinates from instrument 
void PointingModel::sky(Axes *iP, Axes *sP)
{
  if (alignment.isReady())
    alignment.toReferenceDeg(sP->axis1, sP->axis2, iP->axis1, iP->axis2);
  else
  {
    sP->axis1 = iP->axis1;
    sP->axis2 = iP->axis2;
  }
}  

bool PointingModel::isReady(void)
{
  return alignment.isReady();
}

int PointingModel::numStars(void)
{
  return alignment.getRefs();
}
