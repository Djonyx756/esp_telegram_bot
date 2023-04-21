#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h> 
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
 
// Вставьте сюда свои учетные данные WIFI сети
const char* ssid = "********";
const char* password = "********";
const int relay1 = 15;
 
// Запускаем бот
#define BOTtoken "**********************"  // Вставляем токен бота.
 
// Используйте @myidbot, чтобы получить ID пользователя или группы
// Помните, что бот сможет вам писать только после нажатия
// вами кнопки /start
#define CHAT_ID "-****************"
 
#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif
 
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
 
// Каждую секунду проверяет новые сообщения
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
 
const int ledPin = 2;
bool ledState = LOW;
 
// Задаем действия при получении новых сообщений 
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));
 
  for (int i=0; i<numNewMessages; i++) {
    // Идентификатор чата запроса
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Выводим полученное сообщение
    String text = bot.messages[i].text;
    Serial.println(text);
 
    String from_name = bot.messages[i].from_name;
 
    if (text == "/led_on") {
      bot.sendMessage(chat_id, "LED state set to ON1", "");
      ledState = HIGH;
      digitalWrite(ledPin, ledState);
      digitalWrite(ledPin, LOW);
      bot.sendMessage(chat_id, "open door", "");
      digitalWrite(relay1, LOW);
      delay(1000);
      digitalWrite(relay1, HIGH);
    }
    
    if (text == "/led_off") {
      bot.sendMessage(chat_id, "LED state set to OFF", "");
      ledState = LOW;
      digitalWrite(ledPin, ledState);
    }
    
    if (text == "/state") {
      if (digitalRead(ledPin)){
        bot.sendMessage(chat_id, "LED is ON", "");
      }
      else{
        bot.sendMessage(chat_id, "LED is OFF", "");
      }
    }
  }
}
 
void setup() {
  Serial.begin(115200);
  pinMode (relay1, OUTPUT);
  digitalWrite(relay1, HIGH);
 
  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");      // получаем всемирное координированное время (UTC) через NTP
    client.setTrustAnchors(&cert); // Получаем сертификат api.telegram.org
  #endif
 
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);
  
  // Подключаемся к Wi-Fi
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Добавляем корневой сертификат для api.telegram.org
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Выводим IP ESP32
  Serial.println(WiFi.localIP());

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
                                                                                                                                                       
  ArduinoOTA.begin();
                                                                                                                                                 
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  digitalWrite(relay1, HIGH);
  bot.sendMessage(CHAT_ID, "Bot Started", "");
}
 
void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
 
    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  ArduinoOTA.handle();
}
