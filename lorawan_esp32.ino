#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <CayenneLPP.h>
#include <LiquidCrystal_I2C.h>
#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif
// Delay time when showing the icons
#define time_ms 3000
// Create the lcd object address 0x3F and 16 columns x 2 rows
LiquidCrystal_I2C lcd (0x27, 16, 2);
const int INPUTPRESIONR = 34; //seleeciona el pin de la entrada analoga fase "R".
const int INPUTPRESIONS = 32; //seleeciona el pin de la entrada analoga fase "S".
const int INPUTPRESIONT = 25; //seleeciona el pin de la entrada analoga fase "T".
float valuePressureR;
float valuePressureS;
float valuePressureT; 
const int baudRate = 115200; //constant integer to set the baud rate for serial monitor


uint8_t hand[8][8] = {
  {0x0, 0x19, 0x1B, 0x1B, 0x1B, 0x19, 0x18, 0x18},
  {0x0, 0x1E, 0x1E, 0x0, 0x1C, 0x1E, 0x6, 0x6},
  {0x0, 0xE, 0x1B, 0x1B, 0x1B, 0x1F, 0x1F, 0x1B},                        
  {0x1B, 0x1B, 0x0, 0x1, 0x1, 0x1, 0x1, 0x1},
  {0x1E, 0x1C, 0x0, 0x1B, 0xA, 0x1B, 0x12, 0xB},
  {0x1B, 0x1B, 0x0, 0x17, 0x5, 0x17, 0x4, 0x14},
  {0x0, 0x0, 0xF, 0x9, 0xF, 0xA, 0x9, 0x0},  //letra R 
  {0x0, 0x0, 0xF, 0x6, 0x6, 0x6, 0x6, 0x0},  // letra T    
};

uint8_t NWSK[16] = {0x6F, 0xC5, 0xDE, 0x16, 0x0B, 0x05, 0xA3, 0x90, 0x30, 0xB8, 0x5D, 0x9A, 0xBD, 0x18, 0x87, 0x88}; //msb
uint8_t APPSKEY[16] = {0x46, 0x34, 0xAB, 0xC7, 0xA0, 0x28, 0xCB, 0xC9, 0xF3, 0x0D, 0x58, 0x42, 0x15, 0xA3, 0x98, 0x21}; //msb
uint32_t DEVADDR = 0x260C6FA7;

CayenneLPP lpp(50); //Buffer de 50

void os_getArtEui(u1_t* buf) {}
void os_getDevEui(u1_t* buf) {}
void os_getDevKey(u1_t* buf) {}
float getValuePressure(int pin);
const unsigned TX_INTERVAL = 10;
unsigned long previousMillis = 0;

const lmic_pinmap lmic_pins = {
  .nss = 5,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 15,
  .dio = { 2, 12, 14}, // no pongo el 14 que va a d0i2 y d14 va a d0i1
};

void onEvent(ev_t ev) {
  switch (ev) {
    case EV_TXCOMPLETE:
      Serial.println("[LMIC] Radio TX complete");
      break;
    default:
      Serial.println("Event unknown");
      break;
  }
}
void printIcon() {

  lcd.home();
  lcd.printByte(0);
  lcd.printByte(1);
  lcd.printByte(2);
  lcd.setCursor(3, 0);
  lcd.print("|");
  lcd.setCursor(0, 1);
  lcd.printByte(3);
  lcd.printByte(4);
  lcd.printByte(5);
  lcd.setCursor(3, 1);
  lcd.print("|");
  lcd.setCursor(8, 0);
  lcd.printByte(6); //letra R
  lcd.setCursor(8, 1);
  lcd.printByte(7); // letra T
  lcd.setCursor(14, 0);
  lcd.print("s");
}

void setup() {
  lcd.init();
  // Turn on the backlight on LCD.
  lcd. backlight ();
  for (int i = 0; i < sizeof(hand) / 8; i++) {
    lcd.createChar(i, hand[i]);
  }
  printIcon();
  //lcd.setCursor(4, 0);
  //lcd. print ( "PRESION SF6" );
  Serial.begin(baudRate); //initializes serial communication at set baud rate bits per second
  Serial.println("[INFO] Starting Serial");
  os_init();
  LMIC_reset();
  LMIC_setSession(0x1, DEVADDR, NWSK, APPSKEY);
  for (int chan = 0; chan < 72; ++chan) {
    LMIC_disableChannel(chan);
  }
  //904.3 Mhz banda de 915 Mhz 902-928 Mhz
  LMIC_enableChannel(10); //Habilitando el canal 10
  LMIC_setLinkCheckMode(0);
  LMIC_setDrTxpow(DR_SF7, 20); //Spreading Factor
  previousMillis = millis();
}

void loop() {
  if (millis() > previousMillis + (TX_INTERVAL * 1000)) {
    getInfoAndSend();
    previousMillis = millis();
    lpp.reset();
  }
  os_runloop_once(); //Proceso del sistema operativo real time
}

void sendData(uint8_t *mydata, uint16_t len) {
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.print("[LMIC] OP_TXRX, not sending"); //Pendiente un Tx o Rx
  } else {
    LMIC_setTxData2(1, mydata, len, 0);
  }
}

void getInfoAndSend() {
  valuePressureR = getValuePressure(INPUTPRESIONR);
  valuePressureS = getValuePressure(INPUTPRESIONS);
  valuePressureT = getValuePressure(INPUTPRESIONT);
  lcd.setCursor(4, 0);
  lcd.print(valuePressureR);
  lcd.setCursor(10, 0);
  lcd.print(valuePressureS);
  lcd.setCursor(4, 1);
  lcd.print(valuePressureT);
  lcd.setCursor(10, 1);
  lcd.print("BARES");
  delay(100);
  Serial.print("Sending packet: ");
  Serial.print(valuePressureR);
  Serial.print(", ");
  Serial.println(valuePressureS);  
  lpp.addAnalogInput(2, valuePressureR);
  lpp.addAnalogInput(3, valuePressureS);
  lpp.addAnalogInput(4, valuePressureT);
  sendData(lpp.getBuffer(), lpp.getSize());
}

float getValuePressure(int pin){
//Leer sensores y transmitir
  float value = analogRead(pin);
  float pressure = ((value * 0.002417) - 1.09);
  if (pressure >= 0) {
    pressure = ((value * 0.002417) - 1.09);
  }
  else {
    //reads value from input pin and assigns to variable
    pressure = 0000;
  }  
  return pressure;
}
