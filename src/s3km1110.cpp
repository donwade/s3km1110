#ifndef s3km1110_cpp
#define s3km1110_cpp

#include "../include/s3km1110.h"
#include "common.h"

#define S3KM1110_FRAME_COMMAND_SIZE 2
#define S3KM1110_FRAME_LENGTH_SIZE 2

#define CONFIG_DETECTION_DISTANCE_MIN 0x00
#define CONFIG_DETECTION_DISTANCE_MAX 0x01
#define CONFIG_TARGET_ACTIVE_FRAMES 0x02
#define CONFIG_TARGET_INACTIVE_FRAMES 0x03
#define CONFIG_DISAPPEARANCE_DELAY 0x04

#define MODE_DEBUG 0x00
#define MODE_REPORT 0x04
#define MODE_RUNNING 0x64

struct s3km1110ConfigParameters parkingLot;

#define min(a,b) (((a) < (b)) ? (a) :(b))
#define max(a,b) (((a) > (b)) ? (a) :(b))

#define COMMAND_READ_FIRMWARE_VERSION 0x00
#define COMMAND_WRITE_REGISTER 01
#define COMMAND_READ_REGISTER 02
#define COMMAND_FACTORY_TEST_ENTER 0x24
#define COMMAND_FACTORY_TEST_ENTER 0x25
#define COMMAND_FACTORY_GET_TEST 0x26
#define COMMAND_RADAR_SET_CONFIG 0x07
#define COMMAND_RADAR_READ_CONFIG 0x08
#define COMMAND_READ_SERIAL_NUMBER 0x11
#define COMMAND_SET_MODE 0x12
#define COMMAND_READ_MODE 0x13
#define COMMAND_OPEN_COMMAND_MODE 0xFF
#define COMMAND_CLOSE_COMMAND_MODE 0xFE

//======================================================================

typedef struct {
	int num;
	char *msg;
} CMD2TEXT;

CMD2TEXT cmdString[] =
{
	{ -1,   "COMMAND_NOT HANDLED "},
	{ 0x00, "COMMAND_READ_FIRMWARE_VERSION "},
	{ 0x01, "COMMAND_WRITE_REGISTER "},
	{ 0x02, "COMMAND_READ_REGISTER "},
	{ 0x24, "COMMAND_FACTORY_TEST_ENTER "},
	{ 0x25, "COMMAND_FACTORY_TEST_ENTER "},
	{ 0x26, "COMMAND_FACTORY_GET_TEST "},
	{ 0x07, "COMMAND_RADAR_SET_CONFIG "},
	{ 0x08, "COMMAND_RADAR_READ_CONFIG "},
	{ 0x11, "COMMAND_READ_SERIAL_NUMBER "},
	{ 0x12, "COMMAND_SET_MODE "},
	{ 0x13, "COMMAND_READ_MODE "},
	{ 0xFF, "COMMAND_OPEN_COMMAND_MODE "},
	{ 0xFE, "COMMAND_CLOSE_COMMAND_MODE "}
	
};

const char *cmd2String(int cmdNum)
{
	for (int i = 0; i < sizeof(cmdString)/sizeof(cmdString[0]); i++)
	{
		if (cmdNum == cmdString[i].num) return cmdString[i].msg;
	}
	return cmdString[0].msg;
}
//======================================================================

s3km1110::s3km1110() : radarConfiguration(&parkingLot) {};
s3km1110::~s3km1110() = default;

#pragma mark - Public

bool s3km1110::begin(Stream &dataStream, Stream &debugStream)
{
    // UARTs
    _uartRadar = &dataStream;
    _uartDebug = &debugStream;
    #if defined(ESP8266)
    if (&debugStream == &Serial) {
        _uartDebug->write(17); //Send an XON to stop the hung terminal after reset on ESP8266
    }
    #endif

    if (!_uartRadar) {
        return false;
    }

    return _enableReportMode();
}

bool s3km1110::isConnected()
{
    if (millis() - _radarUartLastPacketTime < kRadarUartcommandTimeout) { return true; }
    if (_read_frame()) { return true; }
    return false;
}

bool s3km1110::read()
{
    return _read_frame();
}

#pragma mark - Send command

bool s3km1110::_enableReportMode() 
{
    return _sendCommandAndWait(COMMAND_SET_MODE, 0, 2, MODE_REPORT, 4);
}

bool s3km1110::readFirmwareVersion()
{
    return _sendCommandAndWait(COMMAND_READ_FIRMWARE_VERSION, 0, 0);
}

bool s3km1110::readSerialNumber()
{
    return _sendCommandAndWait(COMMAND_READ_SERIAL_NUMBER, 0, 0);
}

#pragma mark * Radar Configuration Set

bool s3km1110::setRadarConfigurationMinimumGates(uint8_t gates)
{
    uint8_t newValue = max(0, min(15, gates));
    bool isSuccess = _setParameterConfiguration(CONFIG_DETECTION_DISTANCE_MIN, newValue);
	TRACE("setting minimum number of gates to %d = %s\n", newValue, isSuccess? "PASS":"FAIL");
    if (isSuccess) { radarConfiguration->detectionGatesMin = newValue; }
    return isSuccess;
}

bool s3km1110::setRadarConfigurationMaximumGates(uint8_t gates)
{
    uint8_t newValue = max(0, min(15, gates));
    bool isSuccess = _setParameterConfiguration(CONFIG_DETECTION_DISTANCE_MAX, newValue);
	TRACE("setting minimum number of gates to %d = %s\n", newValue, isSuccess? "PASS":"FAIL");
    if (isSuccess) { radarConfiguration->detectionGatesMax = newValue; }
    return isSuccess;
}

bool s3km1110::setRadarConfigurationActiveFrameNum(uint8_t num)
{
    bool isSuccess = _setParameterConfiguration(CONFIG_TARGET_ACTIVE_FRAMES, num);
    if (isSuccess) { radarConfiguration->activeFrameNum = num; }
    return isSuccess;
}

bool s3km1110::setRadarConfigurationInactiveFrameNum(uint8_t num)
{
    bool isSuccess = _setParameterConfiguration(CONFIG_TARGET_INACTIVE_FRAMES, num);
    if (isSuccess) { radarConfiguration->inactiveFrameNum = num; }
    return isSuccess;
}

bool s3km1110::setRadarConfigurationDelay(uint16_t delay)
{
    bool isSuccess = _setParameterConfiguration(CONFIG_DISAPPEARANCE_DELAY, delay);
    if (isSuccess) { radarConfiguration->disappearDelay = delay; }
    return isSuccess;
}

#pragma mark * Radar Configuration Read

bool s3km1110::readRadarConfigMinimumGates()
{
    commandRequested = CONFIG_DETECTION_DISTANCE_MIN;
    return _sendCommandAndWait(COMMAND_RADAR_READ_CONFIG, CONFIG_DETECTION_DISTANCE_MIN, 2);
}

bool s3km1110::readRadarConfigMaximumGates()
{
    commandRequested = CONFIG_DETECTION_DISTANCE_MAX;
    return _sendCommandAndWait(COMMAND_RADAR_READ_CONFIG, CONFIG_DETECTION_DISTANCE_MAX, 2);
}

bool s3km1110::readRadarConfigActiveFrameNumber()
{
    commandRequested = CONFIG_TARGET_ACTIVE_FRAMES;
    return _sendCommandAndWait(COMMAND_RADAR_READ_CONFIG, CONFIG_TARGET_ACTIVE_FRAMES, 2);
}

bool s3km1110::readRadarConfigInactiveFrameNumber()
{
    commandRequested = CONFIG_TARGET_INACTIVE_FRAMES;
    return _sendCommandAndWait(COMMAND_RADAR_READ_CONFIG, CONFIG_TARGET_INACTIVE_FRAMES, 2);
}

bool s3km1110::readRadarConfigDelay()
{
    commandRequested = CONFIG_DISAPPEARANCE_DELAY;
    bool ret = _sendCommandAndWait(COMMAND_RADAR_READ_CONFIG, CONFIG_DISAPPEARANCE_DELAY, 2);
    return ret;
}

bool s3km1110::readAllRadarConfigs()
{
    return
        readRadarConfigMinimumGates() &&
        readRadarConfigMaximumGates() &&
        readRadarConfigActiveFrameNumber() &&
        readRadarConfigInactiveFrameNumber() &&
        readRadarConfigDelay();
}

#pragma mark - Private

// collect up the rx stream. make a frame.
// see if the frame is a data or command repsonse
// call data or command rx processor as required.

bool s3km1110::_read_frame()
{
    if (!_uartRadar->available()) { return false; }

    bool isSuccess = false;
    uint8_t _readData = _uartRadar->read();

    if (_isFrameStarted == false) 
	{
        if (_readData == 0xF4) 
		{
            _radarDataFrame[_radarDataFramePosition++] = _readData;
            _isFrameStarted = true;
            _isCommandFrame = false;
        }
		else if (_readData == 0xFD) 
		{
            _radarDataFrame[_radarDataFramePosition++] = _readData;
            _isFrameStarted = true;
            _isCommandFrame = true;
        }
    } 
	else
	{
        if (_radarDataFramePosition >= S3KM1110_MAX_FRAME_LENGTH) 
		{
            #if defined(S3KM1110_DEBUG_COMMANDS) || defined(S3KM1110_DEBUG_DATA)
            if (_uartDebug != nullptr) 
			{
                _uartDebug->println(F("[Error] Frame out of size"));
            }
            #endif
			
            _radarDataFramePosition = 0;
            _isFrameStarted = false;
			
        } 
		else
		{
            _radarDataFrame[_radarDataFramePosition++] = _readData;

            if (_radarDataFramePosition >= 8) 
			{
                if (_isDataFrameComplete()) 
				{
                    isSuccess = _parseDataFrame();
                    _isFrameStarted = false;
                    _radarDataFramePosition = 0;
                }
				else if (_isCommandFrameComplete()) 
                {
                    isSuccess = _parseCommandFrame();
                    _isFrameStarted = false;
                    _radarDataFramePosition = 0;
                }
            }
        }
    }

    return isSuccess;
}

void s3km1110::_printCurrentFrame()
{
    #if defined(S3KM1110_DEBUG_COMMANDS) || defined(S3KM1110_DEBUG_DATA)
    if (_uartDebug == nullptr) { return; }
    for (uint8_t idx = 0; idx < _radarDataFramePosition; idx++) {
        if (_radarDataFrame[idx] < 0x10) { _uartDebug->print('0'); }
        _uartDebug->print(_radarDataFrame[idx], HEX);
        _uartDebug->print(' ');
    }
    _uartDebug->println(' ');
    #endif
}

bool s3km1110::_isDataFrameComplete()
{
    return 
        _radarDataFrame[0]                              == 0xF4 &&
        _radarDataFrame[1]                              == 0xF3 &&
        _radarDataFrame[2]                              == 0xF2 &&
        _radarDataFrame[3]                              == 0xF1 &&
        _radarDataFrame[_radarDataFramePosition - 4] == 0xF8 &&
        _radarDataFrame[_radarDataFramePosition - 3] == 0xF7 &&
        _radarDataFrame[_radarDataFramePosition - 2] == 0xF6 &&
        _radarDataFrame[_radarDataFramePosition - 1] == 0xF5;
}

bool s3km1110::_isCommandFrameComplete()
{
    return
        _radarDataFrame[0]                              == 0xFD &&
		_radarDataFrame[1]                              == 0xFC &&
		_radarDataFrame[2]                              == 0xFB &&
		_radarDataFrame[3]                              == 0xFA &&
        _radarDataFrame[_radarDataFramePosition - 4] == 0x04 &&
        _radarDataFrame[_radarDataFramePosition - 3] == 0x03 &&
        _radarDataFrame[_radarDataFramePosition - 2] == 0x02 &&
        _radarDataFrame[_radarDataFramePosition - 1] == 0x01;
}

//--------------------------------------------------------------------
// looks like only one type of data frame is processed and that is
// the energy gate data.

bool s3km1110::_parseDataFrame()
{
    uint8_t frame_data_length = _radarDataFrame[4] + (_radarDataFrame[5] << 8);

    #ifdef S3KM1110_DEBUG_DATA
    if (_uartDebug != nullptr) {
        _uartDebug->println(F("----------------------------------------"));
        _uartDebug->print(F("RCV DTA: "));
        _printCurrentFrame();
    }
    #endif

    if (frame_data_length == 35) 
	{
        uint8_t detectionResultRaw = _radarDataFrame[6];
        distanceToTarget = _radarDataFrame[7] + (_radarDataFrame[8] << 8);
        isTargetDetected = detectionResultRaw == 0x01;

        #ifdef S3KM1110_DEBUG_DATA
        if (_uartDebug != nullptr)
		{
            _uartDebug->printf("Detected: %x | Distance: %u\n", detectionResultRaw, distanceToTarget);
            _uartDebug->print(F("Gate energy:\n"));

            for (uint8_t i = 0; i < S3KM1110_DISTANCE_GATE_COUNT; i++) 
			{
                _uartDebug->printf("%02u\t", i);
            }
            _uartDebug->print('\n');
        }
        #endif

		// convert data stream payload into array distanceGateEnergy
		
        uint8_t distanceGateStart = 9;
        for (uint8_t idx = 0; idx < S3KM1110_DISTANCE_GATE_COUNT; idx++) 
		{
            uint16_t energy = _radarDataFrame[distanceGateStart + idx] + (_radarDataFrame[distanceGateStart + idx + 1] << 8);
            distanceGateEnergy[idx] = energy;

            #ifdef S3KM1110_DEBUG_DATA
            if (_uartDebug != nullptr) 
			{
                _uartDebug->printf("%02u\t", distanceGateEnergy[idx]);
            }
            #endif
        }
		
        #ifdef S3KM1110_DEBUG_DATA
        if (_uartDebug != nullptr)
		{
            _uartDebug->print('\n');
        }
        #endif

        return true;
    }
	else 
    {
    	// bad frame length
        #ifdef S3KM1110_DEBUG_DATA
        if (_uartDebug != nullptr) {
            _uartDebug->print(F("\nFrame length unexpected: "));
            _uartDebug->print(_radarDataFramePosition);
            _uartDebug->print('\n');
        }
        #endif
    }

    #ifdef S3KM1110_DEBUG_DATA
    if (_uartDebug != nullptr) {
        _uartDebug->println(F("----------------------------------------"));
    }
    #endif
    return false;
}

//---------------------------------------------------------------------

bool s3km1110::_parseCommandFrame()
{
    uint8_t totalFrameSize = _radarDataFrame[4] + (_radarDataFrame[5] << 8);

    mlastRxCommand = _radarDataFrame[6];
    mbLastRxCmdValid = (_radarDataFrame[8] == 0x00 && _radarDataFrame[9] == 0x00);

    bool bRxPayloadHasSize = false;
	
    if (   mlastRxCommand == COMMAND_READ_SERIAL_NUMBER 
		|| mlastRxCommand == COMMAND_READ_FIRMWARE_VERSION) 
	{
        bRxPayloadHasSize = true;
    }

    uint8_t startPayloadPosition = bRxPayloadHasSize ? 12 : 10;
	
    int16_t payloadLength = _radarDataFramePosition - 4 - startPayloadPosition;

    char payloadRxBytes[payloadLength + 1] = {0};

	// take packet payload and copy into buffer.
    if (payloadLength > 0) 
	{
        for (uint8_t idx = 0; idx < payloadLength; idx++) 
		{
            payloadRxBytes[idx] = _radarDataFrame[startPayloadPosition + idx];
        }
    }

    #ifdef S3KM1110_DEBUG_COMMANDS
    if (_uartDebug != nullptr) 
	{
        //_uartDebug->println(F("-------------------------------------------"));
        _uartDebug->print(F("RCV ACK: "));
        _printCurrentFrame();
        _uartDebug->printf("CMD: 0x%02x | Status: %s | Body: %u | Payload: %u\n", 
				mlastRxCommand, 
				mbLastRxCmdValid ? "Ok" : "Failed", 
				totalFrameSize, 
				payloadLength);

        if (payloadLength > 0) 
		{
            _uartDebug->print(F("Raw payload: "));
            for (uint8_t idx = 0; idx < payloadLength; idx++)
			{
                _uartDebug->printf("%02x ", payloadRxBytes[idx]);
            }
            _uartDebug->print('\n');
        }
    }
    #endif

    bool isSuccess = false;
    if (mlastRxCommand == COMMAND_OPEN_COMMAND_MODE) 
	{
        return mbLastRxCmdValid;
    }
    else if (mlastRxCommand == COMMAND_CLOSE_COMMAND_MODE)
	{
        return mbLastRxCmdValid;
    }
    else if (mlastRxCommand == COMMAND_SET_MODE)
    {
        isSuccess = mbLastRxCmdValid;
    } 
    else if (mlastRxCommand == COMMAND_READ_SERIAL_NUMBER)
    {
        if (payloadLength > 0) 
		{
            serialNumber = new String(payloadRxBytes);
            isSuccess = true;
        }
    } 
    else if (mlastRxCommand == COMMAND_READ_FIRMWARE_VERSION)
    {
        if (payloadLength > 0) 
		{
            mFirmwareVersion = new String(payloadRxBytes);
            isSuccess = true;
        }
    }
    else if (mlastRxCommand == COMMAND_RADAR_SET_CONFIG) 
    {
        return mbLastRxCmdValid;
    }
    else if (mlastRxCommand == COMMAND_RADAR_READ_CONFIG)
    {
        return _parseGetConfigCommandFrame(payloadRxBytes, payloadLength);
    }
    else 
    {
        #ifdef S3KM1110_DEBUG_COMMANDS
        if (_uartDebug != nullptr) {
            _uartDebug->print("[ERROR] Receive Unknown Command\n");
        }
        #endif
    }

    #ifdef S3KM1110_DEBUG_COMMANDS
    if (_uartDebug != nullptr) {
        _uartDebug->println(F("---------------------------------------------"));
    }
    #endif
    return isSuccess;
}

//=====================================================================
#pragma mark * Parse command helpers

// parse repsonse for selected return result
// store response in global variables.

bool s3km1110::_parseGetConfigCommandFrame(char *payload, uint8_t count)
{
    if (count != 4) { return false; }
	
    uint32_t result =  payload[0] 
					+ (payload[1] << 8) 
					+ (payload[2] << 16) 
					+ (payload[3] << 24);

    if (commandRequested == CONFIG_DETECTION_DISTANCE_MIN) 
	{
        radarConfiguration->detectionGatesMin = result;
        TRACE("radarConfiguration->detectionGatesMin = %d\n", result);
    }
    else if (commandRequested == CONFIG_DETECTION_DISTANCE_MAX) 
	{
        radarConfiguration->detectionGatesMax = result;
        TRACE("radarConfiguration->detectionGatesMax = %d\n", result);
    }
    else if (commandRequested == CONFIG_TARGET_ACTIVE_FRAMES) 
	{
        TRACE("radarConfiguration->activeFrameNum = %d\n", result);
        radarConfiguration->activeFrameNum = result;
    }
    else if (commandRequested == CONFIG_TARGET_INACTIVE_FRAMES) 
	{
        TRACE("radarConfiguration->inactiveFrameNum = %d\n", result);
        radarConfiguration->inactiveFrameNum = result;
    }
    else if (commandRequested == CONFIG_DISAPPEARANCE_DELAY) 
	{
        TRACE("radarConfiguration->disappearDelay = %d\n", result);
        radarConfiguration->disappearDelay = result;
    }
	else
	{
		TRACE(" ************* UNHANDLED %d (0x%X)\n", 
			commandRequested,
			commandRequested);
		
        return false;
    }

    return true;
}

bool s3km1110::_setParameterConfiguration(uint16_t parameterCode, int value)
{
    return _sendCommandAndWait(COMMAND_RADAR_SET_CONFIG, parameterCode, 2, value, 4);
}

#pragma mark - Command mode

bool s3km1110::_openCommandMode()
{
    return _sendCommandAndWait(COMMAND_OPEN_COMMAND_MODE, 1, 2, true);
}

bool s3km1110::_closeCommandMode()
{
    return _sendCommandAndWait(COMMAND_CLOSE_COMMAND_MODE, 0, 0, true);
}

#pragma mark * Helpers

void s3km1110::_sendHexData(String rawData)
{
    #ifdef S3KM1110_DEBUG_COMMANDS
    if (_uartDebug != nullptr) {
        _uartDebug->println(F("-------------------------------------------"));
        _uartDebug->print(F("SND: "));
        _uartDebug->println(rawData);
    }
    #endif

    unsigned int count = rawData.length();
    byte bytes[count / 2];
    for (byte idx = 0; idx < count; idx += 2) {
        bytes[idx / 2] = strtoul(rawData.substring(idx, idx + 2).c_str(), NULL, HEX);
    }
    _uartRadar->write(bytes, sizeof(bytes));
}

bool s3km1110::_sendCommandAndWait(uint16_t command, uint32_t payload, uint8_t payloadSize, bool isSkipCommandMode)
{
    return _sendCommandAndWait(command, 0, 0, payload, payloadSize, isSkipCommandMode);
}

bool s3km1110::_sendCommandAndWait(
    uint16_t command, 
    uint32_t subCommand, 
    uint8_t subCommandSize, 
    uint32_t payload, 
    uint8_t payloadSize,
    bool isSkipCommandMode)
{
	TRACE( "sending command %s (0x%X)\n", cmd2String(command), command);
	
    String commandStr = _intToHex(command, S3KM1110_FRAME_COMMAND_SIZE);
    String subCommandStr = subCommandSize > 0 ? _intToHex(subCommand, subCommandSize) : "";
    String payloadStr = payloadSize > 0 ? _intToHex(payload, payloadSize) : "";
    String totalSizeStr = _intToHex(S3KM1110_FRAME_COMMAND_SIZE + subCommandSize + payloadSize, S3KM1110_FRAME_LENGTH_SIZE);
    
    if (isSkipCommandMode || _openCommandMode()) {
        delay(50);
        _sendHexData(_getCommandPrefix() + totalSizeStr + commandStr + subCommandStr + payloadStr + _getCommandPostfix());

        _radarUartLastCommandTime = millis();
        while (millis() - _radarUartLastCommandTime < kRadarUartcommandTimeout) {
            if (_read_frame()) {
                if (mbLastRxCmdValid && mlastRxCommand == command) {
                    delay(50);
                    if (!isSkipCommandMode) { _closeCommandMode(); }
                    return true;
                }
            }
        }
    }

    delay(50);
    if (!isSkipCommandMode) { _closeCommandMode(); }
	TRACE(" failed to get response to command 0x%X\n", command);
    return false;
}

String s3km1110::_intToHex(int value, uint8_t byteCount)
{
    uint8_t width = byteCount * 2;
    char result[width];
    uint32_t littleIndian = __htonl(value);
    snprintf(result, width + 1, "%08x", littleIndian);
    return String(result);
}

#endif //s3km1110_cpp
