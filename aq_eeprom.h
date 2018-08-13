#ifndef AQ_EEPROM
#define AQ_EEPROM

#include <EEPROM.h>
#include "WString.h"

class aq_eeprom
{
public:
	static unsigned char ModeloCLP;
	static unsigned char ModbusSlaveID;
	static unsigned char SerialBoudrate;
	static unsigned char SerialParidade;
	static bool          SerialStopBits;

	static void EEPROMLOAD(String& ssid, String& password, String& usuarioMQTT, String& senhaMQTT);
	static void EEPROMSAVE(String ssid, String password, String usuarioMQTT, String senhaMQTT);
private:
	static void WriteString(String text, int& index);
	static void ReadString(String& text, int& index);
};
#endif