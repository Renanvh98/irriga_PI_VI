#ifndef MAIN_H
#define MAIN_H

// Configure your WiFi here (or use menuconfig in ESP-IDF)
#define WIFI_SSID "SEU_SSID_AQUI"
#define WIFI_PASS "SUA_SENHA_AQUI"

// Threshold for irrigation (0-100%)
#define UMIDADE_LIMIAR 45

// GPIO pin definitions (adjust conforme sua placa)
#define RELAY_GPIO 25      // GPIO para controlar o relé (active HIGH)
#define SOIL_ADC_CHANNEL ADC1_CHANNEL_6 // GPIO34 (ADC1_CH6) - só referência

#endif // MAIN_H
