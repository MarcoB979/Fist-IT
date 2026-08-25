// Shared command/message definitions used by both main.cpp (BLE) and xtoys-webhook.cpp
#pragma once

#include <Arduino.h>

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
#define XTOYS_ID 77 // Sender ID used for commands received via the XToys webhook

#define OFF 10
#define ON  11

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

// Defined in main.cpp; shared so xtoys-webhook.cpp can deliver/observe commands.
extern struct_message outgoing;
extern struct_message incoming;
extern volatile bool g_bleMessagePending;
