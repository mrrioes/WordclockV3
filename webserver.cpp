// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  This module encapsulates a small webserver. It replies to requests on port 80
//  and triggers actions, manipulates configuration attributes or serves files
//  from the internal flash file system.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "ledfunctions.h"
#include "brightness.h"
#include "webserver.h"
#include "ntp.h"

//---------------------------------------------------------------------------------------
// global instance
//---------------------------------------------------------------------------------------
WebServerClass WebServer = WebServerClass();

//---------------------------------------------------------------------------------------
// WebServerClass
//
// Constructor, currently empty
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
WebServerClass::WebServerClass()
{
}

//---------------------------------------------------------------------------------------
// ~WebServerClass
//
// Destructor, removes allocated web server object
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
WebServerClass::~WebServerClass()
{
	if (this->server)
		delete this->server;
}

//---------------------------------------------------------------------------------------
// begin
//
// Sets up internal handlers and starts the server at port 80
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::begin()
{
	SPIFFS.begin();

	this->server = new ESP8266WebServer(80);

	this->server->on("/setcolor", std::bind(&WebServerClass::handleSetColor, this));
	this->server->on("/info", std::bind(&WebServerClass::handleInfo, this));
	this->server->on("/saveconfig", std::bind(&WebServerClass::handleSaveConfig, this));
	this->server->on("/loadconfig", std::bind(&WebServerClass::handleLoadConfig, this));
	this->server->on("/setheartbeat", std::bind(&WebServerClass::handleSetHeartbeat, this));
	this->server->on("/getheartbeat", std::bind(&WebServerClass::handleGetHeartbeat, this));
	this->server->on("/getcolors", std::bind(&WebServerClass::handleGetColors, this));
	this->server->on("/getntpserver", std::bind(&WebServerClass::handleGetNtpServer, this));
	this->server->on("/setntpserver", std::bind(&WebServerClass::handleSetNtpServer, this));
	this->server->on("/h", std::bind(&WebServerClass::handleH, this));
	this->server->on("/m", std::bind(&WebServerClass::handleM, this));
	this->server->on("/r", std::bind(&WebServerClass::handleR, this));
	this->server->on("/g", std::bind(&WebServerClass::handleG, this));
	this->server->on("/b", std::bind(&WebServerClass::handleB, this));
	this->server->on("/brightness", std::bind(&WebServerClass::handleSetBrightness, this));
	this->server->on("/getadc", std::bind(&WebServerClass::handleGetADC, this));
	this->server->on("/setmode", std::bind(&WebServerClass::handleSetMode, this));
	this->server->on("/getmode", std::bind(&WebServerClass::handleGetMode, this));
	this->server->on("/settimezone", std::bind(&WebServerClass::handleSetTimeZone, this));
	this->server->on("/gettimezone", std::bind(&WebServerClass::handleGetTimeZone, this));
	this->server->on("/debug", std::bind(&WebServerClass::handleDebug, this));

	this->server->onNotFound(std::bind(&WebServerClass::handleNotFound, this));
	this->server->begin();
}

//---------------------------------------------------------------------------------------
// process
//
// Must be called repeatedly from main loop
//
// ->
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::process()
{
	this->server->handleClient();
}

//---------------------------------------------------------------------------------------
// serveFile
//
// Looks up a given file name in internal flash file system, streams the file if found
//
// -> path: name of the file; "index.html" will be added if name ends with "/"
// <- true: file was found and served to client
//	false: file not found
//---------------------------------------------------------------------------------------
bool WebServerClass::serveFile(String path)
{
	Serial.println("WebServerClass::serveFile(): " + path);
	if (path.endsWith("/"))
		path += "index.html";
	if (SPIFFS.exists(path))
	{
		File file = SPIFFS.open(path, "r");
		this->server->streamFile(file, this->contentType(path));
		file.close();
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------------------
// contentType
//
// Returns an HTML content type based on a given file name extension
//
// -> filename: name of the file
// <- HTML content type matching file extension
//---------------------------------------------------------------------------------------
String WebServerClass::contentType(String filename)
{
	if (this->server->hasArg("download")) return "application/octet-stream";
	else if (filename.endsWith(".htm")) return "text/html";
	else if (filename.endsWith(".html")) return "text/html";
	else if (filename.endsWith(".css")) return "text/css";
	else if (filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".png")) return "image/png";
	else if (filename.endsWith(".gif")) return "image/gif";
	else if (filename.endsWith(".jpg")) return "image/jpeg";
	else if (filename.endsWith(".ico")) return "image/x-icon";
	else if (filename.endsWith(".xml")) return "text/xml";
	else if (filename.endsWith(".pdf")) return "application/x-pdf";
	else if (filename.endsWith(".zip")) return "application/x-zip";
	else if (filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}

//---------------------------------------------------------------------------------------
// handleM
//
// Handles the /m request, increments the minutes counter (for testing purposes)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
extern int h, m;
void WebServerClass::handleM()
{
	if(++m>59) m = 0;
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleH
//
// Handles the /h request, increments the hours counter (for testing purposes)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleH()
{
	if(++h>23) h = 0;
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleR
//
// Handles the /r request, sets LED matrix to all red (for testing purposes)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleR()
{
	LED.setMode(DisplayMode::red);
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleG
//
// Handles the /g request, sets LED matrix to all green (for testing purposes)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleG()
{
	LED.setMode(DisplayMode::green);
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleB
//
// Handles the /b request, sets LED matrix to all blue (for testing purposes)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleB()
{
	LED.setMode(DisplayMode::blue);
	this->server->send(200, "text/plain", "OK");
}

void WebServerClass::handleSetBrightness()
{
	if(this->server->hasArg("value"))
	{
		Brightness.brightnessOverride = this->server->arg("value").toInt();
		this->server->send(200, "text/plain", "OK");
	}
}

void WebServerClass::handleDebug()
{
	if(this->server->hasArg("led") &&
			   this->server->hasArg("r") &&
			   this->server->hasArg("g") &&
			   this->server->hasArg("b"))
	{
		int led = this->server->arg("led").toInt();
		int r = this->server->arg("r").toInt();
		int g = this->server->arg("g").toInt();
		int b = this->server->arg("b").toInt();
		if(led < 0) led = 0;
		if(led >= NUM_PIXELS) led = NUM_PIXELS - 1;
		if(r < 0) r = 0;
		if(r > 255) r = 255;
		if(g < 0) g = 0;
		if(g > 255) g = 255;
		if(b < 0) b = 0;
		if(b > 255) b = 255;

		LED.currentValues[led*3+0] = r;
		LED.currentValues[led*3+1] = g;
		LED.currentValues[led*3+2] = b;
		LED.show();
		Config.debugMode = 1;
	}

	if(this->server->hasArg("clear"))
	{
		for(int i=0; i<3*NUM_PIXELS; i++) LED.currentValues[i] = 0;
		LED.show();
	}

	if(this->server->hasArg("end"))
	{
		Config.debugMode = 0;
	}
	this->server->send(200, "text/plain", "OK");
}

void WebServerClass::handleGetADC()
{
	int __attribute__ ((unused)) temp = Brightness.value(); // to trigger A/D conversion
	this->server->send(200, "text/plain", String(Brightness.avg));
}

//---------------------------------------------------------------------------------------
// handleSetTimeZone
//
//
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetTimeZone()
{
	if(this->server->hasArg("value"))
	{
		int newTimeZone = this->server->arg("value").toInt();
		if(newTimeZone < - 12) newTimeZone = -12;
		if(newTimeZone > 14) newTimeZone = 14;
		Config.timeZone = newTimeZone;
		Config.save();
		NTP.setTimeZone(Config.timeZone);
	}
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleGetTimeZone
//
//
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetTimeZone()
{
	this->server->send(200, "text/plain", String(Config.timeZone));
}

//---------------------------------------------------------------------------------------
// handleSetMode
//
// Handles the /setmode request. Sets the display mode to one of the allowed values,
// saves it as the new default mode.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetMode()
{
	DisplayMode mode = DisplayMode::invalid;

	if(this->server->hasArg("value"))
	{
		// handle each allowed value for safety
		if(this->server->arg("value") == "0") mode = DisplayMode::plain;
		if(this->server->arg("value") == "1") mode = DisplayMode::fade;
		if(this->server->arg("value") == "2") mode = DisplayMode::flyingLettersVerticalUp;
		if(this->server->arg("value") == "3") mode = DisplayMode::flyingLettersVerticalDown;
		if(this->server->arg("value") == "4") mode = DisplayMode::explode;
	}

	if(mode == DisplayMode::invalid)
	{
		this->server->send(400, "text/plain", "ERR");
	}
	else
	{
		LED.setMode(mode);
		Config.defaultMode = mode;
		Config.save();
		this->server->send(200, "text/plain", "OK");
	}
}

//---------------------------------------------------------------------------------------
// handleGetMode
//
// Handles the /getmode request and returns the current default display mode.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetMode()
{
	int mode = 0;
	switch(Config.defaultMode)
	{
	case DisplayMode::plain:
		mode = 0; break;
	case DisplayMode::fade:
		mode = 1; break;
	case DisplayMode::flyingLettersVerticalUp:
		mode = 2; break;
	case DisplayMode::flyingLettersVerticalDown:
		mode = 3; break;
	default:
		mode = 0; break;
	}
	this->server->send(200, "text/plain", String(mode));
}

//---------------------------------------------------------------------------------------
// handleNotFound
//
// Handles all requests not bound to other handlers, tries to serve a file if found in
// flash, responds with 404 otherwise
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleNotFound()
{
	// first, try to serve the requested file from flash
	if (!serveFile(this->server->uri()))
	{
		// create 404 message if no file was found for this URI
		String message = "File Not Found\n\n";
		message += "URI: ";
		message += this->server->uri();
		message += "\nMethod: ";
		message += (this->server->method() == HTTP_GET) ? "GET" : "POST";
		message += "\nArguments: ";
		message += this->server->args();
		message += "\n";
		for (uint8_t i = 0; i < this->server->args(); i++)
		{
			message += " " + this->server->argName(i) + ": "
					+ this->server->arg(i) + "\n";
		}
		this->server->send(404, "text/plain", message);
	}
}

//---------------------------------------------------------------------------------------
// handleGetNtpServer
//
// Delivers the currently configured NTP server IP address
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetNtpServer()
{
	this->server->send(200, "application/json", Config.ntpserver.toString());
}

//---------------------------------------------------------------------------------------
// handleSetNtpServer
//
// Sets a new IP address for the NTP client
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetNtpServer()
{
	if (this->server->hasArg("ip"))
	{
		IPAddress ip;
		if (ip.fromString(this->server->arg("ip")))
		{
			// set IP address in config
			Config.ntpserver = ip;
			Config.save();

			// set IP address in client
			NTP.setServer(ip);
		}
	}
	this->server->send(200, "application/json", "OK");
}

//---------------------------------------------------------------------------------------
// handleInfo
//
// Handles requests to "/info", replies with JSON structure containing system status
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleInfo()
{
	StaticJsonBuffer<512> jsonBuffer;
	char buf[512];
	JsonObject& json = jsonBuffer.createObject();
	json["heap"] = ESP.getFreeHeap();
	json["sketchsize"] = ESP.getSketchSize();
	json["sketchspace"] = ESP.getFreeSketchSpace();
	json["cpufrequency"] = ESP.getCpuFreqMHz();
	json["chipid"] = ESP.getChipId();
	json["sdkversion"] = ESP.getSdkVersion();
	json["bootversion"] = ESP.getBootVersion();
	json["bootmode"] = ESP.getBootMode();
	json["flashid"] = ESP.getFlashChipId();
	json["flashspeed"] = ESP.getFlashChipSpeed();
	json["flashsize"] = ESP.getFlashChipRealSize();
	json["resetreason"] = ESP.getResetReason();
	json["resetinfo"] = ESP.getResetInfo();
//	switch(LED.getMode())
//	{
//	case DisplayMode::plain:
//		json["mode"] = "plain"; break;
//	case DisplayMode::fade:
//		json["mode"] = "fade"; break;
//	case DisplayMode::flyingLettersVertical:
//		json["mode"] = "flyingLettersVertical"; break;
//	case DisplayMode::matrix:
//		json["mode"] = "matrix"; break;
//	case DisplayMode::heart:
//		json["mode"] = "heart"; break;
//	case DisplayMode::stars:
//		json["mode"] = "stars"; break;
//	case DisplayMode::red:
//		json["mode"] = "red"; break;
//	case DisplayMode::green:
//		json["mode"] = "green"; break;
//	case DisplayMode::blue:
//		json["mode"] = "blue"; break;
//	case DisplayMode::yellowHourglass:
//		json["mode"] = "yellowHourglass"; break;
//	case DisplayMode::greenHourglass:
//		json["mode"] = "greenHourglass"; break;
//	case DisplayMode::update:
//		json["mode"] = "update"; break;
//	case DisplayMode::updateComplete:
//		json["mode"] = "updateComplete"; break;
//	case DisplayMode::updateError:
//		json["mode"] = "updateError"; break;
//	case DisplayMode::wifiManager:
//		json["mode"] = "wifiManager"; break;
//	default:
//		json["mode"] = "unknown"; break;
//	}

	json.printTo(buf, sizeof(buf));
	this->server->send(200, "application/json", buf);
}

//---------------------------------------------------------------------------------------
// extractColor
//
// Converts the given web server argument to a color struct
// -> argName: Name of the web server argument
//	result: Pointer to palette_entry struct to receive result
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::extractColor(String argName, palette_entry& result)
{
	char c[3];

	if (this->server->hasArg(argName) && this->server->arg(argName).length() == 6)
	{
		String color = this->server->arg(argName);
		color.substring(0, 2).toCharArray(c, sizeof(c));
		result.r = strtol(c, NULL, 16);
		color.substring(2, 4).toCharArray(c, sizeof(c));
		result.g = strtol(c, NULL, 16);
		color.substring(4, 6).toCharArray(c, sizeof(c));
		result.b = strtol(c, NULL, 16);
	}
}

//---------------------------------------------------------------------------------------
// handleSetColor
//
// Handles the "/setcolor" request, expects arguments:  // fg = foreground, bg = background, s = ? evtl seconds
//	/setcolor?fg=xxxxxx&bg=yyyyyy&s=zzzzzz
//	with xxxxxx, yyyyyy and zzzzzz being hexadecimal HTML colors (without leading '#')
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetColor()
{
	this->extractColor("fg", Config.fg);
	this->extractColor("bg", Config.bg);
	this->extractColor("s", Config.s);
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleSaveConfig
//
// Saves the current configuration to EEPROM
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSaveConfig()
{
	Config.save();
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleLoadConfig
//
// Loads the current configuration from EEPROM
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleLoadConfig()
{
	Config.load();
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleSetHeartbeat
//
// Sets or resets the heartbeat flag in the configuration based on argument "state"
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetHeartbeat()
{
	Config.heartbeat = (this->server->hasArg("value") && this->server->arg("value") == "1");
	Config.save();
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleGetHeartbeat
//
// Returns the state of the heartbeat flag.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetHeartbeat()
{
	if(Config.heartbeat) this->server->send(200, "text/plain", "1");
	else this->server->send(200, "text/plain", "0");
}

//---------------------------------------------------------------------------------------
// handleGetColors
//
// Outputs the currently active colors as comma separated list for background, foreground
// and seconds color with 3 values each (red, green, blue)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetColors()
{
	String message = String(Config.bg.r) + "," + String(Config.bg.g) + ","
			+ String(Config.bg.b) + "," + String(Config.fg.r) + ","
			+ String(Config.fg.g) + "," + String(Config.fg.b) + ","
			+ String(Config.s.r) + "," + String(Config.s.g) + ","
			+ String(Config.s.b);
	this->server->send(200, "text/plain", message);
}
