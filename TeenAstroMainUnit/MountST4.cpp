#include "MountST4.h"
#include "MainUnit.h"

namespace {
constexpr unsigned int ST4_DEBOUNCE_US = 5;
}

MountST4::MountST4(Mount& mount) : mount_(mount) {}

void MountST4::setup()
{
#if HASST4
  pinMode(ST4RAw, INPUT);
  pinMode(ST4RAe, INPUT);
  pinMode(ST4DEn, INPUT);
  pinMode(ST4DEs, INPUT);
#endif
}

void MountST4::check()
{
#if HASST4
  byte w1 = HIGH, w2 = HIGH, e1 = HIGH, e2 = HIGH, n1 = HIGH, n2 = HIGH, s1 = HIGH, s2 = HIGH;
  static char ST4RA_state = 0;
  static char ST4RA_last = 0;
  static char ST4DE_state = 0;
  static char ST4DE_last = 0;
  if (mount_.errors.lastError != ERRT_NONE)
  {
    if (mount_.guiding.GuidingState == GuidingST4)
    {
      mount_.stopAxis1();
      mount_.stopAxis2();
    }
    return;
  }

  w1 = digitalRead(ST4RAw);
  e1 = digitalRead(ST4RAe);
  n1 = digitalRead(ST4DEn);
  s1 = digitalRead(ST4DEs);
  delayMicroseconds(ST4_DEBOUNCE_US);
  w2 = digitalRead(ST4RAw);
  e2 = digitalRead(ST4RAe);
  n2 = digitalRead(ST4DEn);
  s2 = digitalRead(ST4DEs);

  if ((w1 == w2) && (e1 == e2))
  {
    ST4RA_state = 0;
    if (w1 != e1)
    {
      if (w1 == LOW)
        ST4RA_state = mount_.getPoleSide() == POLE_UNDER ? '+' : '-';
      if (e1 == LOW)
        ST4RA_state = mount_.getPoleSide() == POLE_UNDER ? '-' : '+';
    }
  }
  if ((n1 == n2) && (s1 == s2))
  {
    ST4DE_state = 0;
    if (n1 != s1)
    {
      if (n1 == LOW)
        ST4DE_state = '-';
      if (s1 == LOW)
        ST4DE_state = '+';
    }
  }

  if (ST4RA_last != ST4RA_state)
  {
    ST4RA_last = ST4RA_state;
    if (ST4RA_state)
    {
      mount_.moveAxis1(ST4RA_state == '-', Guiding::GuidingST4);
    }
    else
    {
      mount_.stopAxis1();
    }
  }

  if (ST4DE_last != ST4DE_state)
  {
    ST4DE_last = ST4DE_state;
    if (ST4DE_state)
    {
      mount_.moveAxis2(ST4DE_state == '-', Guiding::GuidingST4);
    }
    else
    {
      mount_.stopAxis2();
    }
  }
#endif
}
