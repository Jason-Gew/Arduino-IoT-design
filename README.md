# Arduino-IoT-design
# Designer: Jason/Ge Wu   jason.ge.wu@gmail.com

This is an IoT edge design in order to upload sensor data to the cloud server. Hardware base on Arduino UNO and WiFi. 
Communication based on MQTT protocol.

This code provides a standard data measuring function of DHT11 humidity sensor and also provides percentage feedback function of light sensor or similar analog sensors.

The most reliable version of compiler is Arduino IDE 1.0.4 correspond to the previous version of Arduino WiFi firmware.
Since Arduino Official WiFi library may not be compatible for all firmware, when you are using the old version of Arduino WiFi Shield, please use Arduino IDE 1.0.4 to compile your code.

