#ifndef AQ_WIFI
#define AQ_WIFI

#include <ESP8266WiFi.h>

class aq_wifi
{
private:
	String  ssid;       // Usuário para acesso ao Wi-Fi.
	String  password;   // Senha para acesso ao Wi-Fi.
	bool    debugSerial;

public:
	aq_wifi(bool _debugSerial, String _ssid, String _password);
	bool verificarWIFI();
};
#endif