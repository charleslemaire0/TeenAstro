#pragma once
#include <stdint.h>
class Mount;

class MountParkHomeController {
public:
  explicit MountParkHomeController(Mount& mount);
  bool setPark();
  void unsetPark();
  void parkClearBacklash();
  void finalizePark();
  uint8_t park();
  bool syncAtPark();
  bool iniAtPark();
  void unpark();
  bool setHome();
  void unsetHome();
  bool goHome();
  void finalizeHome();
  bool syncAtHome();
  void initHome();
private:
  Mount& mount_;
};
