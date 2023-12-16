#pragma once


class PointingModel
{
public:
  void addStar(EqCoords*);
  void init(bool);
  void reset(void);
  void save(void);
  void instr(Axes*, Axes*);   // get instrument coordinates from sky 
  void sky(Axes*, Axes*);     // get sky coordinates from instrument 
  bool isReady(void);
  int numStars(void);
  CoordConv alignment;
};
