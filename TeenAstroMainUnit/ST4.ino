void setupST4()
{
#if HASST4
  pinMode(ST4RAw, INPUT);
  pinMode(ST4RAe, INPUT);
  pinMode(ST4DEn, INPUT);
  pinMode(ST4DEs, INPUT);
#endif 
}

void checkST4()
{
#if HASST4
  //Simulated ST4 with inactive signals
  byte w1 = HIGH, w2 = HIGH, e1 = HIGH, e2 = HIGH, n1 = HIGH, n2 = HIGH, s1 = HIGH, s2 = HIGH;
  static char ST4RA_state = 0;
  static char ST4RA_last = 0;
  static char ST4DE_state = 0;
  static char ST4DE_last = 0;
  // ST4 port is active only if there is no mount Error
  if (lastError != ERRT_NONE)
  {
    if (GuidingState == GuidingST4)
    {
      StopAxis1();
      StopAxis2();
    }
    return;
  }

  w1 = digitalRead(ST4RAw);
  e1 = digitalRead(ST4RAe);
  n1 = digitalRead(ST4DEn);
  s1 = digitalRead(ST4DEs);
  delayMicroseconds(5);
  w2 = digitalRead(ST4RAw);
  e2 = digitalRead(ST4RAe);
  n2 = digitalRead(ST4DEn);
  s2 = digitalRead(ST4DEs);
  //ST4 correction are reverse when the mount is West side of pier as the image is rotated by 180°
  if ((w1 == w2) && (e1 == e2))
  {
    ST4RA_state = 0;
    if (w1 != e1)
    {
      if (w1 == LOW)
        ST4RA_state = GetPoleSide() == POLE_UNDER ? '+' : '-';
      if (e1 == LOW)
        ST4RA_state = GetPoleSide() == POLE_UNDER ? '-' : '+';
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

  // RA changed?
  if (ST4RA_last != ST4RA_state)
  {
    ST4RA_last = ST4RA_state;
    if (ST4RA_state)
    {
      MoveAxis1(ST4RA_state == '-', Guiding::GuidingST4);
    }
    else
    {
      StopAxis1();
    }
  }

  // Dec changed?
  if (ST4DE_last != ST4DE_state)
  {
    ST4DE_last = ST4DE_state;
    if (ST4DE_state)
    {
      MoveAxis2(ST4DE_state == '-', Guiding::GuidingST4);
    }
    else
    {
      StopAxis2();
    }
  }
#endif
}


