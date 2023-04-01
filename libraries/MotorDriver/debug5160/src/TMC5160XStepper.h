class TMC5160XStepper : public TMC5160Stepper {
	public:
		TMC5160XStepper(uint16_t pinCS, float RS = default_RS, int8_t link_index = -1);
		uint32_t FACTORY_CONF(void);

};

