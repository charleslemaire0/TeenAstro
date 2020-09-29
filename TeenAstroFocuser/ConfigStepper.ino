// 
// 
// 

#include "ConfigStepper.h"
#include "Global.h"



void iniMot()
{
  pinMode(CSPin, OUTPUT);
  digitalWrite(CSPin, HIGH);
  teenAstroStepper.initMotor(
    static_cast<Driver::MOTORDRIVER>(AxisDriver),
    steprot->get(),
    EnablePin, CSPin, _DirPin, _StepPin,
    10.*curr->get(), micro->get(), true);
  stepper.setMaxSpeed(lowSpeed->get()*pow(2, micro->get()));
  stepper.setAcceleration(AccFact*manAcc->get());
  stepper.setInverseRotation(reverse->get());
  rotateController.rotateAsync(stepper);
  rotateController.overrideSpeed(0);
  mode = MAN;
}


void MoveTo(long pos)
{
  if (inlimit(pos))
  {
    modeGoto();
    breakgoto = false;
    stepper.setTargetAbs(pos);
    controller.moveAsync(stepper);
  }
}

bool inlimit(unsigned long pos)
{
  return 0UL <= pos && pos <= (unsigned long)maxPosition->get();
}

void writePos()
{
  if (!isMoving)
  {
    long posi = stepper.getPosition();
    if (posi == oldposition)
      return;
    oldposition = posi;
    long pos[3] = { posi,posi, posi };
    rtc->writeRamBulk((uint8_t*)pos, sizeof(pos));
  }
}


void iniPos()
{
  if (rtc == NULL)
  {
    rtc = new DS1302(kCePin, kIoPin, kSclkPin);
  }

  rtc->writeProtect(false);
  uint8_t ram[DS1302::kRamSize];
  rtc->readRamBulk(ram, DS1302::kRamSize);
  unsigned long* posini = (unsigned long*)ram;
  if (posini[0] == posini[1] && inlimit(posini[0]))
  {
    stepper.setPosition(posini[1]);
  }
  else
  {
    stepper.setPosition((long)startPosition->get());;
  }
  target = stepper.getPosition();
}