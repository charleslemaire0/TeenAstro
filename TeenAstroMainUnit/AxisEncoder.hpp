
#ifndef _EncoderAxis_h
#define _EncoderAxis_h

#if defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY_MICROMOD) || defined(ARDUINO_TEENSY31)
#include <Encoder.h>

class EncoderAxis
{
public:
	double pulsePerDegree;
	bool isPulsePerDegreeFix;
	bool reverse;
	bool isReverseFix;
 private:

	 //members for calibration
	 bool has_Ref = false;
	 double deg_T_Ref;
	 long pulse_E_Ref;

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
	 long readencoder()
	 {
		 return m_ecd->read();
	 }
	 double r_deg()
	 {
		 double deg = 0;
		 if (!connected())
			 return deg;
		 deg = readencoder() / (double)pulsePerDegree;
		 if (reverse)
			 deg *= -1;
		return deg;  
	 };
	 void w_deg(const double deg)
	 {
		if (connected())
     {
      if (reverse)
      {
        m_ecd->write(- deg * pulsePerDegree);
      }
      else
      {
        m_ecd->write( deg * pulsePerDegree);
			}
     }
	 };

	 void setRef(const double deg_T)
	 {
		 deg_T_Ref = deg_T;
		 pulse_E_Ref = readencoder();
		 has_Ref = true;
	 }
	 void delRef()
	 {
		 has_Ref = false;
	 }
	 bool calibrating()
	 {
		 return has_Ref;
	 }
	 bool calibrate(const double deg_T)
	 {
		 if (!has_Ref)
			 return  false;
		 double dP = readencoder() - pulse_E_Ref;
		 double dD = deg_T - deg_T_Ref;
		 if (dP == 0 || dD == 0)
			 return false;
		 reverse = (dP > 0) != (dD > 0);
		 pulsePerDegree =  abs(dP / dD);
		 return true;
	 }
	 double deltaTarget(const double deg_Target)
	 {
		 return deg_Target - r_deg();
	 };

};


#endif

#endif
