#include "web/WebUI.h"
#include "web/API.h"
#include "web/web_ui_html.h"
#include <ESPAsyncWebServer.h>

void webUIBegin() {
    apiGetServer()->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(
            200, "text/html", HTML_GZ, HTML_GZ_LEN);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
}
