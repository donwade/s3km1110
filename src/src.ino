#include <../include/s3km1110.h>


// give hardware serial a heads up we are not standard																		  
#define RX2 18
#define TX2 19
 
#include <HardwareSerial.h>
HardwareSerial customSerial(2);

s3km1110 radar;

uint32_t lastReading = 0;

void setup(void) {
    Serial.begin(115200);

	customSerial.setPins(RX2, TX2);  // runtime overide
    customSerial.begin(115200);

    bool isRadarEnabled = radar.begin(customSerial, Serial);
    Serial.printf("Radar status: %s\n", isRadarEnabled ? "Ok" : "Failed");

    if (isRadarEnabled && radar.readAllRadarConfigs()) {
        auto config = radar.radarConfiguration;
        Serial.printf("[Info] Radar config:\n |- Gates  | Min: %u\t| Max: %u\n |- Frames | Detect: %u\t| Disappear: %u\n |- Disappearance delay: %u\n",
                    config->detectionGatesMin, config->detectionGatesMax, config->activeFrameNum, config->inactiveFrameNum, config->delay);
    }
}

void loop() {
    if (radar.isConnected()) {
        lastReading = millis();
        while (millis() - lastReading < 2000) {
            if (radar.read()) {
                // Get radar info
                bool isDetected = radar.isTargetDetected;
                int16_t targetDistance = radar.distanceToTarget;
            }
        }
    }
}
