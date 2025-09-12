
#ifndef s3km1110_h
#define s3km1110_h

#include <Arduino.h>

#define S3KM1110_MAX_FRAME_LENGTH 45
#define S3KM1110_DISTANCE_GATE_COUNT 16

// #define S3KM1110_DEBUG_COMMANDS
// #define S3KM1110_DEBUG_DATA


extern uint16_t distanceGateEnergy[S3KM1110_DISTANCE_GATE_COUNT];

struct s3km1110ConfigParameters
{
    uint8_t detectionGatesMin;   // 0~15 | Minimum detection distance gate
    uint8_t detectionGatesMax;   // 0~15 | Maximum detection distance gate
    uint8_t activeFrameNum;      // Minimum frame number of detected target
    uint8_t inactiveFrameNum;    // Minimum number of frames when the target disappears 
    uint16_t disappearDelay;              // 0~65535 | Target disappearance delay time (sec)
};

class s3km1110 {

    public:
        s3km1110();
        ~s3km1110();

        bool begin(Stream &dataStream, Stream &debugStream);
        bool isConnected(); // Is the sensor connected and sending data regularly
        bool read();        // You must call this frequently in your main loop to process incoming frames from the sensor

        bool readFirmwareVersion(); // Request the firmware version, which is then available on the values below.
        bool readSerialNumber();    // Request the serial number, which is then available on the values below.

        bool readAllRadarConfigs();
        bool readRadarConfigMinimumGates();
        bool readRadarConfigMaximumGates();
        bool readRadarConfigActiveFrameNumber();
        bool readRadarConfigInactiveFrameNumber();
        bool readRadarConfigDelay();

        bool setRadarConfigurationMinimumGates(uint8_t);
        bool setRadarConfigurationMaximumGates(uint8_t);
        bool setRadarConfigurationActiveFrameNum(uint8_t);
        bool setRadarConfigurationInactiveFrameNum(uint8_t);
        bool setRadarConfigurationDelay(uint16_t);

        char  mFirmwareVersion[30];
        char  mSerialNumber[30];

        s3km1110ConfigParameters *radarConfiguration;

        bool bTargetDetected = false;
        int16_t distanceToTarget = -1;  // Distance to the target in centimetres.

    protected:
    private:
        Stream *_uartRadar = nullptr;
        Stream *_uartDebug = nullptr;

        const uint32_t kRadarUartcommandTimeout = 100;
        uint32_t _radarUartLastPacketTime = 0;
        uint32_t _radarUartLastCommandTime = 0;

        uint8_t _radarDataFrame[S3KM1110_MAX_FRAME_LENGTH];
        int8_t _radarDataFramePosition = 0;

        bool _isFrameStarted = false;
        bool _isCommandFrame = false;

        uint8_t mlastRxCommand = 0;
        uint8_t commandRequested = 0;
        bool mbLastRxCmdValid = false;

        bool _enableReportMode();
        void _printCurrentFrame();

        bool _read_frame();
        bool _isDataFrameComplete();
        bool _isCommandFrameComplete();
		bool _parseDataFrame();
		bool _parseCommandFrame();
        bool _parseGetConfigCommandFrame(char*, uint8_t);

        void _sendHexData(String);
        bool _sendCommandAndWait(uint16_t, uint32_t, uint8_t, bool isSkipCommandMode = false);
        bool _sendCommandAndWait(uint16_t, uint32_t, uint8_t, uint32_t, uint8_t, bool isSkipCommandMode = false);
        bool _setParameterConfiguration(uint16_t parameter, int value);

        String _intToHex(int, uint8_t);

        bool _openCommandMode();
        bool _closeCommandMode();

        inline String _getCommandPrefix() { return "FDFCFBFA"; }
        inline String _getCommandPostfix() { return "04030201"; }
};

#endif // s3km1110_h
