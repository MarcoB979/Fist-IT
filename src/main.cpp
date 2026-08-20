//THISIS CONFIRMED A WORKING VERSION!! DO NOT DELETE



#include "FastAccelStepper.h"
#include <Arduino.h>          // Basic Needs
#include <WiFi.h>
#include <wifi.h>
#include <NimBLEDevice.h>
#if defined(ESP32)
#include <esp_log.h>
#endif
#include <ESPmDNS.h>
#include <WiFiManager.h> // WiFiManager library for automatic WiFi configuration

/////////////////////////////////OTA Part/////////////////////////////////////

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <ESP8266WebServer.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <WebServer.h>
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <WiFiServer.h>
  #include <WebServer.h>
#endif

#include <ElegantOTA.h>

// WiFiManager will handle SSID and password configuration automatically
// No need for hardcoded credentials

#if defined(ESP8266)
  ESP8266WebServer server(80);
#elif defined(ESP32)
  WebServer server(80);
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
  WebServer server(80);
#endif

unsigned long ota_progress_millis = 0;


int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

/////////////////////////////////FIST-IT part/////////////////////////////////////


//#define C3devboard
#define C3mini
//#define C3miniV2

#ifdef C3mini
    #define dirPinStepper 1
    #define enablePinStepper 0
    #define stepPinStepper 2
    #define LVpin 99
#endif

#ifdef C3devboard
    #define dirPinStepper 16 //3
    #define enablePinStepper 4 //1
    #define stepPinStepper 17 //2
    #define LVpin 23 //2
#endif


#ifdef C3miniV2
    #define dirPinStepper 1
    #define enablePinStepper 10
    #define stepPinStepper 0
    #define LVpin 99

#endif

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;
int pos=0;
int EndPosition=0;
int StartPosition=0;
int speedValue =1;
int stepsPerRotation = 3600;
int SensationValue = 100;
int F_Pause = 0;
int strokeSpeedSetting = 1;
int strokeAccelSetting = 100;
int START=1;
int STOP=0;
int IDLE=444;
int ANGLE_MODE=555;
int MotorStatus=IDLE;
String Direction="";

const int STROKE_SPEED_MIN_SETTING = 1;
const int STROKE_SPEED_MAX_SETTING = 100;
const int STROKE_ACCEL_MIN_SETTING = 0;
const int STROKE_ACCEL_MAX_SETTING = 100;

// Tune these values to shape the M5 stroke-mode behavior.
const int STROKE_SPEED_SLOWEST_SPS = 50;
const int STROKE_SPEED_FASTEST_SPS = 20000;
const int STROKE_ACCEL_MIN_VALUE = 10;
const int STROKE_ACCEL_MAX_VALUE = 25000;
// At 100% accel setting: finalAccel = speedSps * this multiplier.
// 1.0 means accel equals speed (roughly 1 second to reach target speed).
const float STROKE_ACCEL_SPEED_MULTIPLIER = 1.0f;

#define FSPEED  30
#define FROTATION   31
#define FPAUSE   32
#define FSENSATION  33
#define FANGLE  34
#define FROTATION_SCALE  35   // Set rotation amplification (1-4x)
#define FRESPONSIVENESS  36   // Set speed responsiveness: 1=Normal(0.7x), 2=Fast(2.5x), 3=Ultra(5x)
#define FVELOCITY  37   // Rotation velocity for dynamic speed control
#define FSTOP  38   // Safety stop command (1=STOP/SAFE, 0=GO/ACTIVE)

// OSSM STROKE ENGINE COMMANDS (from M5 Remote)
#define SPEED 1
#define DEPTH 2
#define STROKE 3
#define SENSATION 4
#define PATTERN 5
#define HEARTBEAT 99
#define CONNECT 88

#define OSSM_ID  1 //OSSM_ID Default can be changed with M5 Remote in the Future will be Saved in EPROOM
#define FIST_ID 3 //M5_ID Default can be changed with M5 Remote in the Future will be Saved in EPROOM
#define FIST_IT_GYRO_ID 4 //Fist-IT Gyro Controller ID
#define M5_ID 99 //M5_ID Default can be changed with M5 Remote in the Future will be Saved in EPROOM

#define OFF 10
#define ON  11

#define CONNECT 88
#define HEARTBEAT 99

///////////////////////////////////////////
////
////  To Debug or not to Debug
////
///////////////////////////////////////////

// Uncomment the following line if you wish to print DEBUG info

#define DEBUG 

#ifdef DEBUG
#define LogDebug(...) Serial.println(__VA_ARGS__)
#define LogDebugFormatted(...) Serial.printf(__VA_ARGS__)
#else
#define LogDebug(...) ((void)0)
#define LogDebugFormatted(...) ((void)0)
#endif

// Uncomment the following line if you wish to print DEBUG info
#define DEBUGPRIO 

#ifdef DEBUGPRIO
#define LogDebugPRIO(...) Serial.println(__VA_ARGS__)
#define LogDebugFormattedPRIO(...) Serial.printf(__VA_ARGS__)
#else
#define LogDebugPRIO(...) ((void)0)
#define LogDebugFormattedPRIO(...) ((void)0)
#endif



// Variable to store if sending data was successful
String success;

float out_speed;
float out_depth;
float out_stroke;
float out_sensation;
float out_pattern;
bool out_rstate;
bool out_connected;
int out_command;
float out_value;
int out_target;
int out_sender;

float incoming_speed;
float incoming_depth;
float incoming_stroke;
float incoming_sensation;
float incoming_pattern;
bool incoming_rstate;
bool incoming_connected;
bool incoming_heartbeat;
int incoming_target;
int incoming_sender;

typedef struct struct_message {
  float speed;
  float depth;
  float stroke;
  float sensation;
  float pattern;
  bool rstate;
  bool connected;
  bool heartbeat;
  int command;
  float value;
  int target;
  int sender;
} struct_message;

bool M5_paired = false;
bool FIST_IT_RC_paired = false;
bool OSSM_paired = false;  // OSSM stroke engine pairing status

// User-configurable settings
float rotationScale = 1.0f;        // Scale factor: 0.75x to 1.5x (default 1x)
int responsivenessLevel = 4;     // 1x to 4x speed (default 4x)
float currentRotationVelocity = 0.0f; // Current rotation velocity in degrees/second

// SAFETY SYSTEM VARIABLES
bool gyroSafetyEngaged = true;   // Gyro safety status (true = safe/motor disabled)
int activeController = 0;       // Track which controller is currently active (0=none, M5_ID, FIST_IT_GYRO_ID)
unsigned long lastM5CommandTime = 0;
unsigned long lastGyroCommandTime = 0;
const unsigned long DEVICE_TIMEOUT = 2000; // 2 seconds timeout for device priority

struct_message outgoing;
struct_message incoming;

#define HEARTBEAT_INTERVAL_MS 5000UL

static const char *FIST_BLE_DEVICE_NAME = "Fist-IT";
static const char *FIST_BLE_SERVICE_UUID = "5f8bb6f0-9f17-4aa8-9c42-3d8b8b4d9001";
static const char *FIST_BLE_RX_UUID = "5f8bb6f1-9f17-4aa8-9c42-3d8b8b4d9001";
static const char *FIST_BLE_TX_UUID = "5f8bb6f2-9f17-4aa8-9c42-3d8b8b4d9001";

NimBLEServer *g_bleServer = nullptr;
NimBLECharacteristic *g_bleRxChar = nullptr;
NimBLECharacteristic *g_bleTxChar = nullptr;
volatile bool g_bleClientConnected = false;
volatile bool g_bleMessagePending = false;
unsigned long g_lastHeartbeatMs = 0;



/////////////////////////////////OTA Callbacks/////////////////////////////////////

void onOTAStart() {
  // Log when OTA has started
  LogDebugPRIO("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    LogDebugFormattedPRIO("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    LogDebugPRIO("OTA update finished successfully!");
  } else {
    LogDebugPRIO("There was an error during OTA update!");
  }
  // <Add your own code here>
}

/////////////////////////////////OTA Callbacks/////////////////////////////////////

static bool sendBleMessage(const struct_message &msg) {
  if (!g_bleClientConnected || g_bleTxChar == nullptr) {
    LogDebugFormatted("BLE TX SKIP cmd=%d val=%.3f target=%d sender=%d connected=%d heartbeat=%d\\n",
                      msg.command,
                      msg.value,
                      msg.target,
                      msg.sender,
                      msg.connected ? 1 : 0,
                      msg.heartbeat ? 1 : 0);
    return false;
  }
  LogDebugFormatted("BLE TX cmd=%d val=%.3f target=%d sender=%d connected=%d heartbeat=%d\\n",
                    msg.command,
                    msg.value,
                    msg.target,
                    msg.sender,
                    msg.connected ? 1 : 0,
                    msg.heartbeat ? 1 : 0);
  g_bleTxChar->setValue((uint8_t *)&msg, sizeof(msg));
  g_bleTxChar->notify();
  return true;
}

static int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

static int mapStrokeSpeedSpsFromSetting(int setting) {
  const int clampedSetting = clampInt(setting, STROKE_SPEED_MIN_SETTING, STROKE_SPEED_MAX_SETTING);
  const int inputSpan = STROKE_SPEED_MAX_SETTING - STROKE_SPEED_MIN_SETTING;
  if (inputSpan <= 0) {
    return STROKE_SPEED_SLOWEST_SPS;
  }

  const long outputSpan = (long)STROKE_SPEED_FASTEST_SPS - (long)STROKE_SPEED_SLOWEST_SPS;
  const long offset = (long)(clampedSetting - STROKE_SPEED_MIN_SETTING) * outputSpan / inputSpan;
  return (int)((long)STROKE_SPEED_SLOWEST_SPS + offset);
}

// FastAccelStepper expects speed in microseconds-per-step, so we convert at the final API boundary.
static int convertStrokeSpeedSpsToTickUs(int speedSps) {
  if (speedSps <= 0) {
    return 20000;
  }
  int speedUs = (int)((1000000.0f / (float)speedSps) + 0.5f);
  if (speedUs < 1) speedUs = 1;
  return speedUs;
}

static int calculateStrokeAccelFromSpeed(int accelSetting, int speedSps) {
  const int clampedSetting = clampInt(accelSetting, STROKE_ACCEL_MIN_SETTING, STROKE_ACCEL_MAX_SETTING);
  const int minSpeedSps = (STROKE_SPEED_SLOWEST_SPS < STROKE_SPEED_FASTEST_SPS) ? STROKE_SPEED_SLOWEST_SPS : STROKE_SPEED_FASTEST_SPS;
  const int maxSpeedSps = (STROKE_SPEED_SLOWEST_SPS < STROKE_SPEED_FASTEST_SPS) ? STROKE_SPEED_FASTEST_SPS : STROKE_SPEED_SLOWEST_SPS;
  const int safeSpeedSps = clampInt(speedSps, minSpeedSps, maxSpeedSps);
  const float accelPercent = (float)clampedSetting / 100.0f;
  const float accelFromSpeed = ((float)safeSpeedSps) * accelPercent * STROKE_ACCEL_SPEED_MULTIPLIER;
  int finalAccel = (int)(accelFromSpeed + 0.5f);
  finalAccel = clampInt(finalAccel, STROKE_ACCEL_MIN_VALUE, STROKE_ACCEL_MAX_VALUE);
  return finalAccel;
}

class FistBleServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
    (void)pServer;
    (void)connInfo;
    g_bleClientConnected = true;
    LogDebugPRIO("BLE client connected");
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override {
    (void)pServer;
    (void)connInfo;
    (void)reason;
    g_bleClientConnected = false;
    M5_paired = false;
    NimBLEDevice::startAdvertising();
    LogDebugPRIO("BLE client disconnected");
  }
};

class FistBleRxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override {
    (void)connInfo;
    std::string value = pCharacteristic->getValue();
    if (value.size() != sizeof(struct_message)) {
      LogDebugFormatted("BLE RX invalid size: %d (expected %d)\n", (int)value.size(), (int)sizeof(struct_message));
      return;
    }

    memcpy(&incoming, value.data(), sizeof(incoming));
    g_bleMessagePending = true;
  }
};

// Callback when data is received over BLE
void OnDataRecv() {
  LogDebugFormatted("BLE RX Command: %d, value: %.2f, target: %d, sender: %d\n",
                    incoming.command,
                    incoming.value,
                    incoming.target,
                    incoming.sender);

  if(incoming.target != FIST_ID && incoming.target != 0) {
    LogDebugFormatted("Received BLE data not intended for this device (target ID: %d), ignoring.\n", incoming.target);
    return;
  }

  if (incoming.sender == M5_ID && !M5_paired) {
    M5_paired = true;
    LogDebugPRIO("M5 Remote Connected over BLE");
  } else if (incoming.sender == FIST_IT_GYRO_ID && !FIST_IT_RC_paired) {
    FIST_IT_RC_paired = true;
    LogDebugPRIO("Fist-IT RC Connected over BLE");
  }
    
  // SMART PEER MANAGEMENT: Only add peer if not already paired
  if(incoming.target == FIST_ID) {

    // Process commands from any paired device
    LogDebugFormatted("Received command FOR FIST-IT from paired device: %d, value %.2f\n", incoming.command, incoming.value);
    
    // ALWAYS ALLOW CONNECTION COMMANDS - they establish communication
    if (incoming.command == CONNECT) {
      LogDebugFormatted("CONNECT command from device %d\n", incoming.sender);
      
      // Send immediate response to establish connection
      outgoing.command = HEARTBEAT; 
      outgoing.value = 1.0f;
      outgoing.target = incoming.sender;
      outgoing.sender = FIST_ID;
      outgoing.connected = true;
      sendBleMessage(outgoing);
      LogDebugFormatted("CONNECT response sent to device %d\n", incoming.sender);
      return; // Don't process as other commands
    }
    
    // ALWAYS ALLOW SAFETY STATUS UPDATES - they bypass all safety checks
    if (incoming.sender == FIST_IT_GYRO_ID && incoming.command == FSTOP) {
      gyroSafetyEngaged = (incoming.value > 0.5f); // 1=SAFE, 0=ACTIVE
      LogDebugFormatted("SAFETY: Gyro safety %s\n", gyroSafetyEngaged ? "ENGAGED" : "RELEASED");
      
      // Send acknowledgment back to gyro to confirm connection
      outgoing.command = FSTOP;
      outgoing.value = gyroSafetyEngaged ? 1.0f : 0.0f;
      outgoing.target = FIST_IT_GYRO_ID;
      outgoing.sender = FIST_ID;
      sendBleMessage(outgoing);
      return; // Don't process as movement command
    }
    
    // ALWAYS SEND ACKNOWLEDGMENTS FOR CONNECTION HEARTBEATS
    if (incoming.heartbeat) {
      LogDebugFormatted("HEARTBEAT received from device %d. replying with heartbeat\n", incoming.sender);
      // Send heartbeat response to confirm connection is alive
      outgoing.heartbeat = true;
      outgoing.connected = true;
      outgoing.target = incoming.sender;
      outgoing.sender = FIST_ID;
      sendBleMessage(outgoing);
      LogDebugFormattedPRIO("Heartbeat response sent to device %d\n", incoming.sender);
    }
    
    // SAFETY SYSTEM: Check if movement commands should be processed
    bool allowMovementCommand = true;
    unsigned long currentTime = millis();
    
    if (incoming.sender == M5_ID) {
      // M5 Remote always has movement priority
      lastM5CommandTime = currentTime;
      activeController = M5_ID;
      LogDebug("SAFETY: M5 Remote active - has priority control");
      
      // Check if M5 is sending a stop command (zero velocity)
      bool isStopCommand = (abs(incoming.value) < 0.01f);
      if (isStopCommand) {
        // Stop command - clear M5 activity to allow gyro control later
        lastM5CommandTime = 0;
        LogDebug("SAFETY: M5 Remote STOP - clearing priority for gyro connection");
      }
    } 
    else if (incoming.sender == FIST_IT_GYRO_ID) {
      lastGyroCommandTime = currentTime;
      
      // Check if gyro movement should be blocked by safety system
      bool m5RecentlyActive = (currentTime - lastM5CommandTime) < DEVICE_TIMEOUT;
      
      if (gyroSafetyEngaged) {
        LogDebug("SAFETY: Gyro movement blocked - Safety is engaged");
        allowMovementCommand = false;
      }
      else if (m5RecentlyActive) {
        LogDebug("SAFETY: Gyro movement blocked - M5 Remote has priority");
        allowMovementCommand = false;
      }
      else {
        // Allow gyro control when safety is released AND M5 is inactive
        activeController = FIST_IT_GYRO_ID;
        LogDebug("SAFETY: Gyro movement allowed - M5 inactive and safety released");
      }
    }
    
    // Process movement commands only if safety allows
    if (!allowMovementCommand) {
      LogDebug("Movement command blocked by safety system");
      return;
    }

    switch(incoming.command)
    {
      case FSPEED:
      {
        strokeSpeedSetting = clampInt((int)incoming.value, STROKE_SPEED_MIN_SETTING, STROKE_SPEED_MAX_SETTING);
        speedValue = strokeSpeedSetting;
        LogDebug("Speed Value set to: "); 
        LogDebug(speedValue);

      }
      break;
      case FROTATION:
      {
        StartPosition = 0 ;
        EndPosition = (stepsPerRotation/360)*incoming.value ;  //degrees 0 to xx
      }
      break;
      case FPAUSE:
      {
        F_Pause = incoming.value;  //10th of a second
      }
      break;
      case FSENSATION:
      {
        strokeAccelSetting = clampInt((int)incoming.value, STROKE_ACCEL_MIN_SETTING, STROKE_ACCEL_MAX_SETTING);
        SensationValue = strokeAccelSetting;
        LogDebug("Sensation Value set to: "); 
        LogDebug(SensationValue);

      }
      break;
      case FANGLE:
      {
        // SAFETY: Constrain angle to safe range (-90° to +90° for human safety)
        float safeAngle = incoming.value;
        if (safeAngle > 90.0) safeAngle = 90.0;
        if (safeAngle < -90.0) safeAngle = -90.0;
        
        // DIRECTION FIX: Invert angle so left gyro movement = left motor movement
        safeAngle = -safeAngle;
        
        // USER CONFIGURABLE AMPLIFICATION: Apply rotation scale (0.75x to 1.5x)
        safeAngle = safeAngle * rotationScale;
        
        // Convert to steps with safety check - use 180° for gyro range mapping
        int targetAngleSteps = (stepsPerRotation/180) * safeAngle;  // Use 180° to match gyro ±90° range
        
        // CRITICAL SAFETY LOGGING: Track all motor movements
        // EFFICIENT PROCESSING: Minimal debug for maximum responsiveness
        int currentMotorPosition = stepper->getCurrentPosition();
        int movementDelta = targetAngleSteps - currentMotorPosition;
        float movementDegrees = (float)movementDelta / (stepsPerRotation/180);
        
        //Serial.print("GYRO: "); //Serial.print(incoming.value, 1); 
        //Serial.print("° → MOTOR: "); //Serial.print(safeAngle, 1);
        //Serial.print("° ("); //Serial.print(targetAngleSteps); LogDebug(" steps)");
        
        // SAFETY: Detect dangerous movements (>360°)
        if (abs(movementDegrees) > 360.0f) {
            LogDebug("!!! DANGER: Movement >360° detected !!!");
            //Serial.print("!!! BLOCKING: Would move "); //Serial.print(movementDegrees, 1); LogDebug("° !!!");
            return; // ABORT dangerous movement
        }
        
        // SAFETY: Only prevent truly dangerous movements, don't artificially limit range
        // Allow full gyro range without clamping for smooth movement
        
        // REMOVED ABSOLUTE POSITION LIMIT: Allow full range of movement
        // Previous ±180° limit was causing sudden stops during continuous operation
        // Gyro input is already safely limited to ±90°, so cumulative position tracking is unnecessary
        // User should have full control within the safe gyro input range
        
        // INCREASED BASE SPEED: Higher baseline for better responsiveness across all levels
        // For FastAccelStepper: setSpeedInTicks uses microseconds per step (lower = faster)
        // NEMA23 + TB6600 faster baseline: 6,667 steps/sec = 150µs/step
        int baseSpeed = 150;  // Base: 150 µs/step = 6,667 steps/sec (faster baseline)
        
        // SIMPLIFIED 2-LEVEL RESPONSIVENESS: Speed stays unleashed, acceleration differs
        float responsivenessMultiplier = 1.5f; // Keep speed high for both levels
        bool slowAcceleration = false;
        switch (responsivenessLevel) {
            case 1: 
                slowAcceleration = true;    // Slow - gentler acceleration/deceleration
                break;
            case 2:
            case 3: 
            case 4:
            default: 
                slowAcceleration = false;   // Normal - current fast acceleration
                break;
        }
        
        // EXTREME VELOCITY-PROPORTIONAL SPEED: Much more dramatic differences
        float velocityMagnitude = abs(currentRotationVelocity);
        float velocitySpeedMultiplier = 1.0f;
        
        // IMPROVED velocity mapping: Better base speeds for normal use
        if (velocityMagnitude <= 1.0f) {
            // Very slow rotation: 0.3x - 0.5x speed (still responsive for precision)
            velocitySpeedMultiplier = 0.3f + (velocityMagnitude / 1.0f) * 0.2f; // 0.3x to 0.5x
        } else if (velocityMagnitude <= 5.0f) {
            // Slow rotation: 0.5x - 1.0x speed (good baseline range)
            velocitySpeedMultiplier = 0.5f + ((velocityMagnitude - 1.0f) / 4.0f) * 0.5f; // 0.5x to 1.0x
        } else if (velocityMagnitude <= 15.0f) {
            // Medium rotation: 1.0x - 2.0x speed (noticeably faster)
            velocitySpeedMultiplier = 1.0f + ((velocityMagnitude - 5.0f) / 10.0f) * 1.0f; // 1.0x to 2.0x
        } else if (velocityMagnitude <= 30.0f) {
            // Fast rotation: 2.0x - 4.0x speed (clearly fast)
            velocitySpeedMultiplier = 2.0f + ((velocityMagnitude - 15.0f) / 15.0f) * 2.0f; // 2.0x to 4.0x
        } else {
            // Very fast rotation: 4.0x - 6.0x speed (EXTREME speed)
            velocitySpeedMultiplier = 4.0f + min((velocityMagnitude - 30.0f) / 20.0f, 1.0f) * 2.0f; // 4.0x to 6.0x max
        }
        
        // 2-LEVEL VELOCITY-BASED ACCELERATION: Different responses for Slow vs Normal
        float velocityAccelAdjustment = 1.0f;
        
        if (slowAcceleration) {
            // SLOW RESPONSIVENESS: Old conservative velocity-based acceleration
            if (velocityMagnitude <= 2.0f) {
                velocityAccelAdjustment = 1.5f; // Old conservative for slow precise movements
            } else if (velocityMagnitude > 15.0f) {
                velocityAccelAdjustment = 0.4f; // Old conservative reduction for fast movements
            } else if (velocityMagnitude > 5.0f) {
                velocityAccelAdjustment = 0.7f; // Old conservative for medium movements
            }
        } else {
            // NORMAL RESPONSIVENESS: Current unleashed velocity-based acceleration
            if (velocityMagnitude <= 2.0f) {
                velocityAccelAdjustment = 1.8f; // Unleashed - higher acceleration for slow precise movements
            } else if (velocityMagnitude > 15.0f) {
                velocityAccelAdjustment = 0.7f; // Unleashed - less aggressive reduction for fast movements
            } else if (velocityMagnitude > 5.0f) {
                velocityAccelAdjustment = 1.0f; // Unleashed - better acceleration for medium movements
            }
        }
        
        float totalSpeedMultiplier = responsivenessMultiplier * velocitySpeedMultiplier;
        
        // Calculate final speed: Division correct for µs/step (lower = faster)
        int finalSpeed = (int)(baseSpeed / totalSpeedMultiplier); // Divide because lower µs/step = faster
        if (finalSpeed < 20) finalSpeed = 20; // UNLEASHED: 20 µs/step = 50,000 steps/sec (much higher limit!)
        
        // UNLEASHED: Convert µs/step to actual steps/sec for acceleration calculation
        float actualSpeedStepsPerSec = 1000000.0f / finalSpeed; 
        
        // 2-LEVEL ACCELERATION SYSTEM: Different responsiveness through acceleration control
        float baseAccelerationPercentage;
        
        if (slowAcceleration) {
            // SLOW RESPONSIVENESS: Use old conservative acceleration (gentler feel)
            if (actualSpeedStepsPerSec <= 4000) {
                baseAccelerationPercentage = 0.85f; // Old working Slow setting
            } else if (actualSpeedStepsPerSec <= 7000) {
                baseAccelerationPercentage = 0.65f; // Old medium speeds
            } else if (actualSpeedStepsPerSec <= 10000) {
                baseAccelerationPercentage = 0.45f; // Old fast speeds  
            } else {
                baseAccelerationPercentage = 0.30f; // Old ultra-fast speeds
            }
        } else {
            // NORMAL RESPONSIVENESS: Use current unleashed acceleration (aggressive feel)
            if (actualSpeedStepsPerSec <= 4000) {
                baseAccelerationPercentage = 1.2f; // Unleashed - more aggressive for slow speeds
            } else if (actualSpeedStepsPerSec <= 10000) {
                baseAccelerationPercentage = 0.9f; // Unleashed - better responsiveness for medium speeds
            } else if (actualSpeedStepsPerSec <= 20000) {
                baseAccelerationPercentage = 0.7f; // Unleashed - good balance for fast speeds  
            } else {
                baseAccelerationPercentage = 0.5f; // Unleashed - still responsive for ultra-fast speeds
            }
        }
        
        // Apply velocity-based acceleration adjustment
        float finalAccelerationPercentage = baseAccelerationPercentage * velocityAccelAdjustment;
        int finalAccel = (int)(actualSpeedStepsPerSec * finalAccelerationPercentage);
        
        // Safety limits
        if (finalAccel < 50) finalAccel = 50;    // Minimum for basic responsiveness
        if (finalAccel > 8000) finalAccel = 8000; // Lower maximum to prevent jitter
        // CONTINUOUS VELOCITY DEBUG: Always show velocity info for calibration
        static unsigned long lastDebugTime = 0;
        if (millis() - lastDebugTime > 200) { // Debug 5 times per second for real-time feedback
            String responseName = slowAcceleration ? "Slow" : "Normal";
            String accelType = slowAcceleration ? "Conservative" : "Unleashed";
            float actualSpeed = 1000000.0f / finalSpeed;
            LogDebugFormatted("2-LEVEL: Gyro=%.2f°/s → Speed×%.2fx (%.0f sps) | Accel: %s [%s]\n", 
                         velocityMagnitude, velocitySpeedMultiplier, actualSpeed, accelType.c_str(), responseName.c_str());
            lastDebugTime = millis();
        }
        
        // VERY CONSERVATIVE: Only change speed/acceleration when significantly different
        // This prevents constant micro-adjustments that cause jitter at 200Hz
        static int lastFinalSpeed = -1;
        static int lastFinalAccel = -1;
        
        if (abs(finalSpeed - lastFinalSpeed) > 20) { // Increased from 5 to 20
            stepper->setSpeedInTicks(finalSpeed);
            lastFinalSpeed = finalSpeed;
        }
        
        if (abs(finalAccel - lastFinalAccel) > 500) { // Decreased from 1000 to 500 but more stable
            stepper->setAcceleration(finalAccel);
            lastFinalAccel = finalAccel;
        }
        
        // SIMPLE DIRECT MOVEMENT: Back to what was working better
        // No filtering, no thresholds - just direct smooth updates
        stepper->moveTo(targetAngleSteps);
        
        MotorStatus = ANGLE_MODE; // Set to angle positioning mode
      }
      break;
      case FROTATION_SCALE:
      {
        // Set rotation scale factor (0.75x to 1.5x)
        float newScale = incoming.value;
        if (newScale >= 0.5f && newScale <= 2.0f) { // Allow reasonable range
          rotationScale = newScale; // Now store as float value instead of integer index
          LogDebug("Rotation Scale set to: ");
          LogDebug(rotationScale);
        }
      }
      break;
      case FRESPONSIVENESS:
      {
        // Set speed responsiveness: 1=Slow(0.7x), 2=Normal(1.0x), 3=Fast(1.5x), 4=Ultra(2.5x)
        int newResponsiveness = (int)incoming.value;
        if (newResponsiveness >= 1 && newResponsiveness <= 4) {
          responsivenessLevel = newResponsiveness;
          String responseName = (responsivenessLevel == 1) ? "Slow" : (responsivenessLevel == 2) ? "Normal" : (responsivenessLevel == 3) ? "Fast" : "Ultra";
          LogDebug("Responsiveness set to: ");
          LogDebug(responseName.c_str());
        }
      }
      break;
      case FVELOCITY:
      {
        // Store rotation velocity for dynamic speed control
        float incomingVelocity = incoming.value;
        
        // ANTI-DRIFT DEADZONE: Completely ignore very small velocities to prevent accumulating drift
        const float VELOCITY_DEADZONE = 0.75f; // Ignore velocities below 0.75°/s
        
        if (abs(incomingVelocity) < VELOCITY_DEADZONE) {
            currentRotationVelocity = 0.0f; // Force to zero to prevent drift
            LogDebug("VELOCITY DEADZONE: Ignored ");
            LogDebug(incomingVelocity);
            LogDebug("°/s (below ");
            LogDebug(VELOCITY_DEADZONE);
            LogDebug("°/s threshold)");
        } else {
            currentRotationVelocity = incomingVelocity;
            LogDebug("Velocity: ");
            LogDebug(currentRotationVelocity);
            LogDebug("°/s");
        }
      }
      break;
      case ON:
      {
        LogDebug("MOTOR STARTED");
        LogDebug("Speed Value: ");
        LogDebug(speedValue);   
        LogDebug("Sensation Value: ");
        LogDebug(SensationValue); 
        MotorStatus=START;
      }
      break;
      case OFF:
      {
        MotorStatus=STOP;
      }
      break;
      case CONNECT:
      case HEARTBEAT:
      {
        // Handle connection from M5 Remote
        if (incoming.sender == M5_ID) {
          outgoing.target = M5_ID;
          outgoing.sender = FIST_ID;
          outgoing.command = HEARTBEAT;
          bool result = sendBleMessage(outgoing);
          LogDebug(result ? 1 : 0);
          
          if (result) {
            M5_paired = true;
            LogDebug("M5 remote Connected");
          }
        }
        // Handle connection from Fist-IT Remote Controller
        else if (incoming.sender == FIST_IT_GYRO_ID) {
          outgoing.target = FIST_IT_GYRO_ID;
          outgoing.sender = FIST_ID;
          outgoing.command = HEARTBEAT;

          bool result = sendBleMessage(outgoing);
          LogDebug("Responding to Fist-IT RC connection");
          LogDebug(result ? 1 : 0);
          
          if (result) {
            FIST_IT_RC_paired = true;
            LogDebug("Fist-IT Remote Controller Connected via CONNECT");
          }
        }
      }
      break;
    }
    
    // Send appropriate response directly to the sender (no peer switching needed for responses)
    outgoing.sender = FIST_ID;
    outgoing.command = CONNECT;
    outgoing.value = 1121212123123;
    
    if (incoming.sender == M5_ID) {
      outgoing.target = M5_ID;
    } else if (incoming.sender == FIST_IT_GYRO_ID) {
      outgoing.target = FIST_IT_GYRO_ID;
    }
    
    // Send response directly to connected BLE client
    sendBleMessage(outgoing);

  } // Close if(incoming.target == FIST_ID)
} // Close OnDataRecv function


void setupBLEComm() {
  NimBLEDevice::init(FIST_BLE_DEVICE_NAME);
  g_bleServer = NimBLEDevice::createServer();
  g_bleServer->setCallbacks(new FistBleServerCallbacks());

  NimBLEService *service = g_bleServer->createService(FIST_BLE_SERVICE_UUID);
  g_bleRxChar = service->createCharacteristic(
      FIST_BLE_RX_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  g_bleTxChar = service->createCharacteristic(
      FIST_BLE_TX_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  g_bleRxChar->setCallbacks(new FistBleRxCallbacks());

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(FIST_BLE_SERVICE_UUID);
  advertising->enableScanResponse(true);
  advertising->start();

  LogDebugPRIO("BLE advertising started for Fist-IT");
}


void setup(void) {
  Serial.begin(115200);
  delay(1000);

#if defined(ESP32)
  // Silence verbose NimBLE debug lines like "D NimBLECharacteristic" in serial monitor.
  esp_log_level_set("NimBLECharacteristic", ESP_LOG_WARN);
  esp_log_level_set("NimBLEServer", ESP_LOG_WARN);
  esp_log_level_set("NimBLEClient", ESP_LOG_WARN);
  esp_log_level_set("NimBLEScan", ESP_LOG_WARN);
#endif

  // Initialize BLE transport for M5 Remote communication.
  setupBLEComm();

  
  pinMode(enablePinStepper, OUTPUT);
  digitalWrite(enablePinStepper, HIGH);
  pinMode(LVpin, OUTPUT);
  digitalWrite(LVpin, HIGH);
    
  
  engine.init();
  stepper = engine.stepperConnectToPin(stepPinStepper);
  if (stepper) {
    stepper->setDirectionPin(dirPinStepper);
    stepper->setEnablePin(enablePinStepper);
    stepper->setAutoEnable(true);
    stepper->setDelayToDisable(5000);
    int MaxSpeed1 = stepper->getMaxSpeedInHz();
    LogDebug("Max Speed in HZ: "); 
    LogDebug(MaxSpeed1); 
    int MaxSpeed2 = stepper->getMaxSpeedInMilliHz();
    LogDebug("Max Speed in MiliHZ: "); 
    LogDebug(MaxSpeed2); 
    int MaxSpeed3 = stepper->getMaxSpeedInTicks();
    LogDebug("Max Speed in Ticks: "); 
    LogDebug(MaxSpeed3); 
    int MaxSpeed4 = stepper->getMaxSpeedInUs();
    LogDebug("Max Speed in US: "); 
    LogDebug(MaxSpeed4); 
    int MaxSpeed=10000;

  }

    //test to show board works 
    stepper->setSpeedInTicks(5000);
    stepper->setAcceleration(20000);
    stepper->setCurrentPosition(500);
    stepper->moveTo(0);
  delay(200);
}

void pollBLEComm() {
  if (g_bleMessagePending) {
    g_bleMessagePending = false;
    OnDataRecv();
  }

  if (!g_bleClientConnected) {
    return;
  }

  unsigned long now = millis();
  if (now - g_lastHeartbeatMs < HEARTBEAT_INTERVAL_MS) {
    return;
  }
  g_lastHeartbeatMs = now;

  outgoing.command = HEARTBEAT;
  outgoing.sender = FIST_ID;
  outgoing.heartbeat = true;
  outgoing.target = M5_ID;
  sendBleMessage(outgoing);
}

void loop(void) {

  pollBLEComm();


  if (MotorStatus==START){
//    stepper->setSpeedInTicks(900);
//    stepper->setAcceleration(700);

int commandedSpeed = speedValue;
int commandedAccel = SensationValue;

int finalSpeedSps = mapStrokeSpeedSpsFromSetting(commandedSpeed);
// FastAccelStepper uses microseconds per step, so lower values mean faster motion.
int finalSpeedTickUs = convertStrokeSpeedSpsToTickUs(finalSpeedSps);
int strokeDistanceSteps = abs(EndPosition - StartPosition);
int finalAccel = calculateStrokeAccelFromSpeed(commandedAccel, finalSpeedSps);

stepper->setSpeedInTicks(finalSpeedTickUs);
stepper->setAcceleration(finalAccel);
LogDebugFormatted("Motor Speed: %d sps (tick_us=%d), Accel: %d steps/s^2, SpeedSetting: %d, AccelSetting: %d, StrokeSteps: %d\n",
                  finalSpeedSps,
                  finalSpeedTickUs,
                  finalAccel,
                  strokeSpeedSetting,
                  strokeAccelSetting,
                  strokeDistanceSteps);
if (stepper->targetPos()==stepper->getCurrentPosition()) {
  //delay((F_Pause*100)+1); //pause in ms
  if (stepper->getCurrentPosition()==StartPosition) {
      stepper->moveTo(EndPosition);
      }else      
      if (stepper->getCurrentPosition()==EndPosition) {
        stepper->moveTo(StartPosition);
      }else      
      if (stepper->getCurrentPosition()<EndPosition) {
        stepper->moveTo(EndPosition);
      }else      
        stepper->moveTo(EndPosition);
    }
  }  
/*    
    if (stepper->targetPos()==stepper->getCurrentPosition()) {
    if (stepper->getCurrentPosition()==0) {
      stepper->moveTo(1000);
      }else      
      if (stepper->getCurrentPosition()==1000) {
        stepper->moveTo(0);
      }else      
      if (stepper->getCurrentPosition()<1000) {
        stepper->moveTo(1000);
      }else      
        stepper->moveTo(1000);

    } 
  }*/

  else if (MotorStatus==ANGLE_MODE){
    // In angle mode, just hold the current position
    // The stepper library automatically holds position once target is reached
    // No continuous movement needed - wait for next FANGLE command
  }

else if (MotorStatus==STOP){
  stepper->forceStop();
  LogDebug("MOTOR STOPPED");
  MotorStatus=IDLE;
  }

 /////////////////////////////////OTA code/////////////////////////////////////

//  server.handleClient();
//  ElegantOTA.loop();


}//end of loop


