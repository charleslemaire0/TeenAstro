#pragma once
class timerLoop
{
private:
  long                    m_this_micros = 0;
  long                    m_time = 0;
  long                    m_last_micros = 0;       // workload monitoring
  long                    m_worst_time = 0;
public:
  void monitor()
  {
    m_this_micros = micros();
    m_time = m_this_micros - m_last_micros;
    if (m_time > m_worst_time) m_worst_time = m_time;
    m_last_micros = m_this_micros;
  }
  void update()
  {
    m_last_micros = micros();
  }
  void resetWorstTime()
  {
    m_worst_time = 0;
  }
  long getWorstTime()
  {
    return m_worst_time;
  }
};
static timerLoop tlp;