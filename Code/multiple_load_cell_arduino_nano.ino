#include <HX711_ADC.h>
#include <LiquidCrystal_I2C.h>
#include <Pushbutton.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

//Button
#define BUTTON_PIN 8
Pushbutton button(BUTTON_PIN);

// pIN Hx711
const int HX711_dout_1 = 4; //mcu > HX711 no 1 dout pin
const int HX711_sck_1 = 5; //mcu > HX711 sck pin
const int HX711_dout_2 = 2; //mcu > HX711 no 2 dout pin
const int HX711_dout_3 = 6; //mcu > HX711 no 3 dout pin
const int HX711_dout_4 = 3; //mcu > HX711 no 4 dout pin

// PIN HX711 (dout pin, sck pin)
HX711_ADC LoadCell_1(HX711_dout_1, HX711_sck_1); //HX711 1
HX711_ADC LoadCell_2(HX711_dout_2, HX711_sck_1); //HX711 2
HX711_ADC LoadCell_3(HX711_dout_3, HX711_sck_1); //HX711 3
HX711_ADC LoadCell_4(HX711_dout_4, HX711_sck_1); //HX711 3

unsigned long t = 0;

void setup() {
  Serial.begin(57600);
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  // Inisialisasi LCD 16x02
  lcd.init();
  lcd.clear();    
  lcd.backlight();
  delay(500);
  lcd.setCursor(4, 0);
  lcd.print("Memulai");
  lcd.setCursor(3, 1);
  lcd.print("Startup...");
  delay(1000);
  lcd.clear();

  float calibrationValue_1; 
  float calibrationValue_2; 
  float calibrationValue_3;
  float calibrationValue_4;

  // ganti sesuai dengan kalibrasi tiap sensor
  calibrationValue_1 = -870.92;
  calibrationValue_2 = -920.32;
  calibrationValue_3 = -765.57;
  calibrationValue_4 = -784.43;

  // inisialisasi sensor load cell
  LoadCell_1.begin();
  LoadCell_2.begin();
  LoadCell_3.begin();
  LoadCell_4.begin();

  // Menset ke zero dan stabilisasi pembacaan
  unsigned long stabilizingtime = 2000; 
  boolean _tare = true;
  byte loadcell_1_rdy = 0;
  byte loadcell_2_rdy = 0;
  byte loadcell_3_rdy = 0;
  byte loadcell_4_rdy = 0;
  while ((loadcell_1_rdy + loadcell_2_rdy + loadcell_3_rdy + + loadcell_4_rdy) < 4) {
    if (!loadcell_1_rdy) loadcell_1_rdy = LoadCell_1.startMultiple(stabilizingtime, _tare);
    if (!loadcell_2_rdy) loadcell_2_rdy = LoadCell_2.startMultiple(stabilizingtime, _tare);
    if (!loadcell_3_rdy) loadcell_3_rdy = LoadCell_3.startMultiple(stabilizingtime, _tare);
    if (!loadcell_4_rdy) loadcell_4_rdy = LoadCell_4.startMultiple(stabilizingtime, _tare);
  }
  if (LoadCell_1.getTareTimeoutFlag() || LoadCell_2.getTareTimeoutFlag() || LoadCell_3.getTareTimeoutFlag() || LoadCell_4.getTareTimeoutFlag()) {
    Serial.println("Waktu habis, cek koneksi MCU>HX711 dan desain pin");
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Error: cek");
    lcd.setCursor(2, 1);
    lcd.print("wiring HX711!");
    delay(5000); 
  }

  // set nilai kalibrasi
  LoadCell_1.setCalFactor(calibrationValue_1);
  LoadCell_2.setCalFactor(calibrationValue_2);
  LoadCell_3.setCalFactor(calibrationValue_3);
  LoadCell_4.setCalFactor(calibrationValue_4);
  Serial.println("Startup selesai");

  // tampilkan pesan pengecekan selesai
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Startup");
  lcd.setCursor(4, 1);
  lcd.print("selesai");
  delay(1000);
  lcd.clear();
}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  if (button.getSingleDebouncedPress()){
    Serial.print("tare...");
    Serial.print("");
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Melakukan");
    lcd.setCursor(4, 1);
    lcd.print("tare...");
    delay(1000);
    lcd.clear();
    LoadCell_1.tareNoDelay();
    LoadCell_2.tareNoDelay();
    LoadCell_3.tareNoDelay();
    LoadCell_4.tareNoDelay();
  }

  // Cek data baru
  if (LoadCell_1.update()) newDataReady = true;
  LoadCell_2.update();
  LoadCell_3.update();
  LoadCell_4.update();

  // Smoothing data 
  if ((newDataReady)) {
    if (millis() > t + serialPrintInterval) {
      float a = LoadCell_1.getData();
      float b = LoadCell_2.getData();
      float c = LoadCell_3.getData();
      float d = LoadCell_4.getData();
      float avg = (a + b + c + d)/4.0;
      Serial.print("Load_cell 1 : ");
      Serial.print(a);
      Serial.print("\tLoad_cell 2 : ");
      Serial.print(b);
      Serial.print("\tLoad_cell 3 : ");
      Serial.print(c);
      Serial.print("\tLoad_cell 4 : ");
      Serial.print(d);
      Serial.print("\tAVG : ");
      Serial.println(avg);

      lcd.setCursor(0, 0);
      lcd.print("Berat: ");
      //avg = max(avg,0.0)
      if (avg <= 0.5) {
        avg = 0.0;
      }
      
      lcd.setCursor(0, 1);
      lcd.print(String(avg) + "   ");
      lcd.setCursor(14,1);
      lcd.print("kg");

      newDataReady = 0;
      t = millis();
    }
  }

  // kirim perintah 't' di serial command untuk memulai tare:
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') {
      LoadCell_1.tareNoDelay();
      LoadCell_2.tareNoDelay();
      LoadCell_3.tareNoDelay();
      LoadCell_4.tareNoDelay();
    }
  }

  if (LoadCell_1.getTareStatus() == true) {
    Serial.println("Tare load cell 1 complete");
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("tare loadcell");
    lcd.setCursor(3, 1);
    lcd.print("1 sukses!");
    delay(1000);
    lcd.clear();
  }
  if (LoadCell_2.getTareStatus() == true) {
    Serial.println("Tare load cell 2 complete");
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("tare loadcell");
    lcd.setCursor(3, 1);
    lcd.print("2 sukses!");
    delay(1000);
    lcd.clear();
  }
  if (LoadCell_3.getTareStatus() == true) {
    Serial.println("Tare load cell 3 complete");
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("tare loadcell");
    lcd.setCursor(3, 1);
    lcd.print("3 sukses!");
    delay(1000);
    lcd.clear();
  }
  if (LoadCell_4.getTareStatus() == true) {
    Serial.println("Tare load cell 4 complete");
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("tare loadcell");
    lcd.setCursor(3, 1);
    lcd.print("4 sukses!");
    delay(1000);
    lcd.clear();
  }
}