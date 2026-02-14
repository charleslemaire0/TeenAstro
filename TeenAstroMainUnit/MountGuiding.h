#pragma once
#include "MountTypes.h"
#include "Axis.hpp"

class Mount;

class MountGuiding {
public:
  explicit MountGuiding(Mount& mount);
  bool hasGuiding() const;
  void setGuidingState(Guiding g);
  bool isGuidingStar() const;
  void stopGuiding();
  bool stopIfMountError();
  void performPulseGuiding();
  void performST4Guiding();
  void performGuidingRecenter();
  void performGuidingAtRate();
  void guide();

  volatile Guiding GuidingState;   // used from ISR/timer
  Guiding lastGuidingState;
  double guideRates[5];
  volatile byte activeGuideRate;   // used from ISR/timer
  volatile byte recenterGuideRate; // used from ISR/timer
  GuideAxis guideA1;
  GuideAxis guideA2;
  float pulseGuideRate;
  double DegreesForAcceleration;

private:
  void applyGuidingA1();
  void applyGuidingA2();
  Mount& mount_;
};
