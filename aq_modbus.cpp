#include "aq_modbus.h"

// Pinos para controle de fluxo da serial half-duplex.
int           aq_modbus::PIN_RE_NEG;
int           aq_modbus::PIN_DE;

// Buffers das leituras modbus.
unsigned int  aq_modbus::CoilsBufferSize;
bool*         aq_modbus::CoilsBuffer;
unsigned int  aq_modbus::DiscreteBufferSize;
bool*         aq_modbus::DiscreteBuffer;
unsigned int  aq_modbus::InputRegBufferSize;
unsigned int *aq_modbus::InputRegBuffer;
unsigned int  aq_modbus::HoldingRegBufferSize;
unsigned int *aq_modbus::HoldingRegBuffer;

// Mapa modbus
unsigned char  aq_modbus::CoilsFilledSlots;
unsigned int*  aq_modbus::CoilsEnderecos;
unsigned char* aq_modbus::CoilsQuantidades;
unsigned char  aq_modbus::DiscreteFilledSlots;
unsigned int*  aq_modbus::DiscreteEnderecos;
unsigned char* aq_modbus::DiscreteQuantidades;
unsigned char  aq_modbus::InputRegFilledSlots;
unsigned int*  aq_modbus::InputRegEnderecos;
unsigned char* aq_modbus::InputRegQuantidades;
unsigned char  aq_modbus::HoldingRegFilledSlots;
unsigned int*  aq_modbus::HoldingRegEnderecos;
unsigned char* aq_modbus::HoldingRegQuantidades;

void aq_modbus::setupModbus(unsigned char modeloCLP, unsigned char slaveID, int re_neg, int de)
{
	// Preenche o mapa modbus de acordo com o modelo do CLP.
	loadModbusMap(modeloCLP);

	// Salva o numero nos pinos.
	PIN_RE_NEG = re_neg;
	PIN_DE     = de;

	// Pinos de controle da RS485 half-duplex em modo de saída.
	pinMode(PIN_RE_NEG, OUTPUT);
	pinMode(PIN_DE, OUTPUT);

	// Inicializa a RS485 em modo de leitura.
	digitalWrite(PIN_RE_NEG, 0);
	digitalWrite(PIN_DE, 0);

	// Callbacks allow us to configure the RS485 transceiver correctly
	node.preTransmission(aq_modbus::preTransmission);
	node.postTransmission(aq_modbus::postTransmission);

	//
	// Alocação dos buffers para armazenar o valor das leituras.
	//

	// Coils.
	CoilsBuffer     = NULL;
	CoilsBufferSize = 0;
	for (int i = 0; i < CoilsFilledSlots; i++)
		CoilsBufferSize += CoilsQuantidades[i];
	if (CoilsBufferSize > 0)
		CoilsBuffer = new bool[CoilsBufferSize];

	// Discrete.
	DiscreteBuffer     = NULL;
	DiscreteBufferSize = 0;
	for (int i = 0; i < DiscreteFilledSlots; i++)
		DiscreteBufferSize += DiscreteQuantidades[i];
	if (DiscreteBufferSize > 0)
		DiscreteBuffer = new bool[DiscreteBufferSize];

	// Input Registers.
	InputRegBuffer     = NULL;
	InputRegBufferSize = 0;
	for (int i = 0; i < InputRegFilledSlots; i++)
		InputRegBufferSize += InputRegQuantidades[i];
	if (InputRegBufferSize > 0)
		InputRegBuffer = new unsigned int[InputRegBufferSize];

	// Holding Registers.
	HoldingRegBuffer     = NULL;
	HoldingRegBufferSize = 0;
	for (int i = 0; i < HoldingRegFilledSlots; i++)
		HoldingRegBufferSize += HoldingRegQuantidades[i];
	if (HoldingRegBufferSize > 0)
		HoldingRegBuffer = new unsigned int[HoldingRegBufferSize];

	  // Inicializa o Modbus.
	node.begin(slaveID, Serial);
}

void aq_modbus::preTransmission()
{
	digitalWrite(PIN_RE_NEG, 1);
	digitalWrite(PIN_DE, 1);
}

void aq_modbus::postTransmission()
{
	digitalWrite(PIN_RE_NEG, 0);
	digitalWrite(PIN_DE, 0);
}

bool aq_modbus::readModbus(bool debugSerial)
{
	// Libera dados anteriores do buffer da serial e declara um índice usado nas leituras.
	Serial.flush();
	unsigned int index;

	//
	// Coils.
	//
	index = 0;
	for (unsigned char i = 0; i < CoilsFilledSlots; i++)
	{
		// Bloqueia por até 2 segundos em cada requisição.
		uint8_t result = node.readCoils(CoilsEnderecos[i] - 1, CoilsQuantidades[i]);
		if (result == node.ku8MBSuccess)
		{
			for (unsigned char j = 0; j < CoilsQuantidades[i]; j++)
				CoilsBuffer[index++] = (node.getResponseBuffer(j / 16) & (1UL << j)) ? true : false;
		}
		else
		{
			if (debugSerial)
				Serial.println("Erro na leitura modbus (coils).");
			return false;
		}
	}

	//
	// Discrete Input.
	//
	index = 0;
	for (unsigned char i = 0; i < DiscreteFilledSlots; i++)
	{
		// Bloqueia por até 2 segundos em cada requisição.
		uint8_t result = node.readDiscreteInputs(DiscreteEnderecos[i] - 10001, DiscreteQuantidades[i]);
		if (result == node.ku8MBSuccess)
		{
			for (unsigned char j = 0; j < DiscreteQuantidades[i]; j++)
				DiscreteBuffer[index++] = (node.getResponseBuffer(j / 16) & (1UL << j)) ? true : false;
		}
		else
		{
			if (debugSerial)
				Serial.println("Erro na leitura modbus (discrete inputs).");
			return false;
		}
	}

	//
	// Input Registers.
	//
	index = 0;
	for (unsigned char i = 0; i < InputRegFilledSlots; i++)
	{
		// Bloqueia por até 2 segundos em cada requisição.
		uint8_t result = node.readInputRegisters(InputRegEnderecos[i] - 30001, InputRegQuantidades[i]);
		if (result == node.ku8MBSuccess)
		{
			for (unsigned char j = 0; j < InputRegQuantidades[i]; j++)
				InputRegBuffer[index++] = node.getResponseBuffer(j);
		}
		else
		{
			if (debugSerial)
				Serial.println("Erro na leitura modbus (input register).");
			return false;
		}
	}

	//
	// Holding Registers.
	//
	index = 0;
	for (unsigned char i = 0; i < HoldingRegFilledSlots; i++)
	{
		// Bloqueia por até 2 segundos em cada requisição.
		uint8_t result = node.readHoldingRegisters(HoldingRegEnderecos[i] - 40001, HoldingRegQuantidades[i]);
		if (result == node.ku8MBSuccess)
		{
			for (unsigned char j = 0; j < HoldingRegQuantidades[i]; j++)
				HoldingRegBuffer[index++] = node.getResponseBuffer(j);
		}
		else
		{
			if (debugSerial)
				Serial.println("Erro na leitura modbus (holding register).");
			return false;
		}
	}

	  // Final com sucesso.
	if (debugSerial)
		Serial.println("Sucesso na leitura modbus");
	return true;
}

void aq_modbus::loadModbusMap(unsigned char modeloCLP)
{
	CoilsEnderecos        = NULL;
	CoilsQuantidades      = NULL;
	DiscreteEnderecos     = NULL;
	DiscreteQuantidades   = NULL;
	InputRegEnderecos     = NULL;
	InputRegQuantidades   = NULL;
	HoldingRegEnderecos   = NULL;
	HoldingRegQuantidades = NULL;
	CoilsFilledSlots      = 0;
	DiscreteFilledSlots   = 0;
	InputRegFilledSlots   = 0;
	HoldingRegFilledSlots = 0;

	// Os endereços começam:
	// Coils:                 1 - ...
	// Discrete Input:    10001 - ...
	// Input Registers:   30001 - ...
	// Holding Registers: 40001 - ...
	switch (modeloCLP)
	{
		case 0:
		{
			// PCC1301/PCC1302.

			// Coils.
			//CoilsFilledSlots  = 0;
			//CoilsEnderecos    = (unsigned int*)malloc(CoilsFilledSlots  * sizeof(unsigned int));
			//CoilsQuantidades  = (unsigned char*)malloc(CoilsFilledSlots * sizeof(unsigned char));
			//CoilsEnderecos[0] = 1; CoilsQuantidades[0] = 1;

			// Discrete Input.
			//DiscreteFilledSlots  = 0;
			//DiscreteEnderecos    = (unsigned int*)malloc(DiscreteFilledSlots  * sizeof(unsigned int));
			//DiscreteQuantidades  = (unsigned char*)malloc(DiscreteFilledSlots * sizeof(unsigned char));
			//DiscreteEnderecos[0] = 1; DiscreteQuantidades[0] = 1;

			// Input Registers.
			//InputRegFilledSlots  = 0;
			//InputRegEnderecos    = (unsigned int*)malloc(InputRegFilledSlots  * sizeof(unsigned int));
			//InputRegQuantidades  = (unsigned char*)malloc(InputRegFilledSlots * sizeof(unsigned char));
			//InputRegEnderecos[0] = 1; InputRegQuantidades[0] = 1;

			// Holding Registers.
			HoldingRegFilledSlots  = 7;
			HoldingRegEnderecos    = (unsigned int*)malloc(HoldingRegFilledSlots  * sizeof(unsigned int));
			HoldingRegQuantidades  = (unsigned char*)malloc(HoldingRegFilledSlots * sizeof(unsigned char));
			HoldingRegEnderecos[0] = 40010; HoldingRegQuantidades[0] = 4;
			HoldingRegEnderecos[1] = 40018; HoldingRegQuantidades[1] = 3;
			HoldingRegEnderecos[2] = 40022; HoldingRegQuantidades[2] = 8;
			HoldingRegEnderecos[3] = 40040; HoldingRegQuantidades[3] = 5;
			HoldingRegEnderecos[4] = 40058; HoldingRegQuantidades[4] = 5;
			HoldingRegEnderecos[5] = 40064; HoldingRegQuantidades[5] = 1;
			HoldingRegEnderecos[6] = 40068; HoldingRegQuantidades[6] = 4;
			break;
		}
		default:
			break;
	}
}
