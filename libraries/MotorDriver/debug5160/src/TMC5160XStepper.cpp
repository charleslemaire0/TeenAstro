#include "TMCStepper.h"
#include "TMC5160XStepper.h"

#define FACTORY_CONF_ADDR 0x8

TMC5160XStepper::TMC5160XStepper(uint16_t pinCS, float RS, int8_t link) : TMC5160Stepper(pinCS, RS, link)
  { defaults(); }

uint32_t TMC5160XStepper::FACTORY_CONF(void)
{ 
	return read(FACTORY_CONF_ADDR);
}  