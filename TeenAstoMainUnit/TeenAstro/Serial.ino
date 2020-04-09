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


boolean Serial_available()
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

boolean Serial1_available()
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

// -----------------------------------------------------------------------------------
// Simple soft SPI routines (CPOL=1, CPHA=1)
int _cs = 0;
int _sck = 0;
int _miso = 0;
int _mosi = 0;

void spiStart(int cs, int sck, int miso, int mosi)
{
    _cs = cs;
    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);
    delayMicroseconds(1);
    _sck = sck;
    pinMode(_sck, OUTPUT);
    digitalWrite(_sck, HIGH);
    _miso = miso;
    pinMode(_miso, INPUT);
    _mosi = mosi;
    pinMode(_mosi, OUTPUT);
    digitalWrite(cs, LOW);
    delayMicroseconds(1);
}

void spiEnd()
{
    delayMicroseconds(1);
    digitalWrite(_cs, HIGH);
}

void spiPause()
{
    digitalWrite(_cs, HIGH);
    delayMicroseconds(1);
    digitalWrite(_cs, LOW);
    delayMicroseconds(1);
}

uint8_t spiTransfer(uint8_t data_out)
{
    uint8_t data_in = 0;

    for (int i = 7; i >= 0; i--)
    {
        digitalWrite(_sck, LOW);
        digitalWrite(_mosi, bitRead(data_out, i));
        delayMicroseconds(1);
        digitalWrite(_sck, HIGH);
        bitWrite(data_in, i, digitalRead(_miso));
        delayMicroseconds(1);
    }

    return data_in;
}

uint32_t spiTransfer32(uint32_t data_out)
{
    uint32_t    data_in = 0;

    for (int i = 31; i >= 0; i--)
    {
        digitalWrite(_sck, LOW);
        digitalWrite(_mosi, bitRead(data_out, i));
        delayMicroseconds(1);
        digitalWrite(_sck, HIGH);
        bitWrite(data_in, i, digitalRead(_miso));
        delayMicroseconds(1);
    }

    return data_in;
}
