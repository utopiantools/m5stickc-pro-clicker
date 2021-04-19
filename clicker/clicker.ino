#include <Arduino.h>
#include <M5StickC.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>

using namespace websockets;

const char *SSID = "2.4_SSID_GOES_HERE";         // must be 2.4Ghz
const char *PASSWORD = "PASSPHRASE_GOES_HERE";
char pro_ip[40] = "192.168.10.50";               // IP address of your propresenter computer
char pro_port[6] = "60157";                      // propresenter network
char pro_pass[32] = "control";                   // propresenter remote control password
char pro_vers[2] = "6";                          // propresenter 6 or 7 ??

// battery constants
const float BATMAX = 4.2;
const float BATMIN = 3.27;
const float BATRANGE = BATMAX - BATMIN;

// quick color constants
// green and red leds are swapped in the M5Atom LITE for some reason
const uint32_t red = 0x00f000;
const uint32_t green = 0xf00000;
const uint32_t blue = 0x0000f0;
const uint32_t yellow = 0xf0f000;
const uint32_t black = 0x000000;
uint32_t bgcolor = yellow;
uint32_t fgcolor = black;

// PRO6 commands
const String nextJSON = "{\"action\": \"presentationTriggerNext\" }";
const String prevJSON = "{\"action\": \"presentationTriggerPrevious\" }";

// State variables
String _status = "";
String _lastmsg = "";
bool wifi_connected = false;
bool pro_connected = false;
String localip = "";
int lastbutton_time = 4000;
bool should_handle_button = true;

// NetWork Classes
WebsocketsClient client;

// UI FUNCTIONS
double batPct()
{
  float volts = M5.Axp.GetBatVoltage();
  return (volts - BATMIN) / BATRANGE;
}

void cls()
{
  M5.Lcd.fillScreen(bgcolor);
  M5.Lcd.setTextColor(fgcolor, bgcolor);
  M5.Lcd.setCursor(0, 0);
}
void cls(uint32_t bg, uint32_t fg = BLACK)
{
  bgcolor = bg;
  fgcolor = fg;
  cls();
}

void status()
{
  cls();
  Serial.println("status ---------------");
  Serial.println(pro_ip);
  Serial.println(pro_port);
  Serial.println(pro_vers);
  Serial.println(pro_pass);
  String pro_connection = String(pro_ip) + ":" + String(pro_port);
  String fullstatus = (wifi_connected && pro_connected) ? "PRO " + pro_connection : "NOT CONNECTED";
  String pct = String((int)(batPct() * 100));
  fullstatus += "\nBATTERY : " + pct + "%";
  fullstatus += "\n=======================\n" + _status + "\n" + _lastmsg;
  Serial.println(fullstatus);
  M5.Lcd.print(fullstatus);
}

void status(String s, bool append = false)
{
  if (append)
  {
    _status += s;
    Serial.println(s);
    M5.Lcd.print(s);
  }
  else
  {
    _status = s;
    status();
  }
}

void statusln(String s, bool append = false)
{
  if (append)
    _status += s;
  else
    _status = s;
  _status += '\n';
  status();
}

// CONFIGURATION FUNCTIONS
//callback notifying us of the need to save config
void saveConfigCallback()
{
  Serial.println("Should save config");
}

// WEBSOCKET FUNCTIONS
void onMessageCallback(WebsocketsMessage message)
{
  pro_connected = true;
  _lastmsg = message.data();
  cls(black, YELLOW);
  // status(_lastmsg, true);
  status();
}

void onEventsCallback(WebsocketsEvent event, String data)
{
  if (event == WebsocketsEvent::ConnectionOpened)
  {
    Serial.println("Connnection Opened");
    cls(green);
  }
  else if (event == WebsocketsEvent::ConnectionClosed)
  {
    Serial.println("Connnection Closed");
    _lastmsg = "";
    pro_connected = false;
  }
  else if (event == WebsocketsEvent::GotPing)
  {
    Serial.println("Got a Ping!");
  }
  else if (event == WebsocketsEvent::GotPong)
  {
    Serial.println("Got a Pong!");
  }
}

void pro_connect()
{
  status("connecting to pro presenter:\n" + String(pro_ip) + ":" + String(pro_port));

  // run callback when messages are received
  client.onMessage(onMessageCallback);

  // run callback when events are occuring
  client.onEvent(onEventsCallback);

  // Connect to server
  client.connect(pro_ip, atoi(pro_port), "/remote");

  // Send a message
  String authJSON = "{\"password\": \"" + String(pro_pass) + "\", \"protocol\":" + String(pro_vers) + "00, \"action\":\"authenticate\" }";

  Serial.println(authJSON);
  client.send(authJSON);

  // Send a ping
  // client.ping();
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  cls(green, YELLOW);
  wifi_connected = true;
  localip = WiFi.localIP().toString();
  status();
  pro_connect();
}

void WiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  cls(red);
  wifi_connected = false;
}

void next()
{
  if (!pro_connected)
    pro_connect();
  cls(BLUE, YELLOW);
  status("SENDING: NEXT\n--------------\n");
  client.send(nextJSON);
}
void prev()
{
  if (!pro_connected)
    pro_connect();
  cls(ORANGE, BLACK);
  status("SENDING: PREV\n--------------\n");
  client.send(prevJSON);
}

// put your setup code here, to run once:
void setup()
{
  Serial.begin(1500000);

  M5.begin(true, true, false);
  M5.Axp.ScreenBreath(10);
  M5.Axp.EnableCoulombcounter();
  M5.Lcd.setRotation(1);
  cls(RED);

  status("started...\nconnecting to WiFi...");

  // connect to WiFi
  WiFi.setAutoConnect(true);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiDisconnected, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.begin(SSID, PASSWORD, 0, NULL, true);
}

// RETURN INTEGER VALUES BASED ON ANY VALID BUTTON PRESSES
// NOTHING = 0
// NEXT    = 1
// PREV    = 2
int check_input()
{
  int retval = 0;

  // button B (side button) is always "prev"
  if (M5.BtnB.wasReleased())
  {
    return 2;
  }

  // we do this double check because a long press
  // should activate BEFORE the button is released
  // but only once!
  if (should_handle_button && M5.BtnA.pressedFor(300))
  {
    retval = 2;
    should_handle_button = false;
  }

  // whenever the main button is released,
  // set should_handle_button back to true
  // and optionally return an action
  if (M5.BtnA.wasReleased())
  {
    if (should_handle_button)
      retval = 1;

    should_handle_button = true;
  }

  return retval;
}

void loop()
{
  // dim the screen if it has been 2 seconds since an action
  int now = millis();
  if ((now - lastbutton_time) > 3000)
  {
    M5.Axp.ScreenBreath(7);
  }

  int userdid = check_input();
  if (userdid > 0)
  {
    M5.Axp.ScreenBreath(8);
    lastbutton_time = now;
    switch (userdid)
    {
    case 1:
      next();
      break;
    case 2:
      prev();
      break;
    }
  }

  delay(100);    // just rest for a bit
  client.poll(); // check for a websocket update
  M5.update();   // check for a button update
}
