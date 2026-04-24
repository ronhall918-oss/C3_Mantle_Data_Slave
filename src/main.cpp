// seeedC3_Slave_rcvr V2
// Overview
// This updated version retains the ESP-NOW listener to catch the telemetry data from the Hive Data Receiver,
//  but instead of just printing it, it packs it into a binary payload and forwards it over Serial1 to the maTouch display. 
//  We've added a simple 0xAA, 0xBB header so the maTouch knows exactly where the struct begins.

#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <time.h>

// Add __attribute__((packed)) to force exact byte alignment
typedef struct __attribute__((packed)) struct_hive {
  uint8_t hiveId;
  float weight;
  float temp;
  float humidity;
} struct_hive;

typedef struct __attribute__((packed)) struct_message {
  float oat;
  float outside_humidity;
  uint32_t actual_time;
  uint32_t update_time;
  struct_hive hives[8];
} struct_message;

// Create a global variable to hold the incoming data
struct_message incomingReadings;
volatile bool newDataAvailable = false;

// --- UART PINS FOR SEEED C3 ---
// Adjust these to match the pins you solder/connect to the Grove cable.
// Default suggested pins for UART1 on C3:
#define TX_PIN 06
#define RX_PIN 07

// --- ESP-NOW RECEIVE CALLBACK ---
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  if (len == sizeof(struct_message)) {
    memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
    newDataAvailable = true;
  } else {
    Serial.printf("PACKET REJECTED: Expected %d bytes, got %d bytes.\n", sizeof(struct_message), len);
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize Hardware Serial 1 to transmit to the maTouch
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(1000);

  // --- ESP-NOW INITIALIZATION ---
  WiFi.mode(WIFI_STA);
  delay(3000);
  
  Serial.print("ESP32 Board MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  WiFi.disconnect(); 
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW Listener Active. Waiting for the ladies...");
}

void loop() {
  if (newDataAvailable) {
    newDataAvailable = false;
    
    Serial.println("Data caught via ESP-NOW. Forwarding to maTouch via UART...");
    
    // 1. Send sync header bytes so maTouch can align the incoming stream
    uint8_t header[2] = {0xAA, 0xBB};
    Serial1.write(header, 2);
    
    // 2. Transmit the exact binary structure
    Serial1.write((uint8_t*)&incomingReadings, sizeof(incomingReadings));
  }
}


