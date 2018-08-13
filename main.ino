// ******************************************************************************************
// * Código de LED:                                                                         *
// * Apagado: desligado ou em intervalo de leitura.                                         *
// * Aceso: Processando.                                                                    *
// * Piscando rapidamente: Entrando em modo de configuração. Permanece aceso após entrar.   *
// * Piscando 90% do tempo apagado: Problema ao ler modbus.                                 *
// * Piscando 90% do tempo aceso: Problema ao conectar no WIFI, no MQTT ou ao enviar dados. *
// *                                                                                        *
// * Para adicionar ou editar mapas modbus, é preciso modificar as funcções:                *
// * - "loadModbusMap" no arquivo "aq_modbus.cpp".                                          *
// * - "handleWiFi" (linha 93) e "handleSave" (linha 175) no arquivo "aq_http.cpp".         *
// * - "MQTTPublish" e "MQTTPublishDS" neste arquivo.                                       *
// ******************************************************************************************

// Defines.
#define DebugSerial true    // Plota mensagens na porta serial (o modbus RTU deixa de funcionar).
#define REVISAO_FW  "1.0.0"  // Versão do firmware.

// Pinagem.
//#define PIN_TX       1   // TX
//#define PIN_RX       3   // RX
#define MODO_CONFIG    16  // D0 - Botão para iniciar modo de configuração via Wifi.
#define MAX485_RE_NEG  5   // D1
#define MAX485_DE      4   // D2
//#define GPIO_0       0   // D3 - Botão para gravação de firmware (modo Flash).
#define AQ_LED_BUILTIN 2   // D4 - LED BUILTIN.
//#define SIM800_TX    14  // D5
//#define SIM800_RX    12  // D6
//#define SIM800_RST   13  // D7
//#define AQ_LED_EXT   15  // D8 - LED externo.

// Bibliotecas utilizadas.
#include <pgmspace.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <ModbusMaster.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "aq_wifi.h"
#include "aq_http.h"
#include "aq_modbus.h"
#include "aq_eeprom.h"

// Objetos utilizados no programa.
aq_wifi      *AQ_wifi;
aq_http      *AQ_http;
aq_modbus     AQ_modbus;
WiFiClient    EspClient;
PubSubClient  MQTT(EspClient);

// Atributos globais.
const char*   MQTTBroker      = "m10.cloudmqtt.com";
unsigned int  MQTTPort        = 10770;
String        Serial_Hardware = String(ESP.getChipId(), HEX).c_str();
unsigned long IntervaloLoopms = 10000;
String        usuarioMQTT, senhaMQTT;

// Assinatura das funções globais deste arquivo.
void          piscarLED(int numPiscadas, int tempoApagado, int tempoAceso);
unsigned long calcProcessTime(unsigned long& startTime);
bool          MQTTConnect();
bool          MQTTPublish();
bool          MQTTPublishDS();
void          SetupSerial();
void          DelayKeppingMQTT(unsigned long tempo);

void DelayKeppingMQTT(unsigned long tempo)
{
	for (unsigned char i = 0; i < 10; i++)
	{
		MQTT.loop();
		delay(tempo / 10);
	}
	MQTT.loop();
	delay(tempo % 10);
}

void piscarLED(int numPiscadas, int tempoApagado, int tempoAceso)
{
	// Pisca para indicar o erro.
	for (int i = 0; i < numPiscadas; i++)
	{
		MQTT.loop();
		digitalWrite(AQ_LED_BUILTIN, HIGH);
		delay(tempoApagado);
		MQTT.loop();
		digitalWrite(AQ_LED_BUILTIN, LOW);
		delay(tempoAceso);
	}
}

unsigned long calcProcessTime(unsigned long& startTime)
{
	unsigned long endTime = millis();
	if (endTime >= startTime)
		return (endTime - startTime);
	return (endTime + (4294967295UL - startTime));
}

bool MQTTConnect()
{
	if (DebugSerial)
	{
		Serial.print("Connecting to ");
		Serial.print(MQTTBroker);
	}
	
	if (!MQTT.connect(Serial_Hardware.c_str(), usuarioMQTT.c_str(), senhaMQTT.c_str()))
	{
		if (DebugSerial)
			Serial.println(" fail");
		return false;
	}
	if (DebugSerial)
		Serial.println(" OK");
	return MQTT.connected();
}

bool MQTTPublish()
{
	// Aqui é possível formatar as leituras do jeito que quiser antes de transmitir, pois cada CLP pode ser tratado de forma diferente.
	switch (aq_eeprom::ModeloCLP)
	{
		case 0:
		{
			// PCC1301/PCC1302.

			//if (!MQTT.publish((Serial_Hardware + "/asd1/valor").c_str(),           String(aq_modbus::CoilsBuffer[0]).c_str(),                                    true)) return false;
			//if (!MQTT.publish((Serial_Hardware + "/dfh2/valor").c_str(),           String(aq_modbus::DiscreteBuffer[0]).c_str(),                                 true)) return false;
			//if (!MQTT.publish((Serial_Hardware + "/rty3/valor").c_str(),           String((double)((long)aq_modbus::InputRegBuffer[0] - 32768)).c_str(),         true)) return false;

			if (!MQTT.publish((Serial_Hardware + "/GCROASP/valor").c_str(),        String((double)((long)aq_modbus::HoldingRegBuffer[0]  - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/OpState/valor").c_str(),        String((double)((long)aq_modbus::HoldingRegBuffer[1]  - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/FaultCode/valor").c_str(),      String((double)((long)aq_modbus::HoldingRegBuffer[2]  - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/FaultType/valor").c_str(),      String((double)((long)aq_modbus::HoldingRegBuffer[3]  - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/Va/valor").c_str(),             String((double)((long)aq_modbus::HoldingRegBuffer[4]  - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/Vb/valor").c_str(),             String((double)((long)aq_modbus::HoldingRegBuffer[5]  - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/Vc/valor").c_str(),             String((double)((long)aq_modbus::HoldingRegBuffer[6]  - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/Vab/valor").c_str(),            String((double)((long)aq_modbus::HoldingRegBuffer[7]  - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/Vbc/valor").c_str(),            String((double)((long)aq_modbus::HoldingRegBuffer[8]  - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/Vca/valor").c_str(),            String((double)((long)aq_modbus::HoldingRegBuffer[9]  - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/AOVUFVR/valor").c_str(),        String((double)((long)aq_modbus::HoldingRegBuffer[10] - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/IaAmps/valor").c_str(),         String((double)((long)aq_modbus::HoldingRegBuffer[11] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/IbAmps/valor").c_str(),         String((double)((long)aq_modbus::HoldingRegBuffer[12] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/IcAmps/valor").c_str(),         String((double)((long)aq_modbus::HoldingRegBuffer[13] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/AOAC/valor").c_str(),           String((double)((long)aq_modbus::HoldingRegBuffer[14] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpA/valor").c_str(),       String((double)((long)aq_modbus::HoldingRegBuffer[15] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpB/valor").c_str(),       String((double)((long)aq_modbus::HoldingRegBuffer[16] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpC/valor").c_str(),       String((double)((long)aq_modbus::HoldingRegBuffer[17] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpTotal/valor").c_str(),   String((double)((long)aq_modbus::HoldingRegBuffer[18] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/Freq/valor").c_str(),           String((double)((long)aq_modbus::HoldingRegBuffer[19] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/IaPC/valor").c_str(),           String((double)((long)aq_modbus::HoldingRegBuffer[20] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/IbPC/valor").c_str(),           String((double)((long)aq_modbus::HoldingRegBuffer[21] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/IcPC/valor").c_str(),           String((double)((long)aq_modbus::HoldingRegBuffer[22] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/ControlBatVolt/valor").c_str(), String((double)((long)aq_modbus::HoldingRegBuffer[23] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/EOP/valor").c_str(),            String((double)((long)aq_modbus::HoldingRegBuffer[24] - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/ECT/valor").c_str(),            String((double)((long)aq_modbus::HoldingRegBuffer[25] - 32768) / 10).c_str(), true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/EngSpeed/valor").c_str(),       String((double)((long)aq_modbus::HoldingRegBuffer[26] - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/NumEngStarts/valor").c_str(),   String((double)((long)aq_modbus::HoldingRegBuffer[27] - 32768)).c_str(),      true)) return false;
			if (!MQTT.publish((Serial_Hardware + "/EngRuntime/valor").c_str(),     String((double)(
				((long)aq_modbus::HoldingRegBuffer[28] - 32768) * 9.10222222222 + ((long)aq_modbus::HoldingRegBuffer[29] - 32768) * 0.0002777777778)).c_str(),   true)) return false;
			break;
		}
		default:
			return false;
	}

	// Sucesso.
	return true;
}

bool MQTTPublishDS()
{
	switch (aq_eeprom::ModeloCLP)
	{
		case 0:
		{
			// PCC1301/PCC1302.
			if (!MQTT.publish((Serial_Hardware + "/ds/ModeloCLP").c_str(), "PCC1301/PCC1302", true))  return false;

			// GCROASP
			if (!MQTT.publish((Serial_Hardware + "/GCROASP/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/GCROASP/descricao").c_str(),  "Modo de operação", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/GCROASP/tipodado").c_str(),   "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/GCROASP/máximo").c_str(),     "5", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/GCROASP/mínimo").c_str(),     "0", true))  return false;

			// OpState
			if (!MQTT.publish((Serial_Hardware + "/OpState/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/OpState/descricao").c_str(),  "Estado de operação", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/OpState/tipodado").c_str(),   "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/OpState/máximo").c_str(),     "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/OpState/mínimo").c_str(),     "0", true))  return false;

			// FaultCode
			if (!MQTT.publish((Serial_Hardware + "/FaultCode/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/FaultCode/descricao").c_str(),  "Código de falha", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/FaultCode/tipodado").c_str(),   "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/FaultCode/máximo").c_str(),     "65535", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/FaultCode/mínimo").c_str(),     "0", true))  return false;

			// FaultType
			if (!MQTT.publish((Serial_Hardware + "/FaultType/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/FaultType/descricao").c_str(),  "Tipo de falha", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/FaultType/tipodado").c_str(),   "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/FaultType/máximo").c_str(),     "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/FaultType/mínimo").c_str(),     "0", true))  return false;

			// Va
			if (!MQTT.publish((Serial_Hardware + "/Va/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Va/descricao").c_str(),  "Tensão Va", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Va/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Va/grandeza").c_str(),   "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Va/unidade").c_str(),    "V", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Va/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Va/máximo").c_str(),     "300", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Va/mínimo").c_str(),     "0", true))  return false;

			// Vb
			if (!MQTT.publish((Serial_Hardware + "/Vb/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vb/descricao").c_str(),  "Tensão Vb", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vb/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vb/grandeza").c_str(),   "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vb/unidade").c_str(),    "V", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vb/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vb/máximo").c_str(),     "300", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vb/mínimo").c_str(),     "0", true))  return false;

			// Vc
			if (!MQTT.publish((Serial_Hardware + "/Vc/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vc/descricao").c_str(),  "Tensão Vc", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vc/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vc/grandeza").c_str(),   "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vc/unidade").c_str(),    "V", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vc/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vc/máximo").c_str(),     "300", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vc/mínimo").c_str(),     "0", true))  return false;

			// Vab
			if (!MQTT.publish((Serial_Hardware + "/Vab/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vab/descricao").c_str(),  "Tensão Vab", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vab/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vab/grandeza").c_str(),   "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vab/unidade").c_str(),    "V", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vab/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vab/máximo").c_str(),     "400", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vab/mínimo").c_str(),     "0", true))  return false;

			// Vbc
			if (!MQTT.publish((Serial_Hardware + "/Vbc/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vbc/descricao").c_str(),  "Tensão Vbc", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vbc/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vbc/grandeza").c_str(),   "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vbc/unidade").c_str(),    "V", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vbc/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vbc/máximo").c_str(),     "400", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vbc/mínimo").c_str(),     "0", true))  return false;

			// Vca
			if (!MQTT.publish((Serial_Hardware + "/Vca/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vca/descricao").c_str(),  "Tensão Vca", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vca/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vca/grandeza").c_str(),   "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vca/unidade").c_str(),    "V", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vca/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vca/máximo").c_str(),     "400", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Vca/mínimo").c_str(),     "0", true))  return false;

			// AOVUFVR
			if (!MQTT.publish((Serial_Hardware + "/AOVUFVR/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOVUFVR/descricao").c_str(),  "Tensão para regulação", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOVUFVR/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOVUFVR/grandeza").c_str(),   "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOVUFVR/unidade").c_str(),    "V", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOVUFVR/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOVUFVR/máximo").c_str(),     "400", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOVUFVR/mínimo").c_str(),     "0", true))  return false;

			// IaAmps
			if (!MQTT.publish((Serial_Hardware + "/IaAmps/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaAmps/descricao").c_str(),  "Corrente da fase A", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaAmps/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaAmps/grandeza").c_str(),   "1", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaAmps/unidade").c_str(),    "A", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaAmps/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaAmps/máximo").c_str(),     "200", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaAmps/mínimo").c_str(),     "0", true))  return false;

			// IbAmps
			if (!MQTT.publish((Serial_Hardware + "/IbAmps/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbAmps/descricao").c_str(),  "Corrente da fase B", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbAmps/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbAmps/grandeza").c_str(),   "1", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbAmps/unidade").c_str(),    "A", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbAmps/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbAmps/máximo").c_str(),     "200", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbAmps/mínimo").c_str(),     "0", true))  return false;

			// IcAmps
			if (!MQTT.publish((Serial_Hardware + "/IcAmps/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcAmps/descricao").c_str(),  "Corrente da fase C", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcAmps/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcAmps/grandeza").c_str(),   "1", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcAmps/unidade").c_str(),    "A", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcAmps/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcAmps/máximo").c_str(),     "200", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcAmps/mínimo").c_str(),     "0", true))  return false;

			// AOAC
			if (!MQTT.publish((Serial_Hardware + "/AOAC/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOAC/descricao").c_str(),  "Média das correntes", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOAC/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOAC/grandeza").c_str(),   "1", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOAC/unidade").c_str(),    "A", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOAC/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOAC/máximo").c_str(),     "200", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/AOAC/mínimo").c_str(),     "0", true))  return false;

			// VoltAmpA
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpA/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpA/descricao").c_str(),  "Potência aparente da fase A", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpA/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpA/grandeza").c_str(),   "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpA/unidade").c_str(),    "kVA", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpA/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpA/máximo").c_str(),     "100", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpA/mínimo").c_str(),     "0", true))  return false;

			// VoltAmpB
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpB/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpB/descricao").c_str(),  "Potência aparente da fase B", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpB/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpB/grandeza").c_str(),   "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpB/unidade").c_str(),    "kVA", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpB/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpB/máximo").c_str(),     "100", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpB/mínimo").c_str(),     "0", true))  return false;

			// VoltAmpC
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpC/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpC/descricao").c_str(),  "Potência aparente da fase C", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpC/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpC/grandeza").c_str(),   "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpC/unidade").c_str(),    "kVA", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpC/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpC/máximo").c_str(),     "100", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpC/mínimo").c_str(),     "0", true))  return false;

			// VoltAmpTotal
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpTotal/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpTotal/descricao").c_str(),  "Potência aparente total", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpTotal/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpTotal/grandeza").c_str(),   "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpTotal/unidade").c_str(),    "kVA", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpTotal/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpTotal/máximo").c_str(),     "100", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/VoltAmpTotal/mínimo").c_str(),     "0", true))  return false;

			// Freq
			if (!MQTT.publish((Serial_Hardware + "/Freq/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Freq/descricao").c_str(),  "Frequência", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Freq/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Freq/grandeza").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Freq/unidade").c_str(),    "Hz", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Freq/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Freq/máximo").c_str(),     "100", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/Freq/mínimo").c_str(),     "0", true))  return false;

			// IaPC
			if (!MQTT.publish((Serial_Hardware + "/IaPC/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaPC/descricao").c_str(),  "Corrente da fase A em percentual", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaPC/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaPC/grandeza").c_str(),   "1", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaPC/unidade").c_str(),    "%", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaPC/algarismos").c_str(), "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaPC/máximo").c_str(),     "100", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IaPC/mínimo").c_str(),     "0", true))  return false;

			// IbPC
			if (!MQTT.publish((Serial_Hardware + "/IbPC/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbPC/descricao").c_str(),  "Corrente da fase B em percentual", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbPC/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbPC/grandeza").c_str(),   "1", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbPC/unidade").c_str(),    "%", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbPC/algarismos").c_str(), "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbPC/máximo").c_str(),     "100", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IbPC/mínimo").c_str(),     "0", true))  return false;

			// IcPC
			if (!MQTT.publish((Serial_Hardware + "/IcPC/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcPC/descricao").c_str(),  "Corrente da fase C em percentual", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcPC/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcPC/grandeza").c_str(),   "1", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcPC/unidade").c_str(),    "%", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcPC/algarismos").c_str(), "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcPC/máximo").c_str(),     "100", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/IcPC/mínimo").c_str(),     "0", true))  return false;

			// ControlBatVolt
			if (!MQTT.publish((Serial_Hardware + "/ControlBatVolt/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ControlBatVolt/descricao").c_str(),  "Tensão da bateria", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ControlBatVolt/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ControlBatVolt/grandeza").c_str(),   "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ControlBatVolt/unidade").c_str(),    "V", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ControlBatVolt/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ControlBatVolt/máximo").c_str(),     "100", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ControlBatVolt/mínimo").c_str(),     "0", true))  return false;

			// EOP
			if (!MQTT.publish((Serial_Hardware + "/EOP/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EOP/descricao").c_str(),  "Pressão do óleo", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EOP/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EOP/grandeza").c_str(),   "6", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EOP/unidade").c_str(),    "kPa", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EOP/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EOP/máximo").c_str(),     "32767", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EOP/mínimo").c_str(),     "0", true))  return false;

			// ECT
			if (!MQTT.publish((Serial_Hardware + "/ECT/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ECT/descricao").c_str(),  "Temperatura da água", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ECT/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ECT/grandeza").c_str(),   "7", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ECT/unidade").c_str(),    "°C", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ECT/algarismos").c_str(), "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ECT/máximo").c_str(),     "200", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/ECT/mínimo").c_str(),     "0", true))  return false;

			// EngSpeed
			if (!MQTT.publish((Serial_Hardware + "/EngSpeed/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngSpeed/descricao").c_str(),  "Velocidade do motor", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngSpeed/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngSpeed/grandeza").c_str(),   "10", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngSpeed/unidade").c_str(),    "RPM", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngSpeed/algarismos").c_str(), "4", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngSpeed/máximo").c_str(),     "10000", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngSpeed/mínimo").c_str(),     "0", true))  return false;

			// NumEngStarts
			if (!MQTT.publish((Serial_Hardware + "/NumEngStarts/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/NumEngStarts/descricao").c_str(),  "Número de partidas", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/NumEngStarts/tipodado").c_str(),   "2", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/NumEngStarts/máximo").c_str(),     "32767", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/NumEngStarts/mínimo").c_str(),     "0", true))  return false;

			// EngRuntime
			if (!MQTT.publish((Serial_Hardware + "/EngRuntime/revisao").c_str(),    "0", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngRuntime/descricao").c_str(),  "Tempo de funcionamento", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngRuntime/tipodado").c_str(),   "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngRuntime/grandeza").c_str(),   "5", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngRuntime/unidade").c_str(),    "h", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngRuntime/algarismos").c_str(), "3", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngRuntime/máximo").c_str(),     "300000", true))  return false;
			if (!MQTT.publish((Serial_Hardware + "/EngRuntime/mínimo").c_str(),     "0", true))  return false;
		}
		default:
			return false;
	}

	if (!MQTT.publish((Serial_Hardware + "/ds/REVISAO_FW").c_str(),    REVISAO_FW,                               true))  return false;
	if (!MQTT.publish((Serial_Hardware + "/ds/ModbusSlaveID").c_str(), String(aq_eeprom::ModbusSlaveID).c_str(), true))  return false;

	String baudrate;
	switch (aq_eeprom::SerialBoudrate)
	{
		case 0:  baudrate = "300";    break;
		case 1:  baudrate = "600";    break;
		case 2:  baudrate = "1200";   break;
		case 3:  baudrate = "2400";   break;
		case 4:  baudrate = "4800";   break;
		case 5:  baudrate = "9600";   break;
		case 6:  baudrate = "14400";  break;
		case 7:  baudrate = "19200";  break;
		case 8:  baudrate = "28800";  break;
		case 9:  baudrate = "38400";  break;
		case 10: baudrate = "57600";  break;
		case 11: baudrate = "115200"; break;
		default: baudrate = "Erro";   break;
	}
	if (!MQTT.publish((Serial_Hardware + "/ds/SerialBoudrate").c_str(), baudrate.c_str(), true)) return false;

	String paridade;
	switch (aq_eeprom::SerialParidade)
	{
		case 0:  paridade = "Nenhuma"; break;
		case 1:  paridade = "Ímpar";   break;
		case 2:  paridade = "Par";     break;
		default: paridade = "Erro";    break;
	}
	if (!MQTT.publish((Serial_Hardware + "/ds/SerialParidade").c_str(), paridade.c_str(),                      true)) return false;
	if (!MQTT.publish((Serial_Hardware + "/ds/SerialStopBits").c_str(), aq_eeprom::SerialStopBits ? "2" : "1", true)) return false;

	// Sucesso.
	return true;
}

void SetupSerial()
{
	if (DebugSerial)
		Serial.begin(115200, SERIAL_8N1);
	else
	{
		unsigned long serialBoudrate;
		switch (aq_eeprom::SerialBoudrate)
		{
			case 0:
				serialBoudrate = 300;
				break;
			case 1:
				serialBoudrate = 600;
				break;
			case 2:
				serialBoudrate = 1200;
				break;
			case 3:
				serialBoudrate = 2400;
				break;
			case 4:
				serialBoudrate = 4800;
				break;
			case 5:
				serialBoudrate = 9600;
				break;
			case 6:
				serialBoudrate = 14400;
				break;
			case 7:
				serialBoudrate = 19200;
				break;
			case 8:
				serialBoudrate = 28800;
				break;
			case 9:
				serialBoudrate = 38400;
				break;
			case 10:
				serialBoudrate = 57600;
				break;
			case 11:
				serialBoudrate = 115200;
				break;
			default:
				serialBoudrate = 115200;
				break;
		}
		switch (aq_eeprom::SerialParidade)
		{
			case 0:
				Serial.begin(serialBoudrate, aq_eeprom::SerialStopBits ? SERIAL_8N2 : SERIAL_8N1);
				break;
			case 1:
				Serial.begin(serialBoudrate, aq_eeprom::SerialStopBits ? SERIAL_8O2 : SERIAL_8O1);
				break;
			case 2:
				Serial.begin(serialBoudrate, aq_eeprom::SerialStopBits ? SERIAL_8E2 : SERIAL_8E1);
				break;
			default:
				Serial.begin(serialBoudrate, SERIAL_8N1);
				break;
		}
	}
}

void setup()
{
	// Configura o LED em modo de saída e aciona o LED.
	pinMode(AQ_LED_BUILTIN, OUTPUT);
	digitalWrite(AQ_LED_BUILTIN, LOW);

	String ssid, password;
	aq_eeprom::EEPROMLOAD(ssid, password, usuarioMQTT, senhaMQTT);

	// Inicializa a interface serial.
	SetupSerial();

	// Verifica se é para entrar em modo de configuração.
	pinMode(MODO_CONFIG, INPUT);
	delay(100);
	if (digitalRead(MODO_CONFIG) == 0)
	{
		if (DebugSerial)
			Serial.println("Entrando em modo de configuracao");
		piscarLED(8, 200, 200);
		AQ_http = new aq_http();
		AQ_http->ModoConfig(DebugSerial, ssid, password, usuarioMQTT, senhaMQTT, Serial_Hardware, REVISAO_FW); // Nunca sai daqui, é bloqueante.
	}

	// Inicializa o modbus.
	AQ_modbus.setupModbus(aq_eeprom::ModeloCLP, aq_eeprom::ModbusSlaveID, MAX485_RE_NEG, MAX485_DE);

	// Inicializa o WIFI.
	AQ_wifi = new aq_wifi(DebugSerial, ssid, password);

	// MQTT Broker setup.
	MQTT.setServer(MQTTBroker, MQTTPort);

	// Inicia o super-loop.
	if (DebugSerial)
		Serial.println("Iniciando rotina de monitoramento");
}

void loop()
{
	// Início de loop.
	unsigned long startTime = millis();
	digitalWrite(AQ_LED_BUILTIN, LOW);

	// Mantém o Wifi conectado.
	if (!AQ_wifi->verificarWIFI())
	{
		piscarLED(10, 100, 900);
		return;
	}

	// Mantém o MQTT conectado.
	if (MQTT.connected())
		MQTT.loop();
	else if (MQTTConnect())
	{
		// Acabou de conectar no MQTT. Publica as facilidades no broker.
		if (!MQTTPublishDS())
		{
			piscarLED(10, 100, 900);
			return;
		}
	}
	else
	{
		piscarLED(10, 100, 900);
		return;
	}

	// Faz a leitura Modbus.
	if (!AQ_modbus.readModbus(DebugSerial))
	{
		piscarLED(10, 900, 100);
		return;
	}

	// Publica os dados no broker MQTT.
	if (!MQTTPublish())
	{
		piscarLED(10, 100, 900);
		return;
	}

	// Fim de loop. Plota o tempo total de execução.
	if (DebugSerial)
	{
		Serial.print("Tempo total de execucao (ms): ");
		Serial.println(calcProcessTime(startTime));
	}

	// Encerra o laço, calculando o tempo restante até o início do próximo laço.
	unsigned long processTime = calcProcessTime(startTime);
	digitalWrite(AQ_LED_BUILTIN, HIGH);
	if (processTime < IntervaloLoopms)
		DelayKeppingMQTT(IntervaloLoopms - processTime);
	else
		yield();
}
