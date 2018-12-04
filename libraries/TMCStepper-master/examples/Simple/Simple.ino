/**
 * Author Teemu Mäntykallio
 * Initializes the library and runs the stepper motor.
 */

#include <TMCStepper.h>

#define EN_PIN    38  // Enable
#define DIR_PIN   55  // Direction
#define STEP_PIN  54  // Step
#define CS_PIN    42  // Chip select
#define SW_MOSI   66  // Software Master Out Slave In (MOSI)
#define SW_MISO   44  // Software Master In Slave Out (MISO)
#define SW_SCK    64  // Software Slave Clock (SCK)
#define SW_RX     63  // TMC2208/TMC2224 SoftwareSerial receive pin
#define SW_TX     40  // TMC2208/TMC2224 SoftwareSerial transmit pin

#define R_SENSE 0.11 // Match to your driver
                     // SilentStepStick series use 0.11
                     // UltiMachine Einsy and Archim2 boards use 0.2
                     // Panucatt BSD2660 uses 0.1

// Select your stepper driver type
//TMC2130Stepper driver = TMC2130Stepper(CS_PIN, R_SENSE); // Hardware SPI
//TMC2130Stepper driver = TMC2130Stepper(CS_PIN, R_SENSE, SW_MOSI, SW_MISO, SW_SCK); // Software SPI
//TMC2208Stepper driver = TMC2208Stepper(&Serial, R_SENSE); // Hardware Serial0
//TMC2208Stepper driver = TMC2208Stepper(SW_RX, SW_TX, R_SENSE); // Software serial
//TMC2660Stepper driver = TMC2660Stepper(EN_PIN, R_SENSE); // Hardware SPI
//TMC2660Stepper driver = TMC2660Stepper(EN_PIN, R_SENSE, SW_MOSI, SW_MISO, SW_SCK); // Software SPI
//TMC5160Stepper driver = TMC5160Stepper(CS_PIN, R_SENSE);

void setup() {
  SPI.begin();
  pinMode(EN_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);

  driver.begin();
  driver.toff(2);
  driver.tbl(24);
  driver.microsteps(16);
  driver.rms_current(800);
}

void loop() {
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(160);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(160);
}
