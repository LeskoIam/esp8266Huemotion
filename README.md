# esp8266Huemotion
  
Used instead of very expensive Hue motion sensor.
It's basicly a [D1 mini](https://www.wemos.cc/product/d1-mini.html), which breaks out pins from [ESP8266](https://en.wikipedia.org/wiki/ESP8266) WiFi module and adds some periphery to enable USB serial communication.

<img src="http://i.imgur.com/AjdKpPT.jpg?1" width="640">


## Operation:
It uses PIR sensor attached to pin D1 (arduino library maps this pin to pin 5) to detect motion and turn on specified Hue light bulb if all conditions are met. These conditions are:
* It's night time. This is determined based on if specified light bulb is on or off. State of the light bulb is checked and updated every set number of minutes ("lights_check_delay").
*  If motion is detected. Specified light bulb is turned on for set amount of time ("motion_detected_delay"). Motion is still detected and updated when light is turned on, this means light is turned off only after no motion is detected for set delay.

## The circuit:
  * [Philips Hue](http://www2.meethue.com/en-us) lights
  * [D1 mini](https://www.wemos.cc/product/d1-mini.html) or [clone](http://www.ebay.co.uk/itm/ESP8266-D1-Mini-Clone-WIFI-Dev-Kit-Development-Board-for-Arduino-ESP-NodeMCU-Lua-/251863466044) ([ESP8266](https://en.wikipedia.org/wiki/ESP8266)).
  * PIR motion sensor ([HC-SR501](http://www.ebay.com/itm/10pcs-HC-SR501-Adjust-IR-Pyroelectric-Infrared-PIR-Motion-Sensor-Detector-Module-/131028677440)).

## Settings:
Look for code in "####" block at the begining of code.

* Mandatory:
  * ssid - wifi network name
  * password - wifi password
  * bridge_ip, port - Hue bridge ip and port
  * user - Hue bridge username
  * light - number of controlled light

* Optional:
  * motion_detected_delay
  * lights_check_delay
  * hue_on - command send to Hue bridge when turning on the light
  * hue_off - command send to Hue bridge when turning off the light