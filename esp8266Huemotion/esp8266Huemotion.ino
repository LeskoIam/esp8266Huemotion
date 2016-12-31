/*
  esp8266Huemotion
  https://github.com/LeskoIam/esp8266Huemotion
  polensek.matevz@gmail.com

  Used instead of very expensive Hue motion sensor.
  It's basicly a D1 mini, which breaks out pins from esp8266 WiFi module and adds some
  periphery to enable USB serial communication.

  Operation:
  It uses PIR sensor attached to pin D1 (arduino library maps this pin to pin 5) to
  detect motion and turn on specified Hue light bulb if all conditions are met. These
  conditions are:
    - It's night time. This is determined based on if specified light bulb is on or off.
      State of the light bulb is checked and updated every set number of minutes
      ("lights_check_delay").
    - If motion is detected. Specified light bulb is turned on for set amount of time
      ("motion_detected_delay"). Motion is still detected and updated when light is turned on,
      this means light is turned off only after no motion is detected for set delay.

  The circuit:
  - Philips Hue lights
  - D1 mini or clone (esp8266).
  - PIR motion sensor (HC-SR501).

  Settings:
  Look for code in #### block.
  Mandatory settings:
    - ssid
    - password
    - bridge_ip, port
    - user
    - light
  Optional:
    - motion_detected_delay
    - lights_check_delay
    - hue_on
    - hue_off

*/
#include <ESP8266WiFi.h>

//// Global settings and variables
// ################################################
// Wifi Settings
const char* ssid = "<your_ssid_name>";
const char* password = "<your_wifi_password>";

// Hue settings
const char* bridge_ip = "<your_hue_bridge_ip>";  // Hue bridge IP
const int port = 80;
String user = "<your_hue_bridge_user>";
String light = "<number_of_the_light_you_want_to_control>";  // Number of the light you want to control

// Motion timing
unsigned long motion_detected_time = 0;         // When was motion last detected
unsigned long motion_detected_delay = 60*1000;  // Keep light on for that many seconds after last motion was detected

// All lights off check timing
unsigned long lights_check_time = 0;            // When was light state last checked
unsigned long lights_check_delay = 5*20*1000;   // Check light state every that many minutes

// Commands
String hue_on = "{\"on\":true, \"bri\":5, \"xy\":[0.1540,0.0806]}";
String hue_off = "{\"on\":false}";
// ################################################

// Pin settings
// PIR sensor is attached to D1 mini D1 pin which maps to pin 5 for arduino library
int pirPin = 5;

int light_state = 0;              // Internally track state of light
int first_loop = 1;               // Is this first loop
int check_lights_first_loop = 1;  // Same as above but for checking light state
bool night_time;

void setup()
{
  Serial.begin(115200);
  delay(10);
  
  // Connect to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set PIR pin as input pin
  pinMode(pirPin, INPUT);

  Serial.println("Waiting 2 seconds for stable sensor readings...");
  delay(1000);
  Serial.println("Setup done. Main loop starting in a second...");
  delay(1000);
  Serial.println();
}

void loop() 
{
  //// Check state of the light and set nigh_time accordingly
  if ((((lights_check_time + lights_check_delay) < millis()) || check_lights_first_loop == 1) && (light_state != 1))
  {
    check_lights_first_loop = 0;
    if (is_light_on() == 1) { night_time = 0; }
    else { night_time = 1; }
    lights_check_time = millis();
  }

  // Some debug prints
  Serial.println("-----");
  Serial.print("night_time: ");
  Serial.println(night_time);
  Serial.print("motion_detected_time: ");
  Serial.println(motion_detected_time);
  Serial.print("light_state: ");
  Serial.println(light_state);

  //// Main motion-light logic
  // Enter only if it is nigh time and ligh is not on because of us
  if ((night_time == 1) || (light_state == 1))
  {
    // Read PIR sensor
    int motion = digitalRead(pirPin);
    Serial.print("motion: ");
    Serial.println(motion);

    // If motion is detected
    if (motion == HIGH)
    {
      // And light is off because of us.
      // This also prevents multiple turn ons (sensor output stays on for around 2 seconds)
      if (light_state == 0)
      {
        // Turn light on only if previous on time delay has passed or if this is first loop
        // first_loop check was added to handle situation when motion-light has
        // not yet been running for more than motion delay time
        if (((motion_detected_time + motion_detected_delay) < millis()) || first_loop == 1)
        {
          Serial.println("Turning light on");
          light_control(hue_on);
          light_state = 1;
          first_loop = 0;
        }
      }
      // Detect every motion and update detection time
      motion_detected_time = millis();
    }
    else
    {
      // Only turn off light if they were turned on by us
      if (light_state == 1)
      {
        // Turn light off only if on time delay has passed
        if ((motion_detected_time + motion_detected_delay) < millis())
        {
          Serial.print("No motion for ");
          Serial.print(motion_detected_delay/1000);
          Serial.println(" seconds. Turning light off");
          light_control(hue_off);
          light_state = 0;
        }
      }
    }
  }
  delay(333);
}


/*  light_control.
 *  Send PUT command to hue light (bridge). Function takes json formated command.
 *  Returns:
 *  1 if operation successful
 *  0 if operation not successful
 *  -1 if error occurred 
 */
bool light_control(String command)
{
  int retval = 0;     // return value
  WiFiClient client;  // WiFiClient class to create TCP connections
  
  if (!client.connect(bridge_ip, port))
  {
    Serial.println("ERR>> light_control - Connection failed");
    return -1;
  }
  
  // This will send PUT request to the server
  client.println("PUT /api/" + user + "/lights/" + light + "/state");
  client.println("Host: " + String(bridge_ip) + ":" + String(port));
  client.println("User-Agent: ESP8266/1.0");
  client.println("Connection: keep-alive");
  client.println("Content-type: text/xml; charset=\"utf-8\"");
  client.print("Content-Length: ");
  client.println(command.length()); // PUT COMMAND HERE
  client.println();
  client.println(command); // PUT COMMAND HERE

  // Wait 5 seconds for server to respond
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 5000)
    {
      Serial.println("ERR>> light_control - Client timeout");
      client.stop();
      return -1;
    }
  }
  
  // Read all the lines of the reply from server
  while(client.available())
  {
    String line = client.readStringUntil('\r');
    // Print line to serial if it's request status or json formated string
    if (((line.indexOf("{") != -1) && (line.indexOf("}") != -1)) || (line.indexOf("HTTP") != -1)){ Serial.print(line); }
    // If success string is found in reply we have successfully sexecuted command
    if (line.indexOf("\"success\":") != -1){ retval = 1; }
  }
  Serial.println();
  client.stop();
  return retval;
}

/*  are_all_lights_on
 *  Returns:
 *  1 if operation successful
 *  0 if operation not successful
 *  -1 if error occurred 
 */
bool are_all_lights_on()
{
  int retval = 0;     // return value
  WiFiClient client;  // WiFiClient class to create TCP connections
  
  if (!client.connect(bridge_ip, port))
  {
    Serial.println("ERR>> are_all_lights_on - Connection failed");
    return -1;
  }
  
  // This will send GET request to the server
  client.println("GET /api/" + user + "/lights HTTP/1.1");
  client.println("Host: " + String(bridge_ip) + ":" + String(port));
  client.println("Connection: close");

  // Wait maximum of 5 seconds for server to respond
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 5000)
    {
      Serial.println("ERR>> are_all_lights_on - Client timeout");
      client.stop();
      return -1;
    }
  }
  
  // Read all the lines of the reply from server
  while(client.available())
  {
    String line = client.readStringUntil('\r');
    // Print line to serial if it's request status or json formated string
    if (((line.indexOf("{") != -1) && (line.indexOf("}") != -1)) || (line.indexOf("HTTP") != -1)){ Serial.print(line); }
    // If any light is off - all lights are not on
    if (line.indexOf("\"on\":false") == -1){ retval = 1; }
  }
  Serial.println();
  client.stop();
  return retval;
}

/* is_light_on.
 *  Returns:
 *  1 if operation successful
 *  0 if operation not successful
 *  -1 if error occurred 
 */
bool is_light_on()
{
  int retval = 0;
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  
  if (!client.connect(bridge_ip, port))
  {
    Serial.println("ERR>> is_light_on - Connection failed");
    return -1;
  }
  
  // This will send GET request to the server
  client.println("GET /api/" + user + "/lights/" + light + " HTTP/1.1");
  client.println("Host: " + String(bridge_ip) + ":" + String(port));
  client.println("Connection: close");
  
  // Wait maximum of 5 seconds for server to respond
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 5000)
    {
      Serial.println("ERR>> is_light_on - Client timeout");
      client.stop();
      return -1;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available())
  {
    String line = client.readStringUntil('\r');
    // Print line to serial if it's request status or json formated string
    if (((line.indexOf("{") != -1) && (line.indexOf("}") != -1)) || (line.indexOf("HTTP") != -1)){ Serial.print(line); }
    // Check if light is on
    if (line.indexOf("\"on\":true") != -1){ retval = 1; }
  }
  Serial.println();
  client.stop();
  return retval;
}

