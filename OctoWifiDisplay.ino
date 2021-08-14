#include <ESP8266WiFi.h> //ESP8266 Core WiFi Library
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>



LiquidCrystal_I2C lcd(0x27, 20, 4);

const char* mqtt_server = "192.168.1.2";
WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

int value = 0;

void setup() {
  Serial.begin(115200);
  delay(10);
  // put your setup code here, to run once:
  WiFi.mode(WIFI_STA);
  wifiManager.setTimeout(120);
  wifiManager.setAPCallback(configModeCallback);
  // initialize LCD
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to Wifi");
  
  wifiManager.autoConnect("OctoWifiDisplay");
  
  Serial.println("connected...yeey :)");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wifi is connected..");
  lcd.setCursor(0, 1);
  lcd.print("Connecting MQTT");
  randomSeed(micros());
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);



}

void configModeCallback (WiFiManager *myWiFiManager) {// this function should return true if it was connected to wifi. otherwise will dispay error message and start wifi access point
    Serial.println("unable to connect..");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Not Connected to");
    lcd.setCursor(0, 1);
    lcd.print("Wifi");
    lcd.setCursor(0, 2);
    lcd.print("Connect to Access");
    lcd.setCursor(0, 3);
    lcd.print("Point");
}


int Connected_ts, Printing_ts, Disconnected_ts , PrintDone_ts , PrintCancelled_ts;
int PrintingProgress;
String PrintingProgress_str;
String PrintingFilename ;
double  BedTemp_act , HoteEndTemp_act, BedTemp_tar , HoteEndTemp_tar;
String PrinterStatus;
String PrinterStatusOffline;
String PrintTimeLeft;
String totalLayer , currentLayer;


boolean PrintPowerState_bol;
String PrintPowerState_str;
long OffTime = millis();

    

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (PrinterStatus == "Printing Done" or  (PrinterStatusOffline == "Printing Finished" &&  PrinterStatus == "Disconnected" )    ){
    LCDBlink();
  }else{
    lcd.backlight();
  }


}



void LCDBlink(){
//Serial.println("In Blink");
  if (millis()-OffTime > 1000 && millis()-OffTime < 2000 ) {
    
    lcd.noBacklight();
  }else if(millis()-OffTime > 2000){
    
    lcd.backlight();
    OffTime = millis(); 
  }
  
}


void callback(char* topic, byte* payload, unsigned int length) {

  String topic_str(topic);
  Serial.println(topic_str);
  DynamicJsonDocument doc(512);
  deserializeJson(doc, payload);
  JsonObject obj = doc.as<JsonObject>();


  if (topic_str == "octoPrint/event/PrintStarted")
  {
    Printing_ts = doc["_timestamp"];
    //PrintingProgress = doc["progress"];
    PrintingFilename = doc["path"].as<String>();
    PrintTimeLeft = "NA";
    currentLayer = "-";
    totalLayer = "-";
    PrintingProgress_str = "0";
   
  }

  if (topic_str == "octoPrint/progress/printing")
  {
    //Printing_ts = doc["_timestamp"];
    PrintingProgress = doc["progress"];
    //PrintingFilename = obj["path"].as<String>();
  }
  if (topic_str == "octoPrint/event/Connected")
  {
    Connected_ts = doc["_timestamp"];

  }
  if (topic_str == "octoPrint/event/Disconnected")
  {
    Disconnected_ts = doc["_timestamp"];
   
  }
  if (topic_str == "octoPrint/event/PrintDone")
  {
    PrintDone_ts = doc["_timestamp"];
   


  }
  if (topic_str == "octoPrint/event/PrintCancelled")
  {
    PrintCancelled_ts = doc["_timestamp"];
 
  }
  if (topic_str == "octoPrint/temperature/tool0")
  {
    HoteEndTemp_act = doc["actual"];
    HoteEndTemp_tar = doc["target"];
  }
  if (topic_str == "octoPrint/temperature/bed")
  {
    BedTemp_act = doc["actual"];
    BedTemp_tar = doc["target"];
  }
  if (topic_str == "octoPrint/event/plugin_psucontrol_psu_state_changed")
  {

    PrintPowerState_bol = doc["isPSUOn"];
    if (PrintPowerState_bol) {
      PrintPowerState_str = "On";
    } else {
      PrintPowerState_str = "Off";
    }

  }
  if (topic_str == "octoPrint/event/DisplayLayerProgress_progressChanged" )
  {
    PrintingProgress_str = doc["progress"].as<String>();
    PrintTimeLeft = doc["printTimeLeft"].as<String>();
    totalLayer = doc["totalLayer"].as<String>();
    currentLayer = doc["currentLayer"].as<String>();
  }


  //......................................................................................................................................................................................//
  //......................................................................................................................................................................................//
  //......................................................................................................................................................................................//

  if ( Connected_ts > Printing_ts and Connected_ts > Disconnected_ts and Connected_ts > PrintDone_ts and Connected_ts > PrintCancelled_ts ) {
    PrinterStatus = "Connected";
    PrinterStatusOffline = "";
  }

  if ( Printing_ts > Connected_ts and Printing_ts > Disconnected_ts and Printing_ts > PrintDone_ts and Printing_ts > PrintCancelled_ts  ) {
    PrinterStatus = "Printing";


  }
  if ( Disconnected_ts > Printing_ts and Disconnected_ts > Connected_ts and Disconnected_ts > PrintDone_ts and Disconnected_ts > PrintCancelled_ts ) {
    PrinterStatus = "Disconnected";
  }
   if( PrintDone_ts > Printing_ts and PrintDone_ts > Disconnected_ts and PrintDone_ts > Connected_ts and PrintDone_ts > PrintCancelled_ts ){
    PrinterStatus = "Printing Done";
    PrinterStatusOffline = "Printing Done";

  }
  if ( PrintCancelled_ts > Printing_ts and PrintCancelled_ts > Disconnected_ts and PrintCancelled_ts > PrintDone_ts and PrintCancelled_ts > Connected_ts ) {
    PrinterStatus = "Printing Cancelled";


  }
  if ( PrintCancelled_ts > PrintDone_ts) { // for offline status......./////////////////////////////////////////////////////
    PrinterStatusOffline = "Printing Cancelled";
    

  }else{
    PrinterStatusOffline = "Printing Finished";

  }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  writeToDisplay();

  //Serial.print("ConnectedTS: ");  Serial.println(Connected_ts);
  //Serial.print("PrintingTs: ");  Serial.println(Printing_ts);
  //Serial.print("DisconnectedTS: ");  Serial.println(Disconnected_ts);
  //Serial.print("PrintDoneTS: ");  Serial.println(PrintDone_ts);
  //Serial.print("PrintCancelledTS: ");  Serial.println(PrintCancelled_ts);
  Serial.print("PrinterStatusOffline: ");  Serial.println(PrinterStatusOffline);





  //................................................................................................................................................................................//


}

void writeToDisplay() {
  Serial.println(PrinterStatus);

  lcd.clear();



  if (PrinterStatus != "Disconnected") {

    if (PrinterStatus == "Printing" or  PrinterStatus == "Printing Done" ) { // display progress ..................
      lcd.setCursor(0, 0);
      lcd.print(PrinterStatus + " " + String(PrintingProgress_str) + "%");
      lcd.setCursor(0, 1);
      lcd.print("Layer: " + currentLayer + ">" + totalLayer);

      lcd.setCursor(0, 2);
      lcd.print("TimeLeft: " + PrintTimeLeft);


    } else { // no progress ............................................
      lcd.setCursor(0, 0);
      lcd.print(PrinterStatus);
    }
    // display tempreture for hotend and bed ..........................................
    lcd.setCursor(0, 3);
    lcd.print("End:" + String(HoteEndTemp_act, 1) + " Bed:" + String(BedTemp_act, 1));



  }
  else {  // Printer is disconnected display the psu power state .................................................................................
    lcd.setCursor(0, 0);
    lcd.print(PrinterStatus + "(" + PrintPowerState_str + ")");
    lcd.setCursor(0, 1);
    lcd.print(PrinterStatusOffline);

  }


}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wifi is connected..");
      lcd.setCursor(0, 1);
      lcd.print("MQTT is Connected..");
      lcd.setCursor(0, 2);
      lcd.print("Getting Data Now");


      // Once connected, publish an announcement...
      // ... and resubscribe
      client.subscribe("octoPrint/temperature/#");
      client.loop();
      client.subscribe("octoPrint/event/PrintStarted");
      client.loop();
      //     client.subscribe("octoPrint/progress/printing");
      //     client.loop();
      client.subscribe("octoPrint/event/Connected");
      client.loop();
      client.subscribe("octoPrint/event/Disconnected");
      client.loop();
      client.subscribe("octoPrint/event/PrintDone");
      client.loop();
      client.subscribe("octoPrint/event/PrintCancelled");
      client.loop();
      client.subscribe("octoPrint/event/DisplayLayerProgress_progressChanged");
      client.loop();
   //   client.subscribe("octoPrint/event/DisplayLayerProgress_printerStateChanged");
  //    client.loop();
      client.subscribe("octoPrint/event/plugin_psucontrol_psu_state_changed");
      client.loop();
      
      



    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
