#ifndef AQ_MODBUS
#define AQ_MODBUS

#include <ModbusMaster.h>
#include "aq_eeprom.h"

class aq_modbus
{
private:
	ModbusMaster         node; // Modbus RTU Master.
	static int           PIN_RE_NEG;
	static int           PIN_DE;

	// Mapa modbus, diferente para cada comedo de CLP.
	static unsigned char  CoilsFilledSlots;
	static unsigned int*  CoilsEnderecos;
	static unsigned char* CoilsQuantidades;
	static unsigned char  DiscreteFilledSlots;
	static unsigned int*  DiscreteEnderecos;
	static unsigned char* DiscreteQuantidades;
	static unsigned char  InputRegFilledSlots;
	static unsigned int*  InputRegEnderecos;
	static unsigned char* InputRegQuantidades;
	static unsigned char  HoldingRegFilledSlots;
	static unsigned int*  HoldingRegEnderecos;
	static unsigned char* HoldingRegQuantidades;

public:
	static unsigned int  CoilsBufferSize;
	static bool*         CoilsBuffer;
	static unsigned int  DiscreteBufferSize;
	static bool*         DiscreteBuffer;
	static unsigned int  InputRegBufferSize;
	static unsigned int* InputRegBuffer;
	static unsigned int  HoldingRegBufferSize;
	static unsigned int* HoldingRegBuffer;

	void setupModbus(unsigned char modeloCLP, unsigned char slaveID, int re_neg, int de);
	bool readModbus(bool debugSerial);

private:
	static void preTransmission();
	static void postTransmission();
	void        loadModbusMap(unsigned char modeloCLP);
};
#endif