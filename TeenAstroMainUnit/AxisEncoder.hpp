
#ifndef _EncoderAxis_h
#define _EncoderAxis_h

#if defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY_MICROMOD) || ARDUINO_TEENSY32
#include <Encoder.h>

class EncoderAxis
{
public:
	unsigned int gear;
	bool isGearFix;
	unsigned int pulseRot;
	bool isPulseRotFix;
	bool reverse;
	bool isReverseFix;
 private:
	 double m_pulse2degree = 0.0001;
	 Encoder* m_ecd = NULL;
 public:
	 bool connected()
	 {
		 return m_ecd != NULL;
	 }
	 void init(int pinA, int pinB)
	 {
		 m_ecd = new Encoder(pinA, pinB);
	 };
	 void updateRatio()
	 {
		 m_pulse2degree = reverse ? -360. / (gear * pulseRot) : 360. / (gear * pulseRot);
	 };
	 void r_deg(double& deg)
	 {
		deg = connected() ? (double)(m_ecd->read()) * m_pulse2degree : 0;  
	 };
	 void w_deg(const double deg)
	 {
     if (connected())
     {
       m_ecd->write(deg / m_pulse2degree);
     }
	 };
};


#endif

#endif
