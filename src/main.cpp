#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <mbedtls/md.h>
#include "WiFi.h"

const char* deviceId = "98029792-d500-489a-9f02-ac1cbb3a68a1";
const char* apiKey = "eb37cec3bc5133ec7028f0472acb0bb0adda578bae1e4dc88c0ac153bedd6208";
const char* serverUrl = "http://192.168.100.15:3100/api/chargeVerfiy/merchant-initiate-verify";

String createSignature(const String& deviceId, const String& timestamp) {
  String data = deviceId + timestamp;

  byte hmacResult[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)apiKey, strlen(apiKey));
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)data.c_str(), data.length());
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);

  String signature = "";
  for (int i = 0; i < sizeof(hmacResult); i++) {
    char str[3];
    sprintf(str, "%02x", (int)hmacResult[i]);
    signature += str;
  }

  return signature;
}

void initializeTransfer() {
  HTTPClient http;  // Changed from https to http
  
  Serial.println("Connecting to: " + String(serverUrl));
  http.begin(serverUrl);

  String timestamp = String(time(nullptr));
  String requestBody = "{\"nfcData\":{\"phoneNumber\":\"0556296669\",\"email\":\"enochcobbina1@gmail.com\",\"customerId\":\"CUS_gqekouycyz27yxu\"},\"amount\":0.1}";
  String signature = createSignature(deviceId, timestamp);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("deviceid", deviceId);
  http.addHeader("timestamp", timestamp);
  http.addHeader("signature", signature);

  Serial.println("Sending POST request...");
  int httpCode = http.POST(requestBody);

  if (httpCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    String response = http.getString();
    Serial.println("Response:");
    Serial.println(response);
  } else {
    Serial.printf("Error on HTTP request: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  const char* ssid = "B612";
  const char* password = "SheReallyIs1";
  
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  configTime(0, 0, "pool.ntp.org");
  
  initializeTransfer();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    initializeTransfer();
  } else {
    Serial.println("WiFi Disconnected");
  }
  delay(100000);
}
