#include "WebSocketHelper.h"
#include "DeviceControl.h"

void initWebSocketServer() {
  webSocketServer.begin();  // Begin WebSocket server
  webSocketServer.onEvent([](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    // Handle WebSocket events (e.g., new client connection, disconnection)
    switch (type) {
      case WStype_CONNECTED:
        Serial.printf("[%u] WebSocket client connected from IP %s\n", num, webSocketServer.remoteIP(num).toString().c_str());
        break;
      case WStype_DISCONNECTED:
        Serial.printf("[%u] WebSocket client disconnected\n", num);
        break;
      case WStype_TEXT:
        Serial.printf("[%u] Received text: %s\n", num, payload);
        // Handle text message received from WebSocket client
        break;
      case WStype_BIN:
        Serial.printf("[%u] Received binary data of length %u\n", num, length);
        // Handle binary data received from WebSocket client
        break;
      default:
        break;
    }
  });
}

void handleWebSocket() {
  webSocketServer.loop();
}

void publishSensorStatus() {
  String response = getDeviceStatus();
  webSocketServer.broadcastTXT(response);
}
