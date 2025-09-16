# irrigacao_iot_PI

Projeto Integrador - Sistema IoT de Irrigação Inteligente (ESP32 + Dashboard)

Estrutura:
- CMakeLists.txt (top)
- main/CMakeLists.txt
- main/main.c
- main/main.h
- www/ (index.html, app.js, styles.css) - arquivos do dashboard servidos pelo ESP32

Como usar (resumo):
1. Instale o ESP-IDF e ferramentas (https://docs.espressif.com)
2. Coloque as credenciais WiFi em main/main.h (WIFI_SSID e WIFI_PASS) ou utilize menuconfig.
3. Compile e grave no ESP32 via `idf.py build` e `idf.py -p (PORT) flash`.
4. Acesse o IP do ESP32 no navegador (o IP será obtido pelo log do ESP-IDF).
5. O dashboard estará disponível na raiz (/) com endpoints de API em /api/status, /api/ligar e /api/desligar.

Obs: Para projeto de PI da UNIVESP, inclua no relatório as capturas de tela do dashboard, o esquema elétrico e a justificativa técnica.

"# irriga_PI_VI" 
