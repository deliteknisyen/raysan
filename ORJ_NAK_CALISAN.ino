/* -----------------------------------------------------------------------------
  - Project: Raysan Alarm Bariyer V2 
  -   - Date:  06/07/2023
  - Add Web İnterface with Wifi Manager And Manuel Test Virtual Button

   -----------------------------------------------------------------------------
  This code was created by Murat DALGIÇ 
  The AlarmBar Project by Raysan Bariyer
   ---------------------------------------------------------------------------*/



#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"



// Create AsyncWebServer object on port 80
AsyncWebServer server(80);



// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

// Sensor Tanımlamaları
const int sensorPin = 2;
const int buzzerPin = 4;

const char *device_id = "6155eb7f806a321b";
const char *area = "YUKLEME ALANI";
const char *company = "RAYSAN BARIYER";

// WiFi Tanımlamaları
//const char* ssid = "HiperNET";
//const char* password = "cilginpanda41"; 
String hostname = "RaysanBarrier";

// AP (Erişim Noktası) modu için gerekli ayarlar
const char* apSSID = "RYSAN_ALRM_B_AP";
const char* apPassword = "12345678";

// Server Connection Settings
const char* serverAddress = "http://raysanbariyer.com";
const int serverPort = 80;

// Alarmsız çalışma sınırları
const int lowerThreshold = 5;
const int upperThreshold = 100;

//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;
String StrUID;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";


// WiFi istemci ve sunucu nesnelerini 
WiFiClient client;



IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;

//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 255, 0);

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

// Set LED GPIO
// const int ledPin = 2; " Burada Tanımlama Yapılmış zaten Kontrol Edilip Değiştirilecek"
// Stores LED state

String buzzerState;

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Yüklenirken Bir Hata Oluştur Kontrol Edip Tekrar Deneyiniz");
  }
  Serial.println("SPIFFS Başarıyla Yüklendi");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Dosya Okunuyor: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- Dosya Okunurken Bir Hata Olustu Kontrol Edip Tekrar Deneyiniz");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Ayarlar Kaydediliyor: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- Kayıt Yapılırken Bir Hata Oluştu Kontrol Edip Tekrar Deneyiniz");
    return;
  }
  if(file.print(message)){
    Serial.println("- Ayarlar Yazılıyor");
  } else {
    Serial.println("- Ayarlar Kaydedilemedi");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Tanımsız SSID yada Ip Adresi.");
     return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());


  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Ayarlanamadı");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("WiFi Bağlantısı Yapılıyor...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
     Serial.println("Bağlantı Başarısız Oldu. SSID yok Yada Hatalı Ayar Parametresi");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

// Replaces placeholder with BUZZER state value
String processor(const String& var) {
  if(var == "STATE") {
    if(digitalRead(buzzerPin)) {
      buzzerState = "Buzzer Activated";
    }
    else {
      buzzerState = "Buzzer Deactivate";
    }
    return buzzerState;
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  
    initSPIFFS();

  // Set GPIO 2 as an OUTPUT
  
  pinMode(buzzerPin, OUTPUT);
  pinMode(sensorPin,INPUT);
  
  
  
  // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile (SPIFFS, gatewayPath);
  
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);

  if(initWiFi()) {
    // Route for root / web page ( Eğer Wifi Bağlantısı Başarılı Olursa )
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.serveStatic("/", SPIFFS, "/");
    
    // Route to set GPIO state to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(buzzerPin, HIGH);
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });

    // Route to set GPIO state to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(buzzerPin, LOW);
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.begin();
  }
  else {
    // Connect to Wi-Fi network with SSID and password ( Eğer Aktif Bağlantı yoksa sadece Wifi Ayar Sayfasına Gidecek)
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("Rysan_AlarmBar_Conf", NULL);
    WiFi.setHostname(hostname.c_str());
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL ( Eğer Wifi Bağlantısı Başarısız Olursa Wifi Ayar Sayfasına Gitmeli Kullanıcı)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "GUNCELLENDI.SISTEM YENIDEN BASLATILIYOR, ROUTER ILE BAGLANTI KURULDUGUNDAN BU ADRESI ZIYARET EDINIZ: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
}

void loop() {
    
   HTTPClient http;    //Declare object of class HTTPClient
    String UIDresultSend, postData;
    UIDresultSend = device_id;
    
    //Post Data
    postData = "UIDresult=" + UIDresultSend;
   
    // WiFi bağlantısı başarılı, titreşim sensöründen okuma yap
    int sensorValue = digitalRead(sensorPin);

    if (sensorValue == HIGH) {
      Serial.println("Alarm : Çarpma Tespit Edildi!");
      http.begin("http://barrier.buysafe.com.tr/getUID.php");  //Specify request destination
      http.addHeader("Content-Type", "application/x-www-form-urlencoded"); //Specify content-type header
      int httpCode = http.POST(postData);   //Send the request
      String payload = http.getString();    //Get the response payload
  
      Serial.println(UIDresultSend);
      Serial.println(httpCode);   //Print HTTP return code
      Serial.println(payload);    //Print request response payload
      http.end();  //Close connection
      delay(1000);
      // Buzzer'dan kesik sesli uyarı verin
      for (int i = 0; i < 5; i++) {
        digitalWrite(buzzerPin, HIGH);
        delay(600);
        digitalWrite(buzzerPin, LOW);
        delay(200);
      }
     
      }
    
  }
