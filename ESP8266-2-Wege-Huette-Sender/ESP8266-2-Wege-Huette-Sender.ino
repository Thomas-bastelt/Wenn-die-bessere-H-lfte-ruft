#include <ESP8266WiFi.h>
#include <espnow.h>

// MAC-Adresse des Empfängers in der Hütte für den Sender
uint8_t broadcastAddress[] = {0xAC, 0x0B, 0xFB, 0xD0, 0x27, 0x7F};

const int heizung=5, heiz_taster=4;
const int rufton=12, ruf_taster=13;
bool t_signal, t_ea=false, h_signal, h_ea=false;
bool s_rufton=false, s_heizung=false;
bool b_heizung=LOW, b_ruf=LOW, rb_heizung=LOW, rb_ruf=LOW;

//Struktur, um die Tastersignale zu speichern
//Muss mit der Empfängerstruktur übereinstimmen!
typedef struct struct_signale {
    bool b_heizung;
    bool b_ruf;
} struct_signale;

// Eine Signalstruktur zur Übermittlung der Signale definieren
struct_signale signale;

// Eine Signalstruktur für die empfangenen Rücksignale, werden aber nicht gebraucht!
struct_signale rueck_signale;


// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

// Callback when data is received
void OnDataRecv(uint8_t * mac, uint8_t *eingangssignale, uint8_t len) {
  memcpy(&rueck_signale, eingangssignale, sizeof(eingangssignale));
  Serial.print("Bytes received: ");
  Serial.println(len);
  rb_heizung = rueck_signale.b_heizung;
  rb_ruf=rueck_signale.b_ruf;
}

void taster_auswerten(bool signal_1, bool signal_2, bool signal_3, int pin){
  if(signal_1 && !signal_2 && !signal_3){
    if(pin==rufton){   //Rufton
      digitalWrite(rufton, HIGH);
      s_rufton=true;
      t_ea=true;     
    }
    if(pin==heizung){   //Heizung
      digitalWrite(heizung, HIGH);
      s_heizung=true;
      h_ea=true;     
    }  
  }
  if(!signal_1 && signal_3){
    if(pin==rufton) t_ea=false;    //Rufton
    if(pin==heizung) h_ea=false;    //Heizung
  }
  if(signal_1 && !signal_2 && signal_3){
    if(pin==rufton){   //Rufton
      digitalWrite(rufton, LOW);
      s_rufton=false;
    } 
    if(pin==heizung){   //Heizung
      digitalWrite(heizung, LOW);
      s_heizung=false;
    } 
  }
  delay(200);  
}

void printIncomingReadings(){
  // Display Readings in Serial Monitor
  Serial.println("INCOMING READINGS");
  Serial.print("Heizung: ");
  Serial.print("Rufton: ");
  Serial.print(rb_ruf); 
}
 
void setup() {
  pinMode(rufton, OUTPUT);
  digitalWrite(rufton, LOW);  
  pinMode(heizung, OUTPUT);
  digitalWrite(heizung, LOW);  
  pinMode(heiz_taster, INPUT);  
  digitalWrite(heiz_taster, LOW);
  // Init Serial Monitor
  Serial.begin(115200);

  // Setze das Gerät als Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialisiere ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // ESP-NOW Rolle setzen
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
//
//Hole die Tasterstellungen
//
  h_signal=digitalRead(heiz_taster); // Signal-Taster Heizung abfragen
  s_heizung=digitalRead(heizung);     // Status Anzeige Signal-Taster Heizung abfragen: gedrückt/nicht gedrückt
  taster_auswerten(h_signal, h_ea, s_heizung, heizung); //Taster auswerten für Status Heiz-Kontrollampe und Sendesignal
  t_signal=digitalRead(ruf_taster); // Signal-Taster Rufen abfragen
  s_rufton=digitalRead(rufton);     // Status Anzeige Signal-Taster Rufen abfragen: gedrückt/nicht gedrückt
  taster_auswerten(t_signal, t_ea, s_rufton, rufton); //Taster auswerten für Status Ruf-Kontrollampe und Sendesignal  
//    
//Setze die Werte zum Versenden
//
    signale.b_heizung = digitalRead(heizung); 
    signale.b_ruf= digitalRead(rufton);
//
// Versende via ESP-NOW
//
    esp_now_send(broadcastAddress, (uint8_t *) &signale, sizeof(signale));

    // Print incoming readings
//    printIncomingReadings();
}
