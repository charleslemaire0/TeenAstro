# MotorDriver library



MotorDriver allows driving a TMC5160 stepper controller in either Step/Dir mode or Motion Controller (SPI-only) mode. It automatically selects the driver according to the SD_MODE bit in the IOIN register that reflects the hardware input on the board.

It requires FreeRTOS, since the API needs a task for the StepDir driver (not for the Motion Controller which is very simple).

See the debug5160 monitor program for an example of how to use it.







