/*************************************************************************
   PROJECT: IoT Gateway
   AUTHOR: GCT Aquavison - EEE-25/GCT
 
   FUNC: To send and receive MQTT messages to HiveMQ cloud server.
   
   SIMCARD: 4G sim cards from Airtel/Vodaphone/Jio/BSNL can be used. 
   
   IMPORTANT: Configure the APN accordingly as per your Sim provider
   
   COPYRIGHT: BharatPi @MIT license for usage on Bharat Pi boards
 *************************************************************************/

 #define TINY_GSM_MODEM_SIM7600 //TINY_GSM_MODEM compatible for 7672 as well
 #define TINY_GSM_RX_BUFFER 1024
 
 
 #define SerialAT Serial1
 #define SerialMon Serial
 
 #define UART_BAUD   115200
 #define PIN_DTR     25
 #define PIN_TX      17
 #define PIN_RX      16
 #define PWR_PIN     32
 
 #define LED_PIN 2
 
 #include <TinyGsmClient.h> //Lib version - 0.12.0
 #include "SSLClient.h" //Lib version - 2.1.7
 #include <PubSubClient.h> //Lib version - 2.8

 #include <WiFi.h>
 #include <WebSocketsServer.h>
 #include <ArduinoJson.h>

 const char* ssid = "IoT-Gateway";
 const char* password = "12345678";

 WebSocketsServer webSocket(81);

 struct ClientInfo {
    uint8_t id;        // WebSocket client ID (assigned by library)
    bool isConnected;
    String clientId;   // Fixed unique identifier
    String name;       // Fixed name
 };

 // Define your fixed clients
 const int MAX_CLIENTS = 2; // Increase if needed
 ClientInfo clients[2] = {
    {255, false, "device1", "Drinking-WaterTank"},
    {255, false, "device2", "Domestic-WaterTank"},
 };
 
 //CA Certificate for HiveMQ Private Cluster
 const char root_ca[] PROGMEM =
 "-----BEGIN CERTIFICATE-----\n"
 "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\n"
 "-----END CERTIFICATE-----";
 
 const char* sub_topics[] = {
  "Pumphouse/Domestic-WaterTank/Pump_State",
  "Pumphouse/Domestic-WaterTank/Mode",
  "Pumphouse/Domestic-WaterTank/Timer",
  "Pumphouse/Domestic-WaterTank/Timerdata",
  "Pumphouse/Drinking-WaterTank/Pump_State",
  "Pumphouse/Drinking-WaterTank/Mode",
  "Pumphouse/Drinking-WaterTank/Timer",
  "Pumphouse/Drinking-WaterTank/Timerdata"
};
 

 /*********************************************
   SECTION: Set APN based on your sim card
     AIRTEL: "airtelgprs.com" 
     BSNL: "bsnlnet / bsnllive" 
     VI: "portalnmms"
     JIO: "jionet"
 *********************************************/
 const char apn[]  = "bsnlnet"; //Change this as per your Sim card operator
 
 /*********************************************
   SECTION: MQTT Broker Credentials
 *********************************************/
 const char* mqtt_server = "8e3ddd6ba80a4e3e99739281bebb36d8.s1.eu.hivemq.cloud";
 const int mqtt_port = 8883;  
 const char* mqtt_username = "Sandhep";
 const char* mqtt_password = "Sandhep13";
 const char* clientID = "ESP32";
 const char* lwt_topic = "Pumphouse/LWT/Status";
 const char* lwt_message = "Offline";
 const int qos = 1;
 const bool lwt_retain = true;
 
 TinyGsm modem(SerialAT);
 
 TinyGsmClient gsm(modem);
 SSLClient secure_client(&gsm);
 PubSubClient  client(secure_client);
 

 
 /*********************************************
   SECTION: Modem operation functions
 *********************************************/
 void modemPowerOn(){
   pinMode(PWR_PIN, OUTPUT);
   digitalWrite(PWR_PIN, LOW);
   delay(1000);
   digitalWrite(PWR_PIN, HIGH);
 }
 
 void modemPowerOff(){
   pinMode(PWR_PIN, OUTPUT);
   digitalWrite(PWR_PIN, LOW);
   delay(1500);
   digitalWrite(PWR_PIN, HIGH);
 }
 
 void modemRestart(){
   modemPowerOff();
   delay(1000);
   modemPowerOn();
 }
 
 void printLocalTime()
 {
   struct tm timeinfo;
   if(!getLocalTime(&timeinfo)){
     Serial.println("Failed to obtain time");
     return;
   }
   Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
 }
 
 // Define your phone numbers array
 String phoneNumbers[] = {"9384584369"};
 
 void sendSMS(String message) {
 
   // Send SMS to each number in the array
 
   for (int i = 0; i < sizeof(phoneNumbers) / sizeof(phoneNumbers[0]); i++) {
     Serial.print("Sending SMS to ");
     Serial.println(phoneNumbers[i]);
 
     // Send SMS
     modem.sendSMS(phoneNumbers[i], message);
 
     delay(500); // Delay between SMS sends
   }
   Serial.println();
 }

 /*********************************************
   SECTION: WebSocket Operations 
 *********************************************/

 // Find client by WebSocket ID
ClientInfo* findClientByWsId(uint8_t wsId) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].id == wsId && clients[i].isConnected) {
          return &clients[i];
      }
  }
  return nullptr;
}

// Find client by clientId
ClientInfo* findClientById(String clientId) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].clientId == clientId) {
          return &clients[i];
      }
  }
  return nullptr;
}

void sendJsonMessage(uint8_t clientNum, const char* event, const char* value) {
  JsonDocument doc;
  doc["event"] = event;
  doc["value"] = value;
  String jsonString;
  serializeJson(doc, jsonString);
  webSocket.sendTXT(clientNum, jsonString);
}

void sendMessageToClient(ClientInfo& device, const char* event, const char* value) {
  if (device.isConnected) {
      JsonDocument doc;
      doc["event"] = event;
      doc["value"] = value;
      String jsonString;
      serializeJson(doc, jsonString);
      webSocket.sendTXT(device.id, jsonString);
      Serial.printf("Sent message to %s (%s)\n", device.name.c_str(), device.clientId.c_str());
  }
}

void sendMessagetoAllClients() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].isConnected) {
          sendMessageToClient(clients[i], "message", ("Hello " + clients[i].name + " - " + String(millis())).c_str());
      }
  }
}

void handleEvent(uint8_t clientNum, uint8_t* payload) {
  String message = String((char*)payload);
  Serial.printf("Received from [%u]: %s\n", clientNum, message.c_str());

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
      Serial.println("JSON Parsing Failed!");
      sendJsonMessage(clientNum, "error", "Invalid JSON");
      return;
  }

  String event = doc["event"];
  
  // First identify which registered client this is
  ClientInfo* device = findClientByWsId(clientNum);
  if (!device) {
      Serial.println("Message from unregistered client!");
      sendJsonMessage(clientNum, "error", "Not registered");
      return;
  }
  
  Serial.printf("Processing command from %s (%s)\n", device->name.c_str(), device->clientId.c_str());

  if (event == "OHT_FLOAT") {

      String value = doc["value"];

      if (value == "ON") {

          char topic[200]; 
          sprintf(topic, "Pumphouse/%s/Sensor/OHT_Float", device->name.c_str());
          client.publish(topic, "ON");

          sendJsonMessage(clientNum, "status", "OHT_FLOAT ON");

      } else {

          char topic[200];  
          sprintf(topic, "Pumphouse/%s/Sensor/OHT_Float", device->name.c_str());
          client.publish(topic, "OFF");

          sendJsonMessage(clientNum, "status", "OHT_FLOAT OFF");
      }

  } else if (event == "UGT_FLOAT") {
      String value = doc["value"];
      if (value == "ON") {

          char topic[200]; 
          sprintf(topic, "Pumphouse/%s/Sensor/UGT_Float", device->name.c_str());
          client.publish(topic, "ON");

          sendJsonMessage(clientNum, "status", "UGT_FLOAT ON");
      } else {

          char topic[200]; 
          sprintf(topic, "Pumphouse/%s/Sensor/UGT_Float", device->name.c_str());
          client.publish(topic, "OFF");

          sendJsonMessage(clientNum, "status", "UGT_FLOAT OFF");
      }
  } else if(event == "PumpState"){

     String value = doc["value"];

     if (value == "ON") {

          char topic[200]; 
          sprintf(topic, "Pumphouse/%s/ManualSwitch/Pump_State", device->name.c_str());
          client.publish(topic, "ON");

          sendJsonMessage(clientNum, "status", "Pump ON");

      } else {

          char topic[200]; 
          sprintf(topic, "Pumphouse/%s/ManualSwitch/Pump_State", device->name.c_str());
          client.publish(topic, "OFF");

          sendJsonMessage(clientNum, "status", "Pump OFF");

     }

  } else if (event == "register") {
          // Already handled during connection
          sendJsonMessage(clientNum, "status", "Already registered");
  }
}

void webSocketEvent(uint8_t clientNum, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
      case WStype_CONNECTED: {
          Serial.printf("Client [%u] connected\n", clientNum);
          
          // Extract URL parameters (e.g., ws://ip/?clientId=device1)
          String url = String((char*)payload);
          int paramStart = url.indexOf("clientId=");
          
          if (paramStart > 0) {
              String clientId = url.substring(paramStart + 9); // 9 is length of "clientId="
              clientId.trim();
              
              // Find the predefined client with this ID
              ClientInfo* client = findClientById(clientId);
              
              if (client) {
                  client->id = clientNum;
                  client->isConnected = true;
                  Serial.printf("Recognized %s as %s (client [%u])\n", 
                               clientId.c_str(), client->name.c_str(), clientNum);
                  sendJsonMessage(clientNum, "welcome", client->name.c_str());
              } else {
                  Serial.printf("Unknown clientId: %s\n", clientId.c_str());
                  sendJsonMessage(clientNum, "error", "Unknown client ID");
              }
          } else {
              Serial.println("Client connected without clientId");
              sendJsonMessage(clientNum, "error", "No client ID provided");
          }
          break;
      }

      case WStype_DISCONNECTED:
          Serial.printf("Client [%u] disconnected\n", clientNum);
          for (ClientInfo client : clients) {
              if (client.id == clientNum) {
                  client.isConnected = false;
                  client.id = 255; // Reset the WebSocket ID
                  Serial.printf("%s (%s) disconnected\n", 
                               client.name.c_str(), client.clientId.c_str());
              }
          }
          break;

      case WStype_TEXT:
          handleEvent(clientNum, payload);
          break;
  }
 }

 /*********************************************
   SECTION: MQTT Operations 
 *********************************************/
 
 void callback(char* topic, byte* payload, unsigned int length) {
   Serial.print("Message arrived [");
   Serial.print(topic);
   Serial.print("] ");
   String message = "";
   for (int i = 0; i < length; i++) {
      message += (char)payload[i];
   }
   Serial.println(message);

   const char* command = message.c_str();

   String messageTopic = String(topic);

   if(messageTopic.equals("Pumphouse/Domestic-WaterTank/Pump_State")){

     Serial.println("Message sent to Device 2");
     ClientInfo* client = findClientById("device2");
     sendJsonMessage(client->id, "Pump_State", command);

   }else if(messageTopic.equals("Pumphouse/Drinking-WaterTank/Pump_State")){

     Serial.println("Message sent to Device 1");
     ClientInfo* client = findClientById("device1");
     sendJsonMessage(client->id, "Pump_State", command);

   }else if(messageTopic.equals("Pumphouse/Domestic-WaterTank/Mode")){

      Serial.println("Message sent to Device 2");
      ClientInfo* client = findClientById("device2");
      sendJsonMessage(client->id, "Mode", command);

   }else if(messageTopic.equals("Pumphouse/Drinking-WaterTank/Mode")){

      Serial.println("Message sent to Device 1");
      ClientInfo* client = findClientById("device1");
      sendJsonMessage(client->id, "Mode", command);
      
   }
 }
 
 void reconnect() {

   while (!client.connected()) {

     Serial.print("Attempting MQTT connection...");

     if (client.connect(clientID, mqtt_username, mqtt_password, lwt_topic, qos, lwt_retain, lwt_message)) {
       
      Serial.println("connected");
      client.publish("Pumphouse/Domestic-WaterTank/Status", "Online", true);
      client.publish("Pumphouse/Drinking-WaterTank/Status", "Online", true);
      
      // Subscribe to control topics
      for (const char* topic : sub_topics) {
        client.subscribe(topic, qos);
      }
       
     } else {
       Serial.print("failed, rc=");
       Serial.print(client.state());
       Serial.println(" try again in 5 seconds");
       delay(5000);
     }
   }
 }

 /*********************************************
   SECTION: Main setup
 *********************************************/
 void setup(){
   // Set console baud rate
   SerialMon.begin(115200);
   pinMode(LED_PIN, OUTPUT);
   digitalWrite(LED_PIN, LOW);
 
   delay(100);
 
   modemPowerOn();
  
   SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  
   secure_client.setCACert(root_ca);  // Adding Security Certificates
 
   Serial.clearWriteError();
   Serial.println();
   Serial.println();
   Serial.println("/*******************************************************************************/");
   Serial.println("  Bharat Pi HiveMQ Cloud Sync using MQTT over 4G/LTE ");
   Serial.println("");
   Serial.println("  IMPORTANT: To initialize/latch the 4G/LTE network, please make sure the antenna has been");
   Serial.println("  connected, SIM is inserted in the SIM slot (back side of the board) and 9V 2A power adapter is connected.");
   Serial.println("/******************************************************************************/\n\n");
 
   delay(2000);
 
   String res;
   Serial.println("Initializing IoT Gateway...");
 
   if (!modem.init()) {
     digitalWrite(LED_PIN, HIGH);
     modemRestart();
     delay(2000);
     Serial.println("Failed to restart modem, continue without restarting");
     digitalWrite(LED_PIN, LOW);
     return;
   }
 
   //Blue LED on the board use as an indicator
   //If blinking: Modem not able to boot
   //If turned ON: connected to network
   //If turned OFF: Modem booted successfully but not connected to network, check your SIM, network coverage etc.
 
   digitalWrite(LED_PIN, LOW); 
 
   Serial.println("Running SIMCOMATI command...");
   modem.sendAT("+SIMCOMATI"); //Get the module information
   modem.waitResponse(1000L, res);
   //res.replace(GSM_NL "OK" GSM_NL, "");
   Serial.println(res);
   res = "";
   Serial.println();
 
   Serial.println("Preferred mode selection (GSM/LTE)...");
   modem.sendAT("+CNMP?");
   if (modem.waitResponse(1000L, res) == 1) {
     //res.replace(GSM_NL "OK" GSM_NL, "");
     Serial.println(res);
   }
   res = "";
   Serial.println();
 
   //Get module manufacturer details
   String modemName = modem.getModemModel();
   Serial.println("Modem Name : " + modemName);
   delay(1000);
 
   String modemInfo = modem.getModemInfo();
   Serial.println("Modem Info : " + modemInfo);
   delay(1000);
 
   /*********************************************
     SECTION: Connect to Sim network 
   *********************************************/
   Serial.println("Network mode connectivity testing (GSM, LTE or GSM/LTE)...");
 
     uint8_t network= 38;
     modem.setNetworkMode(network);
     delay(3000);
     bool isConnected = false;
     int tryCount = 60;
     while (tryCount--) {
       String netoworkOerator = modem.getOperator();
       Serial.print("Operator: ");
       Serial.println(netoworkOerator);
       int16_t signal =  modem.getSignalQuality();
       Serial.print("Signal: ");
       Serial.println(signal);
       Serial.print("isNetworkConnected: ");
       isConnected = modem.isNetworkConnected();
       Serial.println( isConnected ? "CONNECTED" : "NOT CONNECTED YET");
       if (isConnected) {
         break;
       }
       delay(1000);
       digitalWrite(LED_PIN, !digitalRead(LED_PIN));
     }
 
   digitalWrite(LED_PIN, HIGH); //Modem connected to network
 
   Serial.println();
   Serial.println("Yehhh....Device is connected to Sim network.");
   Serial.println();
 
   delay(1000);
   Serial.println("Checking UE (User Equipment) system information...");
   Serial.println();
   modem.sendAT("+CPSI?");
   if (modem.waitResponse(1000L, res) == 1) {
     res.replace(AT_NL "OK" AT_NL, "");
     Serial.println(res);
   }
 
   delay(1000);  
   Serial.println("");
   Serial.println("");
 
   if(modem.isNetworkConnected()){
     Serial.println("Mobile Network is connected.......");
   }  
   // GPRS connection parameters are usually set after network registration
   SerialMon.print(F("DATA Connection: Connecting to APN: "));
   SerialMon.print(apn);
   if(!modem.gprsConnect(apn, "", "")) {
     Serial.println(" APN connect failed");
     delay(10000);
     return;
   }
   Serial.println(" APN connect success");
 
   if (modem.isGprsConnected()) { 
     Serial.println("");
     Serial.println("GPRS network is connected");
   }
   Serial.println("");
 
   /******************************************************************
   SECTION: Set variables and values for pushing data to cloud via MQTT.
            If you want to dynamically capture data from 
            sensors or GPIOs then move this section to Loop 
            and set variables accordingly to push data to HiveMQ Cloud.
   *******************************************************************/
   delay(5000);

   client.setServer(mqtt_server, mqtt_port);
   client.setCallback(callback);

   /******************************************************************
   SECTION: Start Websocket Communications
   *******************************************************************/

   WiFi.softAP(ssid, password);
   Serial.print("IoT Gateway Access Point IP Address: ");
   Serial.println(WiFi.softAPIP());

   webSocket.begin();
   webSocket.onEvent(webSocketEvent);
   
 }
 
 /******************************************************************
 SECTION: Loop call for capturing dynamic data from sensors/GPIOs
 *******************************************************************/  
 
 void loop(){
  if (!client.connected()) {
     reconnect();
   }
   client.loop();
   webSocket.loop();
 }