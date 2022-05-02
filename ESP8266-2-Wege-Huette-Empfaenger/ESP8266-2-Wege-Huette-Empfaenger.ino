#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <TinyGPSPlus.h> 
TinyGPSPlus gps;          
#include <SoftwareSerial.h> 
#define S_RX    14                // Definiere Software Serial RX pin D5, hier GPS-Modul anschließen
#define S_TX    12                // Definiere Software Serial TX pin D6
SoftwareSerial SoftSerial(S_RX, S_TX);    // SoftSerial library konfigurieren
uint8_t letzte_sekunde=61, stunde;
bool wzsz=true;
uint8_t zs=1;
   
#include <LiquidCrystal_I2C.h> // Vorher hinzugefügte LiquidCrystal_I2C Bibliothek einbinden
LiquidCrystal_I2C lcd(0x27, 20, 4); // LCD-Display mit jeweils 20 Zeichen in 4 Zeilen und der HEX-Adresse 0x27. 

// MAC-Adresse des Senders im Haus für den Empfänger
uint8_t broadcastAddress[] = {0xAC, 0x0B, 0xFB, 0xD0, 0x72, 0xF7};

bool b_heizung=LOW, b_ruf=LOW, radio_ein=false, radio_status=false, heizung_ein=false, heiz_status = false;
int rel_an=B00000000, rel_aus=B00001111, rel_set=rel_aus;
const int tmax=19, rufton=14;
byte taster_rh=0;

const int DHTPIN=13;
#define DHTTYPE    DHT22   
DHT dht(DHTPIN, DHTTYPE);
float t = 0.0;
float h = 0.0;
uint32_t i_i=0;
const uint32_t interval = 300;  

//Struktur, um die Tastersignale zu speichern
//Sender- und Empfängerstruktur müssen übereinstimmen
typedef struct struct_signale {
    bool b_heizung;
    bool b_ruf;
} struct_signale;

// Eine Signalstruktur zur Übermittlung der Eingangssignale definieren
struct_signale signale;
struct_signale rueck_signale;

// Rückmeldung, wenn Daten erfolgreich verschickt wurden
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
//  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
//    Serial.println("Erfolgreich versendet");
  }
  else{
//    Serial.println("Delivery fail");
  }
}

// Rückmeldung über empfangene Daten
void OnDataRecv(uint8_t * mac, uint8_t *eingangssignale, uint8_t len) {
  memcpy(&signale, eingangssignale, sizeof(eingangssignale));
//  Serial.print("Bytes received: ");
//  Serial.println(len);
  b_heizung = signale.b_heizung;
  b_ruf=signale.b_ruf;
}

void setup() {
  Wire.begin();
  Wire.beginTransmission(0x20);
  Wire.write(0x00);   //IO A-Register
  Wire.write(0x00);   //Alle Ausgänge auf Output setzen
  Wire.endTransmission();
  Wire.beginTransmission(0x20);
  Wire.write(0x12);   //A-Register auswählen
  Wire.write(rel_aus);   //Die ersten vier Ausgänge auf HIGH="Relais aus" setzen
  Wire.endTransmission(); 
  pinMode(rufton, OUTPUT);   
  lcd.init(); //Im Setup wird der LCD gestartet 
  lcd.backlight(); 
  lcd.setCursor(0, 2);
  lcd.print("Zeit:  :  :");  
  lcd.setCursor(0, 3);
  lcd.print("Datum:  /  /");  
  lcd.print("Alarmzeit: "+std+ std);      
      
  Serial.begin(115200);
  SoftSerial.begin(9600);  
  dht.begin();
  // Modul als Wifi-Station setzen
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialisiere ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Setze ESP-NOW Rolle
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Register für den Sendestatus, der
  // übermittelten Daten
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  // Register für den Rückrufempfang, das aufgerufen wird, sobald Daten empfangen wurden
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
//
// Versende via ESP-NOW
//
  esp_now_send(broadcastAddress, (uint8_t *) &rueck_signale, sizeof(rueck_signale));
  Wire.beginTransmission(0x20);
  Wire.write(0x13);     //Adressierung auf Register B setzen
  Wire.endTransmission();
  Wire.requestFrom(0x20, 1);
  taster_rh=Wire.read();
  if (bitRead(taster_rh, 7) == 1) radio_ein=true;
  if (bitRead(taster_rh, 6) == 1 && !heiz_status) heizung_ein = true;
  if (bitRead(taster_rh, 6) == 1 && heiz_status) heizung_ein = false;
  delay(300);

  i_i++; // Alle dreihundert Durchläufe Temperatur und Feuchtigkeit messen
  if (i_i >= interval) {
    // Setze den Zähler zurück
      i_i=0;
    // Auslesen der Temperatur in Celsius (normal)
    float newT = dht.readTemperature();

    // Wenn kein Auslesen möglich war, den alten Wert behalten
    if (isnan(newT)) {
      Serial.println("Lesefehler vom DHT-Sensor!"); //Hier LED-für Fehler
      
    }
    else {
      t = newT;
    }
    // Auslesen der Luftfeuchtigkeit
    float newH = dht.readHumidity();
    // Falls kein Auslesen möglich war, den alten Wert behalten
    if (isnan(newH)) {
      Serial.println("Lesefehlter vom DHT-Sensor!");
    }
    else {
      h = newH;
    }
    lcd.setCursor(7, 0);
    lcd.print("  "); 
    lcd.setCursor(0, 0);
    lcd.print("Temp.:"); 
    lcd.setCursor(7, 0);
    lcd.print(t);
    lcd.setCursor(11, 0);
    lcd.print("  ");
    lcd.setCursor(13, 0);
    lcd.print("C");  
    lcd.setCursor(8, 1);
    lcd.print("  ");           
    lcd.setCursor(0, 1);
    lcd.print("Luftf.:"); 
    lcd.setCursor(8, 1);
    lcd.print(h);  
    lcd.setCursor(14, 1);
    lcd.print("%");          
  }
  signale_auswerten(); 
  while (SoftSerial.available() > 0) {
    if (gps.encode(SoftSerial.read())) {
      if(letzte_sekunde != gps.time.second()) {
        letzte_sekunde = gps.time.second();
        if (gps.time.isValid()) {
          zs=1;                         // Stundenkorrektur für Winterzeit
          if(!wzsz) zs=2;               // Stundenkorrektur für Sommerzeit
          stunde=gps.time.hour()+zs;
          if(stunde>23){                // Stunde für richtige Darstellung
            stunde=stunde-24;           // korrigieren. Tag, Monat und Jahr werden nicht
          }                             // verändert, sodass maximal 2 Stunden das alte 
          lcd_anzeige(stunde, 5, 2);    // Datum angezeigt wird.
          lcd.setCursor(7, 2);
          lcd.print(":");  
          lcd_anzeige(gps.time.minute(), 8, 2);
          lcd_anzeige(gps.time.second(), 11, 2);    
          lcd.setCursor(14,2);
          if(wzsz) lcd.print("Wz");     // Winterzeit
          if(!wzsz) lcd.print("Sz");    // Sommerzeit
        }  
        if (gps.date.isValid()) {  
          lcd_anzeige(gps.date.day(), 6, 3);      
          lcd_anzeige(gps.date.month(), 9, 3);
          lcd.setCursor(12, 3);
          lcd.print(gps.date.year());
        }          
      }
    }
  }  
}

void signale_auswerten(){
  if(b_heizung) heizung_ein = true;
  b_heizung = false;
  if(heizung_ein) {
    heiz_status = true;
    bitWrite(rel_set, 4, 1);   //Kontrolllampe für aktivierte Heizung einschalten 
  }
  if(t<tmax && heiz_status){   //Heizung nur einschalten, wenn die Temperatur unter 19 Grad liegt
    bitWrite(rel_set, 0, 0);  // Heizung einschalten, Bit für den ersten A-Ausgang setzen, 
    lcd.setCursor(15, 0);     // Relais ist Low-aktiv
    lcd.print("H-akt");
  }
  if(t >= tmax || !heiz_status) {    // Ist die Temperatur zu hoch oder die Heizung nicht aktiviert, Heizung ausschalten
    bitWrite(rel_set, 0, 1);  // Heizung ausschalten, Bit für den ersten A-Ausgang setzen, 
    lcd.setCursor(15, 0);     // Relais ist Low-aktiv
    lcd.print("H-aus");
  }
  if(!heizung_ein && heiz_status) {
      b_heizung = false;
      heiz_status = false;
      bitWrite(rel_set, 0, 1);  // Heizung ausschalten
      lcd.setCursor(15, 0);
      lcd.print("H-aus");
      bitWrite(rel_set, 4, 0);   //Kontrolllampe für aktivierte Heizung ausschalten 
  }

  if(b_ruf) { // Beim Rufen Radio ausschalten
    bitWrite(rel_set, 1, 1);  
    bitWrite(rel_set, 5, 0);    // Kontrollampe für Radio ausschalten
  }

  if(radio_ein && !radio_status) { // Radio einschalten
    bitWrite(rel_set, 1, 0); 
    radio_status = true;
    radio_ein=false; 
    b_ruf=false;
    bitWrite(rel_set, 5, 1);    // Kontrolllampe für Radio einschalten
  }
  if (radio_ein && radio_status) { // Radio ausschalten
      bitWrite(rel_set, 1, 1);
      radio_ein = false;
      radio_status = false;
      bitWrite(rel_set, 5, 0);  // Kontrollampe für Radio ausschalten
  }

  Wire.beginTransmission(0x20);
  Wire.write(0x12);   //Adresseriung für Register A setzen
  Wire.write(rel_set);   //Die ersten vier Ausgänge entsprechend schalten
  Wire.endTransmission();  
} 

void lcd_anzeige(uint8_t wert, uint8_t x, uint8_t y){
         if(wert<10){
          lcd.setCursor(x, y);
          lcd.print("0");
          lcd.setCursor(x+1, y);
          lcd.print(wert);
        }
        else{
          lcd.setCursor(x, y);
          lcd.print(wert);          
        }    
}      
