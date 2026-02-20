#pragma once
/**
 * Application - setup and main loop for TeenAstro Main Unit (C++ build).
 * Thin main (TeenAstroMainUnit.cpp) delegates to application.setup() / application.loop().
 */
class Application {
public:
  void setup();
  void loop();

private:
  void setupStartupBlink();
  void setupEepromAndMount();
  void setupTimeSync();
  void setupReticule();
  void setupMountPinsAndSidereal();
  void setupParkHomeAndGuideRates();
  void setupGnssProbe();
  void setupFocuserProbe();
  void setupCommandSerial();

  void loopSt4AndGuiding();
  void loopEncoderSync();
  void updateForceTracking(bool& forceTracking);
  void loopSiderealAndSafety(bool& forceTracking);
  void loopCommandsAndStatus(ErrorsTraking startLoopError);
};

extern Application application;
