#define TINY_GSM_MODEM_SIM800
#include <TinyGPS++.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <SoftwareSerial.h>

SoftwareSerial sim800(2, 3); // RX, TX serial modem
SoftwareSerial GPSpin(8, 7); // RX,TX serial GPS

// Internet setting
const char apn[]  = "internet"; // GSM apn 
const char user[] = "";
const char pass[] = "";

//Firebase setting 
const char server[] = "//FIREBASE PROJECT URL";
const int  port = 443;
const String Path = "position"; // firebase root table

//global variables
String fireData="";

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(sim800, Serial);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(sim800);
#endif

TinyGsmClientSecure client(modem);
HttpClient https(client, server, port);

TinyGPSPlus gps;
double latitude, longitude, targetLat, targetLng;
int modul, kontak, stater;
String getResponse;


void setup() {
  // Set console baud rate
  Serial.begin(115200);
  sim800.begin(9600);
  delay(3000);
  modem.restart();
  tesKoneksi(); // test internet connection
  GPSpin.begin(9600); // open gps serial
}

void loop() {
  if(GPSpin.isListening()){
    //Serial.println("gps listening");
    while(fireData.equals("")){
    scan();}
  }
  else{
    //Serial.println("gps not listening");
    fireData="";
    GPSpin.begin(9600); 
  }
}


void sendData(const char* method, const String & path , const String & data, HttpClient* http) {
  http->connectionKeepAlive(); // Currently, this is needed for HTTPS
  String url;


/////////////////////Send Data to Firebase//////////////////////  
  if (path[0] != '/') {
    url = "/";
  }
  url += path + ".json";
  url += "?auth=Ew64RsEmkf8ugmw2yzMu9sI2DKgejiCDjRZHO4Zd";
  String contentType = "application/json";
  
  http->beginRequest();
  http->put(url, contentType, data);
  http->endRequest();
  
//read the status code and body of the response//
  int putStatCode = http->responseStatusCode();
  String putResponse = http->responseBody();
  
  Serial.println(putStatCode);
  Serial.println(putResponse);

/////////////////////Receive Data from Firebase///////////////////////
  http->beginRequest();
  http->get("/control.json");
  http->endRequest();

  int getStatCode = http->responseStatusCode();
//  int httplength = http->contentLength()+1;
//  char getResponse[httplength];
//  http->responseBody().toCharArray(getResponse, httplength);

  String response = http->responseBody();
  
  Serial.println(getStatCode);
  Serial.println(response);

  int contentStart = response.indexOf("{");
  int contentEnd = response.indexOf("}");
  String content = response.substring(contentStart + 1, contentEnd);
//  Serial.println(content);

  int value1start = content.indexOf(":");
  String value1 = content.substring(value1start + 1);
  stater = value1.toInt();
  Serial.println("stater");
  Serial.println(stater);
  
  int value2start = content.indexOf("kontak\":")+8;
  int value2end = value2start + 1;
  String value2 = content.substring(value2start, value2end);
  kontak = value2.toInt();
  Serial.println("kontak");
  Serial.println(kontak);

  int value3start = content.indexOf("modul\":")+7;
  int value3end = value3start + 1;
  String value3 = content.substring(value3start, value3end);
  modul = value3.toInt();
  Serial.println("modul");
  Serial.println(modul);
  
  if (!http->connected()) {
    Serial.println();
    http->stop();// Shutdown
    Serial.println("HTTP disconnected");
    sim800.begin(9600);
    tesKoneksi();
    fireData="";
    GPSpin.begin(9600);
  }
}

/////////////////////////////////scanning gps location/////////////////////////////////
void scan(){
  while (GPSpin.available() > 0)
    if (gps.encode(GPSpin.read()))
      displayInfo(); 
}

void displayInfo()
{
  gps.location.isValid();
  latitude = gps.location.lat();
  longitude = gps.location.lng();
  data();
  sim800.begin(9600);
  sendData("PUT", Path, fireData, &https);   
}

/////////////////////////////////connect to internet///////////////////////////////// 
void tesKoneksi(){
  tes:
  Serial.print(F("Koneksi ke Jaringan..."));
  if (!modem.waitForNetwork()) {
    Serial.println(" gagal");
    delay(300);
    goto tes;
  }
  Serial.println(" OK");
  Serial.print(F("Koneksi ke apn: "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" gagal");
    delay(300);
    goto tes;
  }
  Serial.println(" OK");
}

/////////////////////////////////get gps data/////////////////////////////////
void data(){
    fireData="{";
    fireData += " \"lat\" :" + String(latitude,6)+",";
    fireData += " \"lng\" :" + String(longitude,6)+"";
    fireData +="}";
}
