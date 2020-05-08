// -----------------------------------------------------------------------------------
// Communication routines for Serial0 and Serial1
// these are more compact and faster than the Arduino provided one's

void Serial_Init(unsigned long baud)
{
  Serial.begin(baud);
}

void Serial_send(const char data[])
{
  Serial.print(data);
}

void Serial_print(const char data[])
{
  Serial.print(data);
}

bool Serial_available()
{
  return Serial.available();
}

char Serial_read()
{
  return Serial.read();
}

void Serial1_Init(unsigned long baud)
{
  Serial1.begin(baud);
}

void Serial1_send(const char data[])
{
  Serial1_print(data);
}

void Serial1_print(const char data[])
{
  Serial1.print(data);
}

bool Serial1_available()
{
  return Serial1.available();
}

char Serial1_read()
{
  return Serial1.read();
}

void Serial2_Init(unsigned long baud)
{
  Serial2.setRX(FocuserRX);
  Serial2.setTX(FocuserTX);
  Serial2.begin(baud);
  Serial2.setTimeout(10);
}
