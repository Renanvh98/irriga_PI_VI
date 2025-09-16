#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "sdkconfig.h"
#include "main.h"

static const char *TAG = "IRRIGACAO_PI";

static bool irrigando = false;
static int ultima_umidade = 70;
static float ultima_vazao = 0.0;

// --- WiFi --- (simple station connect)
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    // placeholder - using example esp-idf events requires more code; keep minimal logging
    ESP_LOGI(TAG, "WiFi event %d", event_id);
}

static void wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
    ESP_LOGI(TAG, "WiFi init started (SSID=%s)", WIFI_SSID);
}

// --- Simulação / Leitura de sensores ---
// Aqui usamos leituras simuladas para demo. Substitua por ADC real/driver do sensor capacitivo.
static int ler_umidade_simulada() {
    // pequena flutuação para demonstrar
    int delta = (esp_random() % 5) - 2; // -2..2
    int value = ultima_umidade + delta;
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    ultima_umidade = value;
    return value;
}

static float ler_vazao_simulada() {
    // se estiver irrigando, retornar ~2.4 L/min, caso contrário 0
    if (irrigando) {
        float v = 2.2f + (esp_random() % 40) / 100.0f; // 2.20 .. 2.59
        ultima_vazao = v;
        return v;
    } else {
        ultima_vazao = 0.0f;
        return 0.0f;
    }
}

// --- Controlador da válvula (relé) ---
static void set_irrigacao(bool ligar) {
    irrigando = ligar;
    gpio_set_level(RELAY_GPIO, ligar ? 1 : 0);
    ESP_LOGI(TAG, "Comando irrigacao: %s", ligar ? "LIGAR" : "DESLIGAR");
}

// --- HTTP API Handlers ---
// GET /api/status -> JSON with umidade, vazao, irrigando
static esp_err_t api_status_get_handler(httpd_req_t *req) {
    char buffer[256];
    int um = ultima_umidade;
    float vz = ultima_vazao;
    const char *fmt = "{ "umidade": %d, "vazao": %.2f, "irrigando": %s }";
    int len = snprintf(buffer, sizeof(buffer), fmt, um, vz, irrigando ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buffer, len);
    return ESP_OK;
}

// POST /api/ligar -> liga a irrigacao
static esp_err_t api_ligar_post_handler(httpd_req_t *req) {
    set_irrigacao(true);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{ "ok": true, "msg": "Irrigacao ligada" }");
    return ESP_OK;
}

// POST /api/desligar -> desliga a irrigacao
static esp_err_t api_desligar_post_handler(httpd_req_t *req) {
    set_irrigacao(false);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{ "ok": true, "msg": "Irrigacao desligada" }");
    return ESP_OK;
}

// Serve the static dashboard files embedded as simple strings for demo.
// For a real project, sirva via SPIFFS or LittleFS; aqui mostramos endpoints básicos.

extern const unsigned char index_html[] asm("_binary_www_index_html_start");
extern const unsigned char index_html_end[] asm("_binary_www_index_html_end");
extern const unsigned char app_js[] asm("_binary_www_app_js_start");
extern const unsigned char app_js_end[] asm("_binary_www_app_js_end");
extern const unsigned char styles_css[] asm("_binary_www_styles_css_start");
extern const unsigned char styles_css_end[] asm("_binary_www_styles_css_end");

static esp_err_t root_get_handler(httpd_req_t *req) {
    const unsigned char *buf = index_html;
    size_t len = (size_t)(index_html_end - index_html);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char*)buf, len);
    return ESP_OK;
}

static esp_err_t appjs_get_handler(httpd_req_t *req) {
    const unsigned char *buf = app_js;
    size_t len = (size_t)(app_js_end - app_js);
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char*)buf, len);
    return ESP_OK;
}

static esp_err_t styles_get_handler(httpd_req_t *req) {
    const unsigned char *buf = styles_css;
    size_t len = (size_t)(styles_css_end - styles_css);
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char*)buf, len);
    return ESP_OK;
}

static httpd_uri_t uri_get_status = {
    .uri       = "/api/status",
    .method    = HTTP_GET,
    .handler   = api_status_get_handler,
    .user_ctx  = NULL
};

static httpd_uri_t uri_post_ligar = {
    .uri       = "/api/ligar",
    .method    = HTTP_POST,
    .handler   = api_ligar_post_handler,
    .user_ctx  = NULL
};

static httpd_uri_t uri_post_desligar = {
    .uri       = "/api/desligar",
    .method    = HTTP_POST,
    .handler   = api_desligar_post_handler,
    .user_ctx  = NULL
};

static httpd_uri_t uri_root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
    .user_ctx = NULL
};

static httpd_uri_t uri_appjs = {
    .uri = "/app.js",
    .method = HTTP_GET,
    .handler = appjs_get_handler,
    .user_ctx = NULL
};

static httpd_uri_t uri_styles = {
    .uri = "/styles.css",
    .method = HTTP_GET,
    .handler = styles_get_handler,
    .user_ctx = NULL
};

static httpd_handle_t start_rest_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_root);
        httpd_register_uri_handler(server, &uri_appjs);
        httpd_register_uri_handler(server, &uri_styles);
        httpd_register_uri_handler(server, &uri_get_status);
        httpd_register_uri_handler(server, &uri_post_ligar);
        httpd_register_uri_handler(server, &uri_post_desligar);
    }
    return server;
}

void sensor_task(void *pvParameters) {
    while (1) {
        int um = ler_umidade_simulada();
        float vz = ler_vazao_simulada();

        ESP_LOGI(TAG, "Leitura -> Umidade: %d%%, Vazao: %.2f L/min, Irrigando: %s", um, vz, irrigando ? "SIM" : "NAO");

        // lógica simples de controle automático
        if (!irrigando && um < UMIDADE_LIMIAR) {
            set_irrigacao(true);
            ESP_LOGI(TAG, "Umidade abaixo do limiar (%d%%). Iniciando irrigacao automaticamente.", UMIDADE_LIMIAR);
        } else if (irrigando && um >= (UMIDADE_LIMIAR + 8)) {
            set_irrigacao(false);
            ESP_LOGI(TAG, "Umidade atingiu nivel seguro. Parando irrigacao.");
        }

        vTaskDelay(pdMS_TO_TICKS(10000)); // 10s
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      nvs_flash_erase();
      nvs_flash_init();
    }
    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_LOGI(TAG, "Iniciando sistema...");

    // Inicializa GPIO do relé
    gpio_reset_pin(RELAY_GPIO);
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(RELAY_GPIO, 0);

    // WiFi (opcional) - inicia tentativa de conexão
    wifi_init_sta();

    // Inicia servidor HTTP
    start_rest_server();

    // Task que simula leituras e faz controle automático
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}
