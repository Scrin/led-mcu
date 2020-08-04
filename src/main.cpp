#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include "common.h"

#ifdef HTTPUpdateServer
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>

ESP8266WebServer httpUpdateServer(80);
ESP8266HTTPUpdateServer httpUpdater;
#endif

#define LED_ON LOW
#define LED_OFF HIGH

const char *ssid = USER_SSID;
const char *password = USER_PASSWORD;
const char *mqtt_server = USER_MQTT_SERVER;
const int mqtt_port = USER_MQTT_PORT;
const char *mqtt_user = USER_MQTT_USERNAME;
const char *mqtt_pass = USER_MQTT_PASSWORD;
const char *mqtt_client_name = USER_MQTT_CLIENT_NAME;

// Globals
SimpleTimer timer;
RgbwColor stripLeds[NUM_LEDS] = {};
RgbwColor customLeds[NUM_LEDS] = {};
byte enabledLeds[NUM_LEDS / 8 + 1] = {};
Effect effect = eStable;
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;
uint8_t white = 0;
char gradientMode = 'E';
int gradientExtent = 50;
int sunriseDuration = NUM_LEDS;

// Locals
WiFiClient espClient;
PubSubClient client(espClient);
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(NUM_LEDS);
bool boot = true;
bool on = true;
char charPayload[MQTT_MAX_PACKET_SIZE];

int char2int(char input)
{
  if (input >= '0' && input <= '9')
  {
    return input - '0';
  }
  if (input >= 'A' && input <= 'F')
  {
    return input - 'A' + 10;
  }
  if (input >= 'a' && input <= 'f')
  {
    return input - 'a' + 10;
  }
  return 0;
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  String newTopic = topic;
  Serial.print(topic);
  Serial.print("] ");
  memset(&charPayload, 0, sizeof(charPayload));
  memcpy(charPayload, payload, min(static_cast<unsigned int>(sizeof(charPayload)), length));
  charPayload[length] = '\0';
  String newPayload = String(charPayload);
  int intPayload = newPayload.toInt();
  Serial.println(newPayload);
  Serial.println();
  newPayload.toCharArray(charPayload, newPayload.length() + 1);

  if (newTopic == USER_MQTT_CLIENT_NAME "/command")
  {
    if (strcmp(charPayload, "OFF") == 0)
    {
      on = false;
      stopEffect();
    }
    else if (strcmp(charPayload, "ON") == 0)
    {
      on = true;
      startEffect(effect);
    }
    else
    {
      Serial.print("Unknown command: ");
      Serial.println(charPayload);
      return;
    }
    client.publish(USER_MQTT_CLIENT_NAME "/state", charPayload, true);
  }
  else if (newTopic == USER_MQTT_CLIENT_NAME "/effect")
  {
    if (strcmp(charPayload, "stable") == 0)
    {
      startEffect(eStable);
    }
    else if (strcmp(charPayload, "colorloop") == 0)
    {
      startEffect(eColorLoop);
    }
    else if (strcmp(charPayload, "gradient") == 0)
    {
      startEffect(eGradient);
    }
    else if (strcmp(charPayload, "custom") == 0)
    {
      startEffect(eCustom);
    }
    else if (strcmp(charPayload, "sunrise") == 0)
    {
      startEffect(eSunrise);
    }
    else
    {
      Serial.print("Unknown effect: ");
      Serial.println(charPayload);
      return;
    }
    client.publish(USER_MQTT_CLIENT_NAME "/effectState", charPayload, true);
  }
  else if (newTopic == USER_MQTT_CLIENT_NAME "/wakeAlarm")
  {
    on = true;
    sunriseDuration = intPayload;
    startEffect(eSunrise);
    client.publish(USER_MQTT_CLIENT_NAME "/state", "ON", true);
    client.publish(USER_MQTT_CLIENT_NAME "/effect", "sunrise", true);
    client.publish(USER_MQTT_CLIENT_NAME "/effectState", "sunrise", true);
  }
  else if (newTopic == USER_MQTT_CLIENT_NAME "/setGradient")
  {
    if (newPayload.length() >= 3)
    {
      char mode = newPayload[0];
      switch (mode)
      {
      case 'N':
      case 'F':
      case 'C':
      case 'E':
        gradientMode = mode;
        gradientExtent = newPayload.substring(2).toInt();
        break;
      default:
        Serial.print("Invalid gradient: ");
        Serial.println(charPayload);
        return;
      }
      startEffect(eGradient);
      client.publish(USER_MQTT_CLIENT_NAME "/effect", "gradient", true);
      client.publish(USER_MQTT_CLIENT_NAME "/effectState", "gradient", true);
    }
    else
    {
      Serial.print("Invalid gradient: ");
      Serial.println(charPayload);
      return;
    }
  }
  else if (newTopic == USER_MQTT_CLIENT_NAME "/setCustom")
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      customLeds[i] = RgbwColor(0, 0, 0, 0);
    }
    for (int i = 0; charPayload[i] && i / 8 <= NUM_LEDS; i++)
    {
      int value;
      if (i % 2)
      {
        value = char2int(charPayload[i]);
      }
      else
      {
        value = char2int(charPayload[i]) << 4;
      }
      switch (i / 2 % 4)
      {
      case 0:
        customLeds[i / 8].R |= value;
        break;
      case 1:
        customLeds[i / 8].G |= value;
        break;
      case 2:
        customLeds[i / 8].B |= value;
        break;
      case 3:
        customLeds[i / 8].W |= value;
        break;
      }
    }
    startEffect(eCustom);
    client.publish(USER_MQTT_CLIENT_NAME "/effect", "custom", true);
    client.publish(USER_MQTT_CLIENT_NAME "/effectState", "custom", true);
  }
  else if (newTopic == USER_MQTT_CLIENT_NAME "/setEnabledLeds")
  {
    memset(&enabledLeds, 0, sizeof(enabledLeds));
    for (int i = 0; charPayload[i] && i / 2 <= NUM_LEDS / 8; i++)
    {
      if (i % 2)
      {
        enabledLeds[i / 2] |= char2int(charPayload[i]);
      }
      else
      {
        enabledLeds[i / 2] = char2int(charPayload[i]) << 4;
      }
    }
  }
  else if (newTopic == USER_MQTT_CLIENT_NAME "/white")
  {
    white = intPayload;
    switch (effect)
    {
    case eCustom:
    case eSunrise:
    case eColorLoop:
      // Setting the white value should stop effects that don't use the configured color
      effect = eStable;
      client.publish(USER_MQTT_CLIENT_NAME "/effect", "stable", true);
      client.publish(USER_MQTT_CLIENT_NAME "/effectState", "stable", true);
      break;
    default:
      break;
    }
    startEffect(effect);
    client.publish(USER_MQTT_CLIENT_NAME "/whiteState", charPayload, true);
  }
  else if (newTopic == USER_MQTT_CLIENT_NAME "/color")
  {
    int firstIndex = newPayload.indexOf(',');
    int lastIndex = newPayload.lastIndexOf(',');
    if ((firstIndex > -1) && (lastIndex > -1) && (firstIndex != lastIndex))
    {
      red = newPayload.substring(0, firstIndex).toInt();
      green = newPayload.substring(firstIndex + 1, lastIndex).toInt();
      blue = newPayload.substring(lastIndex + 1).toInt();
      switch (effect)
      {
      case eCustom:
      case eSunrise:
      case eColorLoop:
        // Setting the color should stop effects that don't use the configured color
        effect = eStable;
        client.publish(USER_MQTT_CLIENT_NAME "/effect", "stable", true);
        client.publish(USER_MQTT_CLIENT_NAME "/effectState", "stable", true);
        break;
      default:
        break;
      }
      startEffect(effect);
      client.publish(USER_MQTT_CLIENT_NAME "/colorState", charPayload, true);
    }
  }
}

void setup_wifi()
{
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  WiFi.mode(WIFI_STA);

  WiFi.hostname(USER_MQTT_CLIENT_NAME);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect()
{
  // Loop until we're reconnected
  int retries = 0;
  while (!client.connected())
  {
    if (retries < 150)
    {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass, USER_MQTT_CLIENT_NAME "/availability", 0, true, "offline"))
      {
        Serial.println("connected");
        client.publish(USER_MQTT_CLIENT_NAME "/availability", "online", true);
        if (boot)
        {
          client.publish(USER_MQTT_CLIENT_NAME "/checkIn", "rebooted");
          boot = false;
        }
        else
        {
          client.publish(USER_MQTT_CLIENT_NAME "/checkIn", "reconnected");
        }
        client.subscribe(USER_MQTT_CLIENT_NAME "/command");
        client.subscribe(USER_MQTT_CLIENT_NAME "/effect");
        client.subscribe(USER_MQTT_CLIENT_NAME "/color");
        client.subscribe(USER_MQTT_CLIENT_NAME "/white");
        client.subscribe(USER_MQTT_CLIENT_NAME "/wakeAlarm");
        client.subscribe(USER_MQTT_CLIENT_NAME "/setGradient");
        client.subscribe(USER_MQTT_CLIENT_NAME "/setCustom");
        client.subscribe(USER_MQTT_CLIENT_NAME "/setEnabledLeds");
      }
      else
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    else
    {
      ESP.restart();
    }
  }
}

#ifdef HTTPUpdateServer
void setup_http_server()
{
  MDNS.begin(USER_MQTT_CLIENT_NAME);

  httpUpdater.setup(&httpUpdateServer, USER_HTTP_USERNAME, USER_HTTP_PASSWORD);
  httpUpdateServer.begin();

  MDNS.addService("http", "tcp", 80);
}
#endif

void setup()
{
  memset(&enabledLeds, 0xff, sizeof(enabledLeds));
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_ON);

  Serial.begin(115200);
  strip.Begin();
  strip.Show();

  setup_wifi();

#ifdef HTTPUpdateServer
  setup_http_server();
#endif

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  digitalWrite(LED_BUILTIN, LED_OFF);
}

void loop()
{
  if (!client.connected())
  {
    digitalWrite(LED_BUILTIN, LED_ON);
    reconnect();
    digitalWrite(LED_BUILTIN, LED_OFF);
  }
  client.loop();
  timer.run();

  if (on)
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      if (enabledLeds[i / 8] >> (7 - (i % 8)) & 1)
      {
        strip.SetPixelColor(i, stripLeds[i]);
      }
      else
      {
        strip.SetPixelColor(i, RgbwColor(0, 0, 0, 0));
      }
    }
  }
  else
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      strip.SetPixelColor(i, RgbwColor(0, 0, 0, 0));
    }
  }

  strip.Show();

#ifdef HTTPUpdateServer
  httpUpdateServer.handleClient();
#endif
}
