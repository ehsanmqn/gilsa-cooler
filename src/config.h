#define EEPROM_SIZE 20
#define DEVICE_NAME "Gilsa Cooler"
#define statictoken true

// #define TOKEN "1cNEgsWvCOiJQnTX7fMC" // 4
char TOKEN[20] = {0};

// GPIO bindings
#define GPIO0 0
#define GPIO1 1
#define GPIO2 2
#define GPIO3 3
#define GPIO4 4
#define GPIO5 5
#define GPIO9 9
#define GPIO10 10
#define GPIO11 11
#define GPIO12 12
#define GPIO13 13
#define GPIO15 15
#define GPIO13 13
#define GPIO14 14
#define GPIO16 16

// Constants
#define LOOP_SLEEP_TIME 1
#define LONG_PRESS_ITERATIONS 5000
#define CONNECTION_BLINK_ITERATIONS 10000
#define CHECK_CONNETCTION_ITERATIONS 60000

// Input Constants
#define waterButtonSensor GPIO0
#define lowSpeedSensor GPIO3
#define highSpeedSensor GPIO1

// Output Constants
#define waterButtonRelay GPIO5
#define lowSpeedRelay GPIO14
#define highSpeedRelay GPIO12
#define linkLed GPIO13

// Serial configurations
#define serialEnabled false
#define serialBuadrate 115200

// Wifi configuration
#define resetWifiEnabled false
#define autoGenerateSSID false
#define configPortalTimeout 60

// Server Configuration
int mqttPort = 1883;
char dmiotServer[] = "platform.dmiot.ir";

// We assume that all GPIOs are LOW
bool gpioState[] = {false, false, false, false};

// Device token
// #ifdef statictoken
// #define notoken 1
// #else
// char TOKEN[20] = {0};
// #endif
