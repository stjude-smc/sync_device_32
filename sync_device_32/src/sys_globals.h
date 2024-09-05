#define VERSION "0.6.0\n"


/*********************************
HELPFUL BIT MANIPULATION FUNCTIONS
*********************************/
#define bit(b)                         (1UL << (b))
#define bitRead(register, b)            (((register) >> (b)) & 0x01)
#define bitSet(register, b)             ((register) |= (1UL << (b)))
#define bitClear(register, b)           ((register) &= ~(1UL << (b)))
#define bitToggle(register, b)          ((register) ^= (1UL << (b)))
#define bitWrite(register, b, bitvalue) ((bitvalue) ? bitSet(register, b) : bitClear(register, b))


/****************************
PINOUT AND WIRING DEFINITIONS
****************************/
// Laser shutters
const uint32_t CY2_PIN = PIO_PA16_IDX; // aka Arduino pin A0
const uint32_t CY3_PIN = PIO_PA24_IDX; // aka Arduino pin A1
const uint32_t CY5_PIN = PIO_PA23_IDX; // aka Arduino pin A2
const uint32_t CY7_PIN = PIO_PA22_IDX; // aka Arduino pin A3
const uint32_t SHUTTERS_MASK = PIO_PA16 | PIO_PA24 | PIO_PA23 | PIO_PA22;
#define SHUTTERS_PORT IOPORT_PIOA

// Camera trigger
const uint32_t CAMERA_PIN = PIO_PB27_IDX; // aka Arduino pin 13
