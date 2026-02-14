#pragma once
class Mount;

class MountST4 {
public:
  explicit MountST4(Mount& mount);
  void setup();
  void check();
private:
  Mount& mount_;
};
