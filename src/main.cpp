#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <mbedtls/md.h>

#define SS_PIN 5
#define RST_PIN 21

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

const char* ssid = "SSID";
const char* password = "Pass";
const char* serverUrl = "http://192.168.100.1:3100/api/";
const char* deviceId = "98029792-d500-ac1cbb3a68a1";
const char* apiKey = "keeysss";

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  Serial.println("Ready to read RFID card...");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  Serial.println("Card detected");

  String cardData = readCardData();
  if (cardData.length() > 0) {
    sendDataToServer(cardData);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  //mfrc522.PCD_DumpVersionToSerial();

  delay(5000);
}

String readCardData() {
  String content = "";
  byte buffer[18];
  MFRC522::StatusCode status;

  // Start reading from sector 3, block 12
  int startBlock = 12;
  int endBlock = 63;

  for (int block = startBlock; block <= endBlock; block++) {
    // Skip sector trailer blocks
    if ((block + 1) % 4 == 0) {
      block++;
      if (block > endBlock) break;
    }

    // Authenticate the block
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("Authentication failed for block ")); Serial.print(block);
      Serial.print(F(": ")); Serial.println(mfrc522.GetStatusCodeName(status));
      return "";
    }

    // Read the block
    byte byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(block, buffer, &byteCount);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("Reading failed for block ")); Serial.print(block);
      Serial.print(F(": ")); Serial.println(mfrc522.GetStatusCodeName(status));
      return "";
    }

    for (byte i = 0; i < 16; i++) {
      if (buffer[i] != 0) {
        content += (char)buffer[i];
      } else {
        // Stop reading if we encounter a null byte
        Serial.print("Card data: ");
        Serial.println(content);
        return content;
      }
    }
  }

  Serial.print("Card data: ");
  Serial.println(content);
  return content;
}

void sendDataToServer(String cardData) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    String timestamp = String(millis());
    http.addHeader("deviceid", deviceId);
    http.addHeader("timestamp", timestamp);
    
    String signature = generateSignature(deviceId, timestamp);
    http.addHeader("signature", signature);

    // Parse the JSON data
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, cardData);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }

    // Prepare the request body
    String jsonBody;
    serializeJson(doc, jsonBody);

    int httpResponseCode = http.POST(jsonBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

String generateSignature(const String& deviceId, const String& timestamp) {
  
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)data.c_str(), data.length());
  
  unsigned char hmac[32];
  mbedtls_md_hmac_finish(&ctx, hmac);
  
  char hexOutput[65];
  for(int i = 0; i < 32; i++) {
    sprintf(hexOutput + (i * 2), "%02x", hmac[i]);
  }
  hexOutput[64] = '\0';
  
  mbedtls_md_free(&ctx);
  
  return String(hexOutput);
}
