#include "aq_eeprom.h"

unsigned char aq_eeprom::ModeloCLP;
unsigned char aq_eeprom::ModbusSlaveID;
unsigned char aq_eeprom::SerialBoudrate;
unsigned char aq_eeprom::SerialParidade;
bool          aq_eeprom::SerialStopBits;

void aq_eeprom::EEPROMLOAD(String& ssid, String& password, String& usuarioMQTT, String& senhaMQTT)
{
  int index = 0;
  EEPROM.begin(4096);
  ReadString(ssid,        index);
  ReadString(password,    index);
  ReadString(usuarioMQTT, index);
  ReadString(senhaMQTT,   index);

  ModbusSlaveID  = EEPROM.read(index++);
  ModeloCLP      = EEPROM.read(index++);
  SerialBoudrate = EEPROM.read(index++);
  SerialParidade = EEPROM.read(index++);
  SerialStopBits = EEPROM.read(index++) ? true : false;

  //
  // Final.
  //
  char ok[2+1];
  EEPROM.get(index, ok); index += sizeof(ok);
  EEPROM.end();
  if (String(ok) != String("OK") || SerialBoudrate > 11 || SerialParidade > 2 || ModbusSlaveID == 0)
  {
	  // Esses valores devem ser válidos para a execução do programa não gerar exceções.
	  ssid           = String("");
	  password       = String("");
	  usuarioMQTT    = String("");
	  senhaMQTT      = String("");
	  ModbusSlaveID  = 0;
	  ModeloCLP      = 1;
	  SerialBoudrate = 11;    // 115200.
	  SerialParidade = 0;     // Nenhuma.
	  SerialStopBits = false; // 1.
  }
}

void aq_eeprom::EEPROMSAVE(String ssid, String password, String usuarioMQTT, String senhaMQTT)
{
  int index = 0;
  EEPROM.begin(4096);
  WriteString(ssid,        index);
  WriteString(password,    index);
  WriteString(usuarioMQTT, index);
  WriteString(senhaMQTT,   index);

  EEPROM.write(index++, ModbusSlaveID);
  EEPROM.write(index++, ModeloCLP);
  EEPROM.write(index++, SerialBoudrate);
  EEPROM.write(index++, SerialParidade);
  EEPROM.write(index++, SerialStopBits ? 1 : 0);

  // Final.
  char ok[2+1] = "OK";
  EEPROM.put(index, ok); index += sizeof(ok);
  EEPROM.commit();
  EEPROM.end();
}

void aq_eeprom::WriteString(String text, int& index)
{
	unsigned char textSize = (text.length() + 1 > 255) ? 255 : (text.length() + 1);
	char          *_text   = new char[textSize];
	text.toCharArray(_text, textSize);
	EEPROM.put(index++, textSize);
	for (int i = 0; i < textSize; i++)
		EEPROM.write(index++, _text[i]);
	delete[] _text;
}

void aq_eeprom::ReadString(String& text, int& index)
{
	char          *_text;
	unsigned char tmp_uc;
	EEPROM.get(index++, tmp_uc);
	if (tmp_uc == 0)
	{
		text = String("");
		return;
	}
	_text = new char[tmp_uc];
	for (int i = 0; i < tmp_uc; i++)
		_text[i] = EEPROM.read(index++);
	text = String(_text);
	delete[] _text;
}
