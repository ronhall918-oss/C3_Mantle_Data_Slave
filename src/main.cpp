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

// A safety flag to tell the main loop we have new data
volatile bool newDataAvailable = false;


// --- ESP-NOW RECEIVE CALLBACK (Updated for ESP32 Core v3.x) ---
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
    // Check if the packet is exactly the size we expect
    if (len == sizeof(struct_message)) {
        // Instantly copy the radio bytes directly into our structured variable
        memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
        
        // Throw the flag so the main loop knows to update the screen safely
        newDataAvailable = true;
    } else {
        Serial.printf("PACKET REJECTED: Expected %d bytes, got %d bytes.\n", sizeof(struct_message), len);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // --- ESP-NOW INITIALIZATION ---
    // 1. Turn on WiFi but do NOT connect to a router.
    WiFi.mode(WIFI_STA);
    delay(3000);
      // Get and print MAC address
    Serial.print("ESP32 Board MAC Address: ");
    Serial.println(WiFi.macAddress());
    WiFi.disconnect(); // Prevent background scanning which kills PSRAM bandwidth!
    
    // Disable Wi-Fi Sleep. When sleep is enabled, the radio rapidly cycles on and off,
    // which causes massive spikes in PSRAM latency and causes the RGB panel to tear.
    // Keeping it constantly on creates a stable baseline for the DMA.
    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);
    
    // 2. Force the radio to Channel 1 to match the House Receiver
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    // 3. Initialize the ESP-NOW protocol
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    // 4. Register our callback function
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("ESP-NOW Listener Active. Waiting for the ladies...");
    
    // 5. Print the MAC address so we can pair the transmitter later!
    Serial.print("ESP-NOW Listener Active on Channel 1. MAC Address: ");
    Serial.println(WiFi.macAddress());
}


void loop() {
  // If the radio caught a new packet...
    if (newDataAvailable) {
        // 1. Clear the flag immediately
        newDataAvailable = false;
        
        // 2. Print to Serial to verify (You will replace this with LVGL label updates later)
        Serial.printf("Data Updated!");
        Serial.printf("Outside Temp: %.1f\n", incomingReadings.oat);
        Serial.printf("Outside Humidity: %.1f\n", incomingReadings.outside_humidity);
        Serial.printf("Actual Time: %u\n", incomingReadings.actual_time);
        Serial.printf("Last Update: %u\n", incomingReadings.update_time);
        Serial.printf("Hive 1 Weight: %.1f lbs\n", incomingReadings.hives[0].weight);
        Serial.printf("Hive 8 Temp: %.1f F\n", incomingReadings.hives[7].temp);
        
        }


}