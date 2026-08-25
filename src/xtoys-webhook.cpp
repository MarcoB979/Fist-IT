// XToys webhook integration: connects over WiFi + websocket to relay commands
// into the same incoming/OnDataRecv pipeline used by the BLE control path.
#include "xtoys-webhook.h"

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#include "secrets.h"

static const char *XTOYS_HOST = "webhook.xtoys.app";
static const uint16_t XTOYS_PORT = 443;

static WebSocketsClient webSocket;
static bool g_xtoysSocketConnected = false;
static bool g_wifiWasConnected = false;
static unsigned long g_lastWifiRetryMs = 0;
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 5000UL;

static int mapKeyToCommand(const String &key, const String &rawValue, float value) {
  if (key == "speed")     return FSPEED;
  if (key == "depth")     return FROTATION;
  if (key == "stroke")    return FPAUSE;
  if (key == "sensation") return FSENSATION;
  if (key == "status")    return rawValue.equalsIgnoreCase("ON") ? ON : OFF;
  return -1;
}

static void handleXtoysMessage(const uint8_t *payload, size_t length) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) {
    return;
  }

  // Custom Toy protocol: {"id":"Fist-IT","<key>":"<value>"} - no fixed "action" key.
  for (JsonPairConst kv : doc.as<JsonObjectConst>()) {
    const char *key = kv.key().c_str();
    if (strcmp(key, "id") == 0) {
      continue;
    }
    float value = kv.value().as<float>();
    String rawValue = kv.value().as<String>();
    int command = mapKeyToCommand(String(key), rawValue, value);
    if (command < 0) {
      continue;
    }

    incoming.command = command;
    incoming.value = value;
    incoming.target = FIST_ID;
    incoming.sender = XTOYS_ID;
    Serial.printf("[XToys] Parsed key=%s command=%d value=%.2f\n", key, command, incoming.value);
    g_bleMessagePending = true;
    break;
  }
}

static void onXtoysWebSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      g_xtoysSocketConnected = true;
      Serial.println("[XToys] Websocket CONNECTED");
      break;
    case WStype_DISCONNECTED:
      g_xtoysSocketConnected = false;
      Serial.println("[XToys] Websocket DISCONNECTED");
      break;
    case WStype_ERROR:
      Serial.printf("[XToys] Websocket ERROR: %.*s\n", (int)length, (const char *)payload);
      break;
    case WStype_TEXT:
      Serial.printf("[XToys] Received TEXT: %.*s\n", (int)length, (const char *)payload);
      handleXtoysMessage(payload, length);
      break;
    case WStype_BIN:
      Serial.printf("[XToys] Received BIN, %u bytes\n", (unsigned)length);
      handleXtoysMessage(payload, length);
      break;
    case WStype_PING:
      Serial.println("[XToys] Received PING");
      break;
    case WStype_PONG:
      Serial.println("[XToys] Received PONG");
      break;
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      Serial.printf("[XToys] Received fragment type=%d len=%u\n", (int)type, (unsigned)length);
      break;
    default:
      Serial.printf("[XToys] Received unhandled event type=%d\n", (int)type);
      break;
  }
}

static void connectXtoysWebSocket() {
  String path = String("/") + XTOYS_WEBSOCKET_ID;
  webSocket.beginSSL(XTOYS_HOST, XTOYS_PORT, path);
  String authHeader = String("Authorization: Bearer ") + XTOYS_AUTH_TOKEN;
  webSocket.setExtraHeaders(authHeader.c_str());
  webSocket.onEvent(onXtoysWebSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void setupXtoysWebhook() {
  Serial.printf("[XToys] Connecting to WiFi SSID '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  connectXtoysWebSocket();
}

void xtoysWebhookLoop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (g_wifiWasConnected) {
      Serial.println("[XToys] WiFi lost connection");
      g_wifiWasConnected = false;
    }
    unsigned long now = millis();
    if (now - g_lastWifiRetryMs >= WIFI_RETRY_INTERVAL_MS) {
      g_lastWifiRetryMs = now;
      Serial.println("[XToys] Retrying WiFi connection...");
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    return;
  }

  if (!g_wifiWasConnected) {
    g_wifiWasConnected = true;
    Serial.print("[XToys] WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
  }
  webSocket.loop();
}

void xtoysSendMessage(const struct_message &msg) {
  if (!g_xtoysSocketConnected) {
    return;
  }

  // Custom Toy protocol: {"id":"Fist-IT","<key>":"<value>"}
  JsonDocument doc;
  doc["id"] = XTOYS_TOY_IDENTIFIER;
  doc["command"] = msg.command;
  doc["value"] = msg.value;
  doc["heartbeat"] = msg.heartbeat;

  String json;
  serializeJson(doc, json);
  webSocket.sendTXT(json);
}
