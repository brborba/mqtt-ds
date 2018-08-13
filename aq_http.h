#ifndef AQ_HTTP
#define AQ_HTTP

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "aq_eeprom.h"

class aq_http
{
private:
	DNSServer               dnsServer; // Servidor DNS.
	static ESP8266WebServer server;    // Servidor WEB para configuração
	static String           ssid;
	static String           password;
	static String           usuarioMQTT;
	static String           senhaMQTT;
	static String           serialNumber;
	static bool             debugSerial;
	static String           firmware;

public:
	void ModoConfig(bool _debugSerial, String _ssid, String _password, String _usuarioMQTT, String _senhaMQTT, String _serialNumber, String _firmware);

private:
	static void handleRoot();
	static void handleSave();
	static void handleWiFi();
	static void imprimirAviso(String aviso);
};
#endif
