#include "aq_http.h"

// Definições dos atributos estáticos.
ESP8266WebServer aq_http::server;
String           aq_http::ssid;
String           aq_http::password;
String           aq_http::usuarioMQTT;
String           aq_http::senhaMQTT;
String           aq_http::serialNumber;
bool             aq_http::debugSerial;
String           aq_http::firmware;

void aq_http::ModoConfig(bool _debugSerial, String _ssid, String _password, String _usuarioMQTT, String _senhaMQTT, String _serialNumber, String _firmware)
{
	// Salva os parâmetros.
	debugSerial  = _debugSerial;
	ssid         = _ssid;
	password     = _password;
	usuarioMQTT  = _usuarioMQTT;
	senhaMQTT    = _senhaMQTT;
	serialNumber = _serialNumber;
	firmware     = _firmware;

	WiFi.mode(WIFI_AP);
	IPAddress apIP(192, 168, 10, 1);
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP("ESP8266");
	if (debugSerial)
	{
		Serial.print("Endereco IP do AP: ");
		Serial.println(WiFi.softAPIP());
	}
	dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
	dnsServer.start(53, "*", apIP);

	server.on("/", aq_http::handleRoot);
	server.onNotFound(aq_http::handleRoot);
	server.on("/save", aq_http::handleSave);
	server.on("/wifi", aq_http::handleWiFi);
	server.begin(); // Web server start

	while (1)
	{
		dnsServer.processNextRequest();
		server.handleClient();
	}
}

void aq_http::handleWiFi()
{
	server.sendHeader("Cache-Control", F("no-cache, no-store, must-revalidate"));
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
	server.sendContent(F(
		"<html><head><style>@media only screen and (min-width: 650px) { .newspaper { float: left; } }</style></head>"
		"<script> function fillSSID(ssid) { "
		"document.getElementById('n').value=ssid; "
		"document.getElementById('p').value=''; "
		"document.getElementById('p').focus(); } "
		"function validateForm() { "
		//"if (false || false) { return false; } "
		"return true; } </script>"
	));
	server.sendContent(F(
		"<body><table border='1' align='center' bgcolor='#B0FFFF'><tr><td><p align='center'><font size='5'><b>Configura&ccedil;&otilde;es do Sistema</b></font></p><div class='newspaper'>"
		"<table><tr><td colspan='3'><p align='center'><b>Lista de redes localizadas</b></p></td></tr>"
		"<tr><td><b>Nome (SSID)&nbsp;</b></td><td><b>Senha (S/N)&nbsp;</b></td><td><b>Pot&ecirc;ncia (dBm)</b></td></tr>"
	));
	if (debugSerial)
		Serial.println("scan start");

	int n = WiFi.scanNetworks();
	if (debugSerial)
		Serial.println("scan done");
	if (n > 0) {
		for (int i = 0; i < n; i++) {
			server.sendContent(String() + "<tr><td>" + "<a href='#' onclick='fillSSID(\"" + WiFi.SSID(i) + "\")'>" + WiFi.SSID(i) +
				"</a></td><td>" + String((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "N&atilde;o" : "Sim") + "</td><td>" + WiFi.RSSI(i) + "</td></tr>");
		}
	}
	else {
		server.sendContent(String() + F("<tr><td colspan='3'>Nenhuma rede encontrada</td></tr>"));
	}
	server.sendContent(
		String(F("</table><p style='margin-top: 0; margin-bottom: 0'>&nbsp;</p>"
		"<form method='POST' action='save' onsubmit='return validateForm();'>"
		"<table><tr><td><p>Wireless:</p></td><td><input type='text' placeholder='ssid' name='n' id='n' value='")) + ssid + String(F("' size='25' pattern='.{1,}' required></td></tr>"
		"<tr><td><p>Senha:</p></td><td><input type='text' placeholder='senha' name='p' id='p' value='")) + password + String(F("' size='25'></td></tr>"
		"<tr><td><p>Modelo do CLP:</p></td><td><select name='clp' id='clp' style='width: 140px'>"
	)));
	server.sendContent((aq_eeprom::ModeloCLP == 0) ? F("<option value='PCC1301/PCC1302' selected>PCC1301/PCC1302</option>") : F("<option value='PCC1301/PCC1302'>PCC1301/PCC1302</option>"));
	//server.sendContent((aq_eeprom::ModeloCLP == 1)  ? "<option value='PCC3300' selected>PCC3300</option>" : "<option value='PCC3300'>PCC3300</option>");
	server.sendContent("</select></td></tr>");

	server.sendContent(String(F(
		"<tr><td><p>ID Modbus:</p></td><td><p><input type='number' placeholder='id escravo' name='slave' id='slave' value='")) + String(aq_eeprom::ModbusSlaveID) + String(F("' "
		"size='10' min='1' max='255' required> (1-255)</p></td></tr>"
		"<tr><td><p>Boudrate:</p></td><td><select name='boudrate' id='boudrate' style='width: 140px'>"
	)));
	server.sendContent((aq_eeprom::SerialBoudrate == 0)  ? F("<option value='300' selected>300</option>")       : F("<option value='300'>300</option>"));
	server.sendContent((aq_eeprom::SerialBoudrate == 1)  ? F("<option value='600' selected>600</option>")       : F("<option value='600'>600</option>"));
	server.sendContent((aq_eeprom::SerialBoudrate == 2)  ? F("<option value='1200' selected>1200</option>")     : F("<option value='1200'>1200</option>"));
	server.sendContent((aq_eeprom::SerialBoudrate == 3)  ? F("<option value='2400' selected>2400</option>")     : F("<option value='2400'>2400</option>"));
	server.sendContent((aq_eeprom::SerialBoudrate == 4)  ? F("<option value='4800' selected>4800</option>")     : F("<option value='4800'>4800</option>"));
	server.sendContent((aq_eeprom::SerialBoudrate == 5)  ? F("<option value='9600' selected>9600</option>")     : F("<option value='9600'>9600</option>"));
	server.sendContent((aq_eeprom::SerialBoudrate == 6)  ? F("<option value='14400' selected>14400</option>")   : F("<option value='14400'>14400</option>"));
	server.sendContent((aq_eeprom::SerialBoudrate == 7)  ? F("<option value='19200' selected>19200</option>")   : F("<option value='19200'>19200</option>"));
	server.sendContent((aq_eeprom::SerialBoudrate == 8)  ? F("<option value='28800' selected>28800</option>")   : F("<option value='28800'>28800</option>"));
	server.sendContent((aq_eeprom::SerialBoudrate == 9)  ? F("<option value='38400' selected>38400</option>")   : F("<option value='38400'>38400</option>"));
	server.sendContent((aq_eeprom::SerialBoudrate == 10) ? F("<option value='57600' selected>57600</option>")   : F("<option value='57600'>57600</option>"));
	server.sendContent((aq_eeprom::SerialBoudrate == 11) ? F("<option value='115200' selected>115200</option>") : F("<option value='115200'>115200</option>"));

	server.sendContent(F("</select></td></tr><tr><td><p>Paridade:</p></td><td><select name='paridade' id='paridade' style='width: 140px'>"));
	server.sendContent((aq_eeprom::SerialParidade == 0) ? F("<option value='nenhuma' selected>Nenhuma - None</option>")   : F("<option value='nenhuma'>Nenhuma - None</option>"));
	server.sendContent((aq_eeprom::SerialParidade == 1) ? F("<option value='impar' selected>&Iacute;mpar - Odd</option>") : F("<option value='impar'>&Iacute;mpar - Odd</option>"));
	server.sendContent((aq_eeprom::SerialParidade == 2) ? F("<option value='par' selected>Par - Even</option>")           : F("<option value='par'>Par - Even</option>"));

	server.sendContent(F("</select></td></tr><tr><td><p>Stop bits:</p></td><td><select name='stopbits' id='stopbits' style='width: 140px'>"));
	server.sendContent((aq_eeprom::SerialStopBits) ? F("<option value='1'>1</option><option value='2' selected>2</option>") :
		F("<option value='1' selected>1</option><option value='2'>2</option>"));

	server.sendContent(String(F(
		"</select></td></tr>"
		"<tr><td><p>Usuario MQTT:</p></td><td><p><input type='text' placeholder='usuario MQTT' name='user_mqtt' id='user_mqtt' value='")) + usuarioMQTT + String(F("' size='30' required></p></td></tr>"
		"<tr><td><p>Senha MQTT:</p></td><td><p><input type='text' placeholder='senha MQTT' name='pass_mqtt' id='pass_mqtt' value='")) + senhaMQTT + String(F("' size='30' required></p></td></tr>"
		"<tr><td><p>Serial HW:</p></td><td><p>")) + serialNumber + String(F("</p></td></tr>"
		"<tr><td><p>Firmware:</p></td><td><p>")) + firmware + String(F("</p></td></tr>"
		"</table></div><table>"
	)));
	server.sendContent(F(
		"<p align='right' style='margin-top: 10;'><input type='submit' value='Salvar'>&nbsp;&nbsp; "
		"<input type='reset' value='Resetar' onclick=\"return window.confirm('Realmente deseja resetar o formul&aacute;rio para os valores anteriores?');\">&nbsp;</p>"
		"<p align='center'></p></form></td></tr></table></body></html>"
	));
	server.client().stop(); // Stop is needed because we sent no content length
}

void aq_http::handleSave()
{
	if (debugSerial)
		Serial.println(F("Salvando configurações"));

	     if (server.arg("boudrate") == String("300"))     aq_eeprom::SerialBoudrate = 0;
	else if (server.arg("boudrate") == String("600"))     aq_eeprom::SerialBoudrate = 1;
	else if (server.arg("boudrate") == String("1200"))    aq_eeprom::SerialBoudrate = 2;
	else if (server.arg("boudrate") == String("2400"))    aq_eeprom::SerialBoudrate = 3;
	else if (server.arg("boudrate") == String("4800"))    aq_eeprom::SerialBoudrate = 4;
	else if (server.arg("boudrate") == String("9600"))    aq_eeprom::SerialBoudrate = 5;
	else if (server.arg("boudrate") == String("14400"))   aq_eeprom::SerialBoudrate = 6;
	else if (server.arg("boudrate") == String("19200"))   aq_eeprom::SerialBoudrate = 7;
	else if (server.arg("boudrate") == String("28800"))   aq_eeprom::SerialBoudrate = 8;
	else if (server.arg("boudrate") == String("38400"))   aq_eeprom::SerialBoudrate = 9;
	else if (server.arg("boudrate") == String("57600"))   aq_eeprom::SerialBoudrate = 10;
	else if (server.arg("boudrate") == String("115200"))  aq_eeprom::SerialBoudrate = 11;
	else
	{
		imprimirAviso(F("Erro interno: erro durante a configura&ccedil;&atilde;o do boudrate da porta serial"));
		return;
	}

	     if (server.arg("paridade") == String("nenhuma")) aq_eeprom::SerialParidade = 0;
	else if (server.arg("paridade") == String("impar"))   aq_eeprom::SerialParidade = 1;
	else if (server.arg("paridade") == String("par"))     aq_eeprom::SerialParidade = 2;
	else
	{
		imprimirAviso(F("Erro interno: erro durante a configura&ccedil;&atilde;o da paridade da porta serial"));
		return;
	}

	if (server.arg("stopbits") == String("1"))       aq_eeprom::SerialStopBits = false;
	else                                             aq_eeprom::SerialStopBits = true;

	if (server.arg("clp") == String("PCC1301/PCC1302"))    aq_eeprom::ModeloCLP = 0;
	//else if (server.arg("clp") == String("PCC3300"))     aq_eeprom::ModeloCLP = 1;
	else
	{
		imprimirAviso(F("Erro interno: erro durante a configura&ccedil;&atilde;o do modelo de CLP"));
		return;
	}

	if (server.arg("slave").toInt() > 0 && server.arg("slave").toInt() <= 255)
		aq_eeprom::ModbusSlaveID = server.arg("slave").toInt();
	else
	{
		imprimirAviso(F("Erro interno: o ID do dispositivo Modbus escravo deve ser maior que zero"));
		return;
	}

	if (server.arg("n").length() > 0)
		ssid = server.arg("n");
	else
	{
		imprimirAviso(F("Erro interno: o nome da rede Wireless deve conter ao menos 1 caractere"));
		return;
	}

	if (server.arg("user_mqtt").length() > 0)
		usuarioMQTT = server.arg("user_mqtt");
	else
	{
		imprimirAviso(F("Erro interno: o nome do usuario MQTT deve conter ao menos 1 caractere"));
		return;
	}

	if (server.arg("pass_mqtt").length() > 0)
		senhaMQTT = server.arg("pass_mqtt");
	else
	{
		imprimirAviso(F("Erro interno: a senha MQTT deve conter ao menos 1 caractere"));
		return;
	}

	password = server.arg("p");
	aq_eeprom::EEPROMSAVE(ssid, password, usuarioMQTT, senhaMQTT); // Salva as configurações na EEPROM.
	imprimirAviso("Configura&ccedil;&otilde;es salvas com sucesso");
	if (debugSerial)
		Serial.println("Configurações salvas com sucesso");
}

void aq_http::imprimirAviso(String aviso)
{
	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
	server.sendContent(
		"<html><head></head><body>"
		"<p>" + aviso + "</p>"
		"</body></html>"
	);
	server.client().stop(); // Stop is needed because we sent no content length
}

void aq_http::handleRoot()
{
	server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server.sendHeader("Pragma", "no-cache");
	server.sendHeader("Expires", "-1");
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
	server.sendContent(F(
		"<html><head></head><body>"
		"<p style='margin-bottom: 0'>Para configurar o dispositivo <a href='/wifi'>clique aqui</a>.</p>"
		"</body></html>"
	));
	server.client().stop(); // Stop is needed because we sent no content length
}
