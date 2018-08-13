#include "aq_wifi.h"

aq_wifi::aq_wifi(bool _debugSerial, String _ssid, String _password)
{
	debugSerial = _debugSerial;
	ssid        = _ssid;
	password    = _password;

	// Estado inicial desconectado.
	WiFi.disconnect();
}

bool aq_wifi::verificarWIFI()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		WiFi.disconnect();
		if (debugSerial)
		{
			Serial.println("");
			Serial.println("Conectando");
			Serial.print("SSID: ");
			Serial.println(ssid);
			Serial.print("Senha: ");
			Serial.println(password);
		}
		WiFi.begin(ssid.c_str(), password.c_str());
		for (int i = 0; i < 20; i++)
		{
			if (WiFi.status() != WL_CONNECTED)
			{
				if (debugSerial)
					Serial.print(".");
				delay(500);
			}
			else
			{
				if (debugSerial)
				{
					Serial.println("");
					Serial.println("WiFi conectado");
					Serial.println("Endereco IP: ");
					Serial.println(WiFi.localIP());
					WiFi.printDiag(Serial);
				}
				return true;
			}
		}
		if (debugSerial)
		{
			Serial.println("");
			Serial.println("Erro ao tentar conectar.");
		}
		return false;
	}
	if (debugSerial)
	{
		Serial.print("RSSI (dbm): ");
		Serial.println(WiFi.RSSI());
	}
	return true;
}
