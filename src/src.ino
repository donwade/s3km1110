
#define RX2 18
#define TX2 19

#include <HardwareSerial.h>
HardwareSerial mySerial(2);

String hex_to_send = "FDFCFBFA0800120000006400000004030201";

void setup() 
{

	// Start the serial communication with a baud rate of 115200
	Serial.begin(115200);

	mySerial.setPins(18,19);
	mySerial.begin(115200);

	// Wait for the serial port to initialize
	while (!Serial) {
	delay(100);
	}

	// sendHexData(hex_to_send);  // start reporting?
	// Hex string to send
}

#if 0

void loop() 
{
 // Read and print serial data
  readSerialData();
}

void sendHexData(String hexString) {
  // Convert hex string to bytes
  int hexStringLength = hexString.length();
  byte hexBytes[hexStringLength / 2];
  for (int i = 0; i < hexStringLength; i += 2) {
    hexBytes[i / 2] = strtoul(hexString.substring(i, i + 2).c_str(), NULL, 16);
  }

  // Send bytes through software serial
  mySerial.write(hexBytes, sizeof(hexBytes));
}

void readSerialData()
{
    static int ctr;
    char keeper[15];

    int limit = sizeof(keeper);

    // Read and print data from software serial
    if  (mySerial.available() > 0) 
    {
        char incomingByte = mySerial.read();
        keeper[ctr++] = incomingByte;

        if (ctr == limit)
        {
            Serial.println();

            for (ctr = 0; ctr < limit; ctr++)
            {
                Serial.printf("%02x ", keeper[ctr]);
            }
 
            for (ctr = 0; ctr < limit; ctr++)
            {
                Serial.printf("%c", (keeper[ctr] > 0x19 && keeper[ctr] < 128) ? 
                    keeper[ctr] : '.') ;
            }
            ctr = 0;
        }
        //Serial.print(incomingByte);
    }
}
#endif

