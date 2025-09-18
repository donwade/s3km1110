#include <Arduino.h>
#include <s3km1110.h>
#include <HardwareSerial.h>

HardwareSerial S3km11110(2);

#if defined(ESP32)
  #ifdef ESP_IDF_VERSION_MAJOR // IDF 4+
    #if CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
      #define MONITOR_SERIAL Serial
      #define RADAR_SERIAL S3km11110
      #define RADAR_RX_PIN 18
      #define RADAR_TX_PIN 19
    #elif CONFIG_IDF_TARGET_ESP32S2
      #define MONITOR_SERIAL Serial
      #define RADAR_SERIAL Serial1
      #define RADAR_RX_PIN 9
      #define RADAR_TX_PIN 8
    #elif CONFIG_IDF_TARGET_ESP32C3
      #define MONITOR_SERIAL Serial
      #define RADAR_SERIAL Serial1
      #define RADAR_RX_PIN 4
      #define RADAR_TX_PIN 5
    #else 
      #error Target CONFIG_IDF_TARGET is not supported
    #endif
  #else // ESP32 Before IDF 4.0
    #define MONITOR_SERIAL Serial
    #define RADAR_SERIAL Serial1
    #define RADAR_RX_PIN 32
    #define RADAR_TX_PIN 33
  #endif
#elif defined(__AVR_ATmega32U4__)
  #define MONITOR_SERIAL Serial
  #define RADAR_SERIAL Serial1
  #define RADAR_RX_PIN 0
  #define RADAR_TX_PIN 1
#endif

s3km1110 radar;

uint32_t lastReading = 0;
uint16_t lastDistance = 0;
bool isDetected = false;

#pragma mark - Lyfe cycle

#define PIN_LED 2

void setup(void)
{
    MONITOR_SERIAL.begin(115200);
    #if defined(ESP32)
    RADAR_SERIAL.begin(115200, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN); //UART for monitoring the radar
    #elif defined(__AVR_ATmega32U4__)
    RADAR_SERIAL.begin(115200); //UART for monitoring the radar
    #endif

	pinMode(PIN_LED, OUTPUT);
	digitalWrite(PIN_LED, 0);

    bool isRadarEnabled = radar.begin(RADAR_SERIAL, MONITOR_SERIAL);
    Serial.printf("Radar status: %s\n", isRadarEnabled ? "Ok" : "Failed");  
    
    radar._enableReportMode();
    
    //radar._enableDebugMode();
    //radar._enableNormalMode();

    if (isRadarEnabled) 
    {
    	radar.setRadarConfigurationDelay(2);
    	radar.setRadarConfigurationMaximumGates(15);
		radar.setRadarConfigurationActiveFrameNum(15); //all frames    	
        if (radar.readAllRadarConfigs()) {
            auto config = radar.radarConfiguration;
            MONITOR_SERIAL.printf("[Info] Radar config:\n |- Gates  | Min: %u\t| Max: %u\n |- Frames | Detect: %u\t| Disappear: %u\n |- Disappearance delay: %u\n",
                                config.detectionGatesMin, config.detectionGatesMax, config.activeFrameNum, config.inactiveFrameNum, config.delay);
        }
    }
    
}

void loop(void)
{
    if (radar.isConnected()) {
        lastReading = millis();
        while (millis() - lastReading < 2000) {
            if (radar.read()) {
                bool newIsDetected = radar.isTargetDetected;
                uint16_t newDistance = radar.distanceToTarget;

                if (isDetected && !newIsDetected) {
                	float inches;
                	uint16_t feet;
                	uint16_t in;
                	
                	inches = (float)lastDistance / 2.54;
                	feet = (uint16_t) inches / 12;
                	in = inches - 12 * feet;
                	
                    MONITOR_SERIAL.printf("[INFO] Target lost on:     %3ucm\t%4.2f\"\t %3u\'-%02u\"\n", 
                    						lastDistance,
                    						inches,
                    						feet, in);
                    isDetected = newIsDetected;
                    lastDistance = newDistance;
                    
					digitalWrite(PIN_LED, 0);
                    return;
                }

                isDetected = newIsDetected;

                if (lastDistance != newDistance) 
                {
                	float inches;
                	uint16_t feet;
                	uint16_t in;
                	inches = (float)newDistance / 2.54;
                	feet = (uint16_t) inches / 12;
                	in = inches - 12 * feet;
                	
                    MONITOR_SERIAL.printf("[INFO] Distance to target: %3ucm\t%3.2f\"\t %3u\'-%02u\"\n", 
                    						newDistance,
                    						inches,
                    						feet, in);
					digitalWrite(PIN_LED, 1);
                }

                lastDistance = newDistance;
            }
        }
    }
}
