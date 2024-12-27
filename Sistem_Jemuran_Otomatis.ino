#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <Firebase_ESP_Client.h>
#include <TimeLib.h>

#define ssid "Samsung"
#define password "12345678"
#define auth_firebase "AIzaSyBadqy3o9b-9f-Dj6TPtRKHDFnBmlfiVbA"
#define host_firebase "https://mobile-project-2-71549-default-rtdb.firebaseio.com/"

FirebaseData firebaseData;                              
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig; 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800, 60000);

Servo servoMotor;
Servo servoMotor2;
int servoPin = 32;
int servo2Pin = 33;
int rainSensor1 = 5;
int rainSensor2 = 18;
int ldrPin1 = 34;
int ldrPin2 = 35;
int dhtPin = 19;

int batasCahaya = 3000;

DHT dht(dhtPin, DHT11);

unsigned long waktuTerakhir = 0;
const long interval = 120000;

bool hujanSebelumnya = false; 
unsigned long waktuHujanTerdeteksi = 0;

int jemuranAktif = 0;
int posisiServo = 0;

void connectWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi!");
}

void reconnect() {
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print("Mencoba untuk terhubung kembali...");
        connectWiFi(); 
        delay(1000);
    }
}

void setMotorServo(int posisi){
  servoMotor.write(posisi);
  servoMotor2.write(posisi);
  posisiServo = posisi;
}

void setup() {
    Serial.begin(115200);
    connectWiFi();

    firebaseConfig.api_key = auth_firebase;
    firebaseConfig.database_url = host_firebase;

    if (Firebase.signUp(&firebaseConfig, &firebaseAuth, "", "")){
      Serial.println("ok");
    } else{
      Serial.printf("%s\n",firebaseConfig.signer.signupError.message.c_str());
    }

    timeClient.begin();
    dht.begin();
    servoMotor.attach(servoPin);
    servoMotor2.attach(servo2Pin);
    pinMode(rainSensor1, INPUT);
    pinMode(rainSensor2, INPUT);
    pinMode(ldrPin1, INPUT);
    pinMode(ldrPin2, INPUT);
    setMotorServo(0);
    Firebase.begin(&firebaseConfig, &firebaseAuth);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
      reconnect();  
  }

  timeClient.update(); 

  int nilaiSensorHujan1 = digitalRead(rainSensor1);
  int nilaiSensorHujan2 = digitalRead(rainSensor2);
  int nilaiSensorCahaya1 = analogRead(ldrPin1);
  int nilaiSensorCahaya2 = analogRead(ldrPin2);
  unsigned long epochTime = timeClient.getEpochTime();  
  setTime(epochTime);
  int tahun = year();  
  int bulan = month(); 
  int tanggal = day();     
  int jam = hour();   
  int menit = minute();
  int detik = second();

  float temp = dht.readTemperature();
  float kelembaban = dht.readHumidity();
  
  if (isnan(temp) || isnan(kelembaban)) {
    Serial.println("Gagal membaca data dari sensor DHT22");
  }

  Serial.print("Hujan: ");
  Serial.println(nilaiSensorHujan1 == LOW || nilaiSensorHujan2 == LOW ? "Terdeteksi" : "Tidak terdeteksi");

  Serial.print("Cahaya1: ");
  Serial.println(nilaiSensorCahaya1);
  Serial.print("Cahaya2: ");
  Serial.println(nilaiSensorCahaya2);

  Serial.print("Waktu: ");
  Serial.print(jam);
  Serial.print(":");
  if (menit < 10){ 
    Serial.print("0");
  }
  Serial.print(menit);
  Serial.print(":");
  if (detik < 10) {
    Serial.print("0"); 
  }
  Serial.println(detik);

  Serial.print("Suhu: ");
  Serial.print(temp);
  Serial.println(" Derajat Celcius");

  Serial.print("Kelembaban: ");
  Serial.print(kelembaban);
  Serial.println(" %");

  if (Firebase.RTDB.getInt(&firebaseData, "/status")) {
      jemuranAktif = firebaseData.intData();
  } else {
      Serial.print("Error membaca status jemuran: ");
      Serial.println(firebaseData.errorReason());
  }

  bool waktuSiang = (jam >= 6 && jam < 18);
  bool waktuMalam = (jam >= 18 || jam < 6);
  bool hujan = (nilaiSensorHujan1 == LOW && nilaiSensorHujan2 == LOW) ;
  bool cuacaGelap = (nilaiSensorCahaya1 > batasCahaya && nilaiSensorCahaya2 > batasCahaya);  
 
  if (hujan && !hujanSebelumnya){
    waktuHujanTerdeteksi = epochTime;
  }
  hujanSebelumnya = hujan;

  String statusJemuran = "";

  if(jemuranAktif == 1){
    if(hujan){
      if(posisiServo != 180){
        setMotorServo(180);
        posisiServo = 180;
      }
      statusJemuran = "Tidak Menjemur";
    } else if(cuacaGelap && waktuSiang){
      if(posisiServo != 180){
        setMotorServo(180);
        posisiServo = 180;
      }
      statusJemuran = "Tidak Menjemur";
    } 
     else if(waktuMalam){
      if(posisiServo != 180){
        setMotorServo(180);
        posisiServo = 180;
      }
      statusJemuran = "Tidak Menjemur";
    } 
  }else{
    if(posisiServo != 180){
        setMotorServo(180);
        posisiServo = 180;
      }
    statusJemuran = "Jemuran Tidak Aktif";
  }

  Serial.print("Status Jemuran: ");
  Serial.println(statusJemuran);
  Serial.println("-------------------------------"); 
  Serial.println();

  unsigned long currentMillis = millis();

  if(currentMillis - waktuTerakhir >= interval){
    waktuTerakhir = currentMillis;

    String waktu = String(tahun) + "-" + String(bulan) + "-" + String(tanggal) + "_" + (jam < 10 ? "0" : "") + String(jam) + ":" + (menit < 10 ? "0" : "") + String(menit) + ":" + (detik < 10 ? "0" : "") + String(detik);
    String path = "/DATA_JEMURAN/" + waktu;
    Firebase.RTDB.setBool(&firebaseData, path + "/Hujan", hujan);
    Firebase.RTDB.setInt(&firebaseData, path + "/Cahaya1", nilaiSensorCahaya1);
    Firebase.RTDB.setInt(&firebaseData, path + "/Cahaya2", nilaiSensorCahaya2);
    Firebase.RTDB.setString(&firebaseData, path + "/Waktu", waktu);
    Firebase.RTDB.setFloat(&firebaseData, path + "/Suhu", temp);
    Firebase.RTDB.setFloat(&firebaseData, path + "/Kelembaban", kelembaban);
    Firebase.RTDB.setString(&firebaseData, path + "/Status_Jemuran", statusJemuran);

    if (waktuHujanTerdeteksi > 0) {
      String waktuHujan = String(year(waktuHujanTerdeteksi)) + "-" + String(month(waktuHujanTerdeteksi)) + "-" + String(day(waktuHujanTerdeteksi)) + "_" + (hour(waktuHujanTerdeteksi) < 10 ? "0" : "") + String(hour(waktuHujanTerdeteksi)) + ":" + (minute(waktuHujanTerdeteksi) < 10 ? "0" : "") + String(minute(waktuHujanTerdeteksi)) + ":" + (second(waktuHujanTerdeteksi) < 10 ? "0" : "") + String(second(waktuHujanTerdeteksi));
      Firebase.RTDB.setString(&firebaseData, path + "/Waktu_Hujan", waktuHujan);
    }
    waktuHujanTerdeteksi = 0;

    if (firebaseData.errorReason() != "") {
      Serial.println("Gagal mengirim data ke firebase: " + firebaseData.errorReason());
    }
  }
  delay(2000);
}
