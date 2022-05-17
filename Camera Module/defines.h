// Define the number of bytes you want to access
#define EEPROM_SIZE 1
// Pin definition for CAMERA_MODEL_AI_THINKER
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

#define MICROMINUTECONST 60000000 // defines 1 minute in microseconds for scaling
#define DEEPSLEEPOFFSET 2500000 // defines an offset for setup time in deep sleep
#define YEAROFFSET 1900 // Year Offset

#define OBLED 33  // defines gpio of green led

void userSetup(BluetoothSerial SerialBT, int* cameraCaptureFreq, int* pictureNumber, struct tm tm);