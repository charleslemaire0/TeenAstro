// 
// 
// 
#include "Global.h"
#include "ConfigStepper.h"
#include "Command.h"

#define INPUT_SIZE 30

void Command_move(int sign, double& t)
{
	unsigned int nextposition = storage.reverse ? position - sign : position + sign;
	if (inlimit(nextposition))
	{
		Go(storage.minSpeed, sign, t);
	}
	else
	{
		currSpeed = 0;
		t = 0;
	}
}
void Command_stop(int sign)
{
	Stop(sign);	
}

void Command_Check(void)
{
	long value = 0;
	if (Serial.available() > 0)
	{
		// Get next command from Serial (add 1 for final 0)
		char input[INPUT_SIZE + 1];

		byte size = Serial.readBytes(input, INPUT_SIZE);
		// Add the final 0 to end the C string
		input[size] = 0;

		// Read each command pair
		char command = input[0];
		char* separator = strchr(input, ' ');
		bool valuedefined = false;
		if (separator != 0)
		{
			separator++;
			valuedefined = true;
			value = atoi(separator);
		}
		switch (command)
		{
		case AzCmd_Help:
			sayHello();
			Serial.println("$ Commands");
			Serial.println("$ h Help, g Goto, p Park, s Sync, w Write, ?");
			Serial.println("$ Settings");
			Serial.println("$ 0 startP, 1 maxP, 2 minS , 3 maxS, 4 cmdAcc, 5 mAcc, 6 mDec");
			break;
		case AzCmd_Version:
			sayHello();
			break;
		case FocCmd_Goto:
			if (valuedefined)
			{
				MoveTo(value);
				halt = false;
			}
			break;
		case FocCmd_Park:
			MoveTo(storage.startPosition);
			halt = false;
			break;
		case FocCmd_Sync:      // "reset" position
			setvalue(valuedefined, value, 0, storage.maxPosition, position);
			writePos();
			break;
		case FocCmd_Write:
			saveConfig();
			Serial.println("1");
			break;
		case FocCmd_startPosition:
			setvalue(valuedefined, value, 0, 65535, storage.startPosition);
			break;
		case FocCmd_maxPosition:
			setvalue(valuedefined, value, 0, 65535, storage.maxPosition);
			break;
		case FocCmd_maxSpeed:
			setvalue(valuedefined, value, 1, 100, storage.maxSpeed);
			break;
		case FocCmd_minSpeed:
			setvalue(valuedefined, value, 1, 100, storage.minSpeed);
			break;
		case FocCmd_cmdAcc:
			setvalue(valuedefined, value, 1, 10000, storage.cmdAcc);
			break;
		case FocCmd_manualAcc:
			setvalue(valuedefined, value, 1, 10000, storage.manAcc);
			break;
		case FocCmd_manualDec:
			setvalue(valuedefined, value, 1, 10000, storage.manDec);
			break;
		case FocCmd_Inv:
			setbool(valuedefined, value, storage.reverse);
			break;
		case CmdDumpState: // "?" dump state including details
			dumpState();
			break;
		case CmdDumpConfig:
			dumpConfig();
			break;
		case Char_CR:  // ignore cr
		case Char_Spc:
		case Char_254:
			break;
		default:
			Serial.print("$rcvd: ");
			Serial.println(size, DEC);
		}
	}

}

void HaltRequest(void)
{
	long value = 0;
	if (Serial.available() > 0)
	{
		// Get next command from Serial (add 1 for final 0)
		char input[INPUT_SIZE + 1];

		byte size = Serial.readBytes(input, INPUT_SIZE);
		// Add the final 0 to end the C string
		input[size] = 0;
		// Read each command pair 
		char command = input[0];
		char* separator = strchr(input, ' ');
		bool valuedefined = false;
		if (separator != 0)
		{
			separator++;
			valuedefined = true;
			value = atoi(separator);
		}
		switch (command)
		{
		case FocCmd_Halt:
			halt = true;			
			return;
		default:
			return;
		}
	}
  lastEvent = millis() / updaterate + 2;
}

void setvalue(bool valuedefined, long value, unsigned int min, unsigned int max, unsigned int &adress)
{
	if (valuedefined && value >= min && value <= max)
	{
		adress = value;
		Serial.println("1");
	}
	else
		Serial.println("0");
  lastEvent = millis() / updaterate + 2;
}

void setbool(bool valuedefined, long value, bool  &adress)
{
	if (valuedefined && value == 0 || value == 1)
	{
		adress = value;
		Serial.println("1");
	}
	else
		Serial.println("0");
  lastEvent = millis() / updaterate + 2;
}

void dumpConfig()
{
	char buf[10];
	Serial.print("~");
	sprintf(buf, "%06d ", storage.startPosition);
	Serial.print(buf);
	sprintf(buf, "%06d ", storage.maxPosition);
	Serial.print(buf);
	sprintf(buf, "%03d ", storage.minSpeed);
	Serial.print(buf);
	sprintf(buf, "%03d ", storage.maxSpeed);
	Serial.print(buf);
	sprintf(buf, "%04d ", storage.cmdAcc);
	Serial.print(buf);
	sprintf(buf, "%04d ", storage.manAcc);
	Serial.print(buf);
	sprintf(buf, "%04d ", storage.manDec);
	Serial.print(buf);
	sprintf(buf, "%1d", storage.reverse);
	Serial.print(buf);
	Serial.println();
  lastEvent = millis() / updaterate + 2;
}

void dumpState()
{
	char buf[10];
	Serial.print("?");
	sprintf(buf, "%06d ", position);
	Serial.print(buf);
	sprintf(buf, "%06d ", currSpeed);
	Serial.print(buf);
	Serial.println();
  lastEvent = millis() / updaterate + 2;
}

void updateGoto(void)
{
	event = millis() / updaterate;
	if (position != lastposition && lastEvent < event)
	{
		dumpState();
		HaltRequest();
		lastEvent = event;
		lastposition = position;
		lastCurrSpeed = currSpeed;
	}

}

void update(void)
{
	event = millis() / updaterate;
	if (lastEvent < event)
	{
		dumpState();
		lastEvent = event;
		lastposition = position;
		lastCurrSpeed = currSpeed;
  }
}