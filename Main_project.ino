#include <SoftwareSerial.h>
SoftwareSerial mySerial(8, 9);             // Выводы SIM800L Tx & Rx подключены к выводам Arduino 8 и 9
char incomingByte;
String inputString;

int LED = 13;                           // Вывод управления включением/выключением света


#include <DS3231.h>                    // Подключаем библиотеку Wire  
DS3231  rtc(SDA, SCL);
int intVremy;
String strVremy;
String dattomarus = "No watering";

#include <Wire.h>
#include <LiquidCrystal_I2C.h> // Подключение библиотеки
LiquidCrystal_I2C lcd(0x27, 16, 2); // Указываем I2C адрес

#include <OneWire.h>                     //Библиотека для датчика температруы воды
#include <DallasTemperature.h>   //Библиотека для датчика температруы воды

#include <dht11.h>                          //Библиотека для датчика температуры и влажности воздуха(DHT11)
dht11 temphum;                                    //Объявление переменной класса dht11

OneWire oneWire(6);                         //Порт подключения датчика температуры воды
DallasTemperature ds(&oneWire);             //Температура воды

//Проветривание
#define DHT11PIN     3                       //Объявление названия датчика температуры и влажности воздуха в теплице и его порта

#define SPEED_1      5                      //Скорость вращения двигателя
#define DIR_1        4                      //Направление вращения двигателя

//Полив
#define WATERV       A2                   //Объявление верхнего датчика уровня воды и его порта
#define WATERN       A3                   //Объявление нижнего датчика уровня воды и его порта
#define POMPA        10                      //Объявление помпы и её порта
#define SOLENOID     2                     //Объявление соленоида и его порта
#define  VLPOCHV1      A0                    //Объявление датчика влажности почвы и его порта
#define  VLPOCHV2      A1                    //Объявление датчика влажности почвы и его порта
// Датчик инверсный: больше влажность - меньше значение.
#define  MIN         630                     // Определение минимального показания датчика (в воздухе)
#define MAX          330                    // Определение максимального показания датчика (в воде)
uint16_t sensorpochv1;                              // Создание переменной для хранения адаптированных показаний
uint16_t sensorpochv2;                              // Создание переменной для хранения адаптированных показаний

//Ночная подсветка
#define PIR          11                        //Объявление названия датчика движения и его порта
#define LED          13                     //Объявление названия ночной подсветки в теплице и её порта
#define SVET         12                    //Объявление названия датчика света и его порта

boolean WindowOpen = false;  //Переменная состояния окна
int grotkrwind = 30;
boolean waterSolenoidOn = false;       //Переменная состояния соленоида
boolean flgManualSvet = false;
boolean flgERRHiTemp = false;

int chk;                                //Переменная для считывания показаний с датчика температуры и влажности воздуха

int waterv ;                                //Объявление названия верхнего датчика уровня воды и его порта
int watern ;                    //Объявление названия нижнего датчика уровнгя воды и его порта

int itogpochv;

int pirVal  ;                               //Переменная для считывания показаний с датчика движения
void setup() {
  Serial.begin(9600);                             //Инициализация монитора порта
  ds.begin();                                            //Инициализация датчика ds18b20 температуры воды
  pinMode(WATERV, INPUT);              //Объявление датчика уровня воды на вход
  pinMode(WATERN, INPUT);              //Объявление датчика уровня воды на вход
  pinMode(PIR, INPUT);                        //Объявление датчика движения на вход
  pinMode(SVET, INPUT);                     //Объявление датчика света на вход
  pinMode(POMPA, OUTPUT);            //Объявление порта помпы на выход
  pinMode(SOLENOID, OUTPUT);       //Объявление порта соленоида на выход
  pinMode(LED, OUTPUT);                   //Объявление порта ночной подсветки на выход
  digitalWrite(POMPA, LOW);
  digitalWrite(SOLENOID, LOW);
  digitalWrite(LED, LOW);

  for (int i = 4; i < 5; i++) {
    pinMode(i, OUTPUT);                     //Настройка выводов платы 4, 5 на вывод сигналов
  }

  digitalWrite(DIR_1, LOW );                //Принудительный перевод привода в закрытое состояние
  Serial.println("Закрытие форточки");
  analogWrite(SPEED_1, 255);                 //Включение мотора
  WindowOpen = false;
  delay(7000);                                         //Ожидание закрытия окна
  analogWrite(SPEED_1, 0);                  //Выключение мотора

  lcd.init();                      // Инициализация дисплея
  lcd.backlight();                 // Подключение подсветки

  mySerial.begin(9600);
  while (!mySerial.available()) {           // Зацикливаем и ждем инициализацию SIM800L
    mySerial.println("AT");                  // Отправка команды AT
    delay(1000);                             // Пауза
    Serial.println("Connecting…");         // Печатаем текст
  }
  Serial.println("Connected!");            // Печатаем текст
  mySerial.println("AT+CMGF=1");           // Отправка команды AT+CMGF=1
  delay(1000);                             // Пауза
  mySerial.println("AT+CNMI=1,2,0,0,0");   // Отправка команды AT+CNMI=1,2,0,0,0
  delay(1000);                             // Пауза
  mySerial.println("AT+CMGL=\"REC UNREAD\"");

  rtc.begin();                         // Инициализировать rtc

  // Установка времени
  /*rtc.setTime(15, 33, 10);              //  Установить время 16:29:00 (формат 24 часа)
  rtc.setDate(27, 03, 2020);            //  Установить дату 31 августа 2018 года*/
}
void loop() {
  strVremy = rtc.getTimeStr();
  intVremy = 3600 * (strVremy.substring(0, 2).toInt()) + 60 * (strVremy.substring(3, 5).toInt()) + (strVremy.substring(6, 8).toInt());

  if (intVremy < 500) {
    flgManualSvet = false;
  }

  chk = temphum.read(DHT11PIN); // Чтение данных c датчика температуры и влажности воздуха

  ds.requestTemperatures();                 //Считывание показаний с датчика температуры воды

  sensorpochv1 = analogRead(VLPOCHV1);         //Считывание сырых данных с датчика влажности почвы
  sensorpochv1 = map(sensorpochv1, MIN, MAX, 0, 100);   //Адаптация значения от 0 до 100

  sensorpochv2 = analogRead(VLPOCHV2);         //Считывание сырых данных с датчика влажности почвы
  sensorpochv2 = map(sensorpochv2, MIN, MAX, 0, 100);   //Адаптация значения от 0 до 100

  itogpochv = (sensorpochv2 + sensorpochv1) / 2;

  waterv = analogRead(WATERV);        //Считывание значений с верхнего датчика уровня воды
  watern = analogRead(WATERN);         //Считывание значений с нижнего датчика уровня воды

  pirVal = digitalRead(PIR);                       //Считывание значений с датчика движения


  /* Serial.print("Температура воды=");
    Serial.print(ds.getTempCByIndex(0), 0);
    Serial.print("C;  ");
    Serial.print("Влажность почвы1=");
    Serial.print(sensorpochv1);
    Serial.print("  Влажность почвы2=");
    Serial.print(sensorpochv2);
    Serial.print("  Влажность почвыОбщая=");
    Serial.print(itogpochv);
    Serial.print(";   ");
    Serial.print("Верхний датчик=");
    Serial.print(waterv);
    Serial.print(";   ");
    Serial.print("Нижний датчик=");         //Вывод показаний всех датчиков в монитор порта
    Serial.print(watern);
    Serial.println(";   ");
    Serial.print("Температура воздуха=");
    Serial.print(temphum.temperature);
    Serial.print("C;  ");
    Serial.print("Влажность воздуха=");
    Serial.print(temphum.humidity);
    Serial.println("%;   ");*/

  lcd.setCursor(0, 0);
  lcd.print("Air:T,C=" );
  lcd.setCursor(8, 0);
  lcd.print(temphum.temperature );

  lcd.setCursor(11, 0);
  lcd.print("Hum,%=" );
  lcd.setCursor(17, 0);
  lcd.print(temphum.humidity );
  lcd.setCursor(0, 1);               //Вывод показаний датчик на дисплей
  lcd.print("WaterTemp,C=" );
  lcd.setCursor(12, 1);
  lcd.print(ds.getTempCByIndex(0), 0 );
  lcd.setCursor(0, 2);
  lcd.print("SoilMoisture,%=");
  lcd.setCursor(15, 2);
  lcd.print(itogpochv);
  lcd.setCursor(20, 3);
  lcd.print(rtc.getTimeStr());

  if (((watern > 700) or (waterv > 700)) and (ds.getTempCByIndex(0) > 30) and (itogpochv < 40 )) {
    digitalWrite(POMPA, HIGH);
    Serial.println("Полив включен");
    lcd.setCursor(20, 3);
    dattomarus = rtc.getDateStr();
    lcd.print("Watering On");
  }                                          //Цикл полива
  else {
    digitalWrite(POMPA, LOW);
    Serial.println("Полив вЫключен");
    //lcd.setCursor(20, 3);
    //lcd.print("                   ");
  }

  if (watern == 0 and !waterSolenoidOn) {
    waterSolenoidOn = true;
  }                                          //Цикл наполнения бочки
  if (waterv == 1) {
    waterSolenoidOn = false;
  }

  if (waterSolenoidOn) {
    digitalWrite(SOLENOID, HIGH);
  }                                          //Цикл наполнения бочки
  else {
    digitalWrite(SOLENOID, LOW);
  }

  if (temphum.temperature > grotkrwind and (WindowOpen == false)) {
    digitalWrite(DIR_1, HIGH);
    analogWrite(SPEED_1, 255);
    WindowOpen = true;                       //Цикл открывания окна
    delay(7000);
    analogWrite(SPEED_1, 0);
  }

  if (temphum.temperature <= grotkrwind and (WindowOpen == true)) {
    digitalWrite(DIR_1, LOW );
    analogWrite(SPEED_1, 255);
    WindowOpen = false;                      //Цикл закрывания окна
    delay(7000);
    analogWrite(SPEED_1, 0);
  }
  if (temphum.temperature >= 40) {
    if (flgERRHiTemp == false) {
      sms(String("Attention: High Temperature"), String("+7926*******"));
      flgERRHiTemp = true;
    }
  }
  else {
    flgERRHiTemp = false;
  }
  if ( pirVal == LOW and (flgManualSvet == true or ((strVremy.substring(0, 2).toInt() < 21 and strVremy.substring(0, 2).toInt() > 16) and digitalRead(SVET) == HIGH ))) {
    digitalWrite (LED, HIGH);
  }

  Serial.print (flgManualSvet);
  if (pirVal == HIGH or (digitalRead(SVET) == LOW and flgManualSvet == false )) {
    digitalWrite (LED, LOW);     // вЫключение подсветки
  }

  delay(50);

  if (mySerial.available()) {                // Проверяем, если есть доступные данные
    delay(100);                            // Пауза
    while (mySerial.available()) {          // Проверяем, есть ли еще данные.
      incomingByte = mySerial.read();         // Считываем байт и записываем в переменную incomingByte
      inputString += incomingByte;            // Записываем считанный байт в массив inputString
    }
    delay(10);                             // Пауза
    Serial.println(inputString);           // Отправка в "Мониторинг порта" считанные данные
    inputString.toUpperCase();             // Меняем все буквы на заглавные
    if (inputString.indexOf("LIGHT_ON") > -1) { // Проверяем полученные данные, если ON_1 включаем реле 1
      digitalWrite(LED, HIGH);
      flgManualSvet = true;
      //sms(String("LIGHT - ON"), String("+7926*******"));

    } // Отправка SMS
    if (inputString.indexOf("LIGHT_OFF") > -1) { // Проверяем полученные данные, если OFF_1 выклюем реле 1
      digitalWrite(LED, LOW);
      flgManualSvet = false;
      //sms(String("LIGHT - OFF"), String("+7926*******"));
    }// Отправка SMS
    delay(50);
    if (inputString.indexOf("INFO") > -1) {     // Проверяем полученные данные
      sms(String("AirTemp: " + String(temphum.temperature) + " *C " + " AirHum: " + String(temphum.humidity) + " % " + " WaterTemp " + String(ds.getTempCByIndex(0)) + " *C " + " SoilHum " + String(itogpochv) + " %"), String("+7926*******")); // Отправка SMS
    }
    if (inputString.indexOf("DEMO") > -1) {
      digitalWrite(DIR_1, HIGH);
      analogWrite(SPEED_1, 255);                       //Цикл открывания окна
      delay(7000);
      analogWrite(SPEED_1, 0);// Проверяем полученные данные
      delay(9000);
      digitalWrite(POMPA, HIGH);
      lcd.setCursor(20, 3);
      lcd.print("Watering On");
      Serial.println("Полив включен");
      lcd.setCursor(20, 3);
      lcd.print("Watering On");
      delay(2500);
      digitalWrite(POMPA,LOW);
      Serial.println("Полив вЫключен");
      lcd.setCursor(20, 3);
      lcd.print("                   ");
      delay(5500);
      digitalWrite (SOLENOID, HIGH);
      delay(2500);
      digitalWrite (SOLENOID, LOW);
      delay(2900);
      digitalWrite (LED, HIGH);
      delay(5000);
      digitalWrite (LED, LOW);
      delay(1300);
      digitalWrite(DIR_1, LOW );
      analogWrite(SPEED_1, 255);
      delay(7000);
      analogWrite(SPEED_1, 0);
    }

    if (inputString.indexOf("WIND_OFF") > -1) { //Закрытие окна
      digitalWrite(DIR_1, LOW);
      analogWrite(SPEED_1, 255);                       //Цикл закрывания окна
      delay(7000);
      analogWrite(SPEED_1, 0);
    }

    if (inputString.indexOf("WIND_ON") > -1) { //Открытие окна
      digitalWrite(DIR_1, HIGH);
      analogWrite(SPEED_1, 255);                       //Цикл открывания окна
      delay(7000);
      analogWrite(SPEED_1, 0);
    }

    if (inputString.indexOf("T20") > -1) { 
      grotkrwind = 20;
    }

    if (inputString.indexOf("T25") > -1) { 
      grotkrwind = 25;
    }

    if (inputString.indexOf("T30") > -1) {
      grotkrwind = 30;
    }

    if (inputString.indexOf("T35") > -1) { 
      grotkrwind = 35;
    }

    if (inputString.indexOf("T40") > -1) { 
      grotkrwind = 40;
    }

    if (inputString.indexOf("WATER_ON") > -1) { //Включение полива на одну секунду\минуту(ориг.)
      digitalWrite(POMPA, HIGH);
      delay(1000);
      digitalWrite(POMPA, LOW);
    }

    if (inputString.indexOf("SOIL_HUM") > -1) { //Влажность почвы
      sms(String("SoilHum " + String(itogpochv) + " %"), String("+7926*******"));
    }

    if (inputString.indexOf("TEMPHUMA") > -1) { //Температура и влажность воздуха
      sms(String("AirTemp: " + String(temphum.temperature) + " *C " + " AirHum: " + String(temphum.humidity) + " % "), String("+7926*******"));
    }

    if (inputString.indexOf("W_LEVEL") > -1) { //Уровень воды
      if ((watern > 700) and (waterv > 700))
      {
        sms(String("HIGH"), String("+7926*******"));
      }
      if ((watern > 700) and (waterv < 700))
      {
        sms(String("Medium"), String("+7926*******"));
      }
      else {
        sms(String("LOW"), String("+7926*******"));
      }
    }

    if (inputString.indexOf("W_T") > -1) { //Температура воды
      sms(String(" WaterTemp " + String(ds.getTempCByIndex(0)) + " *C "), String("+7926*******"));
    }

    if (inputString.indexOf("LAST_P") > -1) { //Последний полив
      sms(String(dattomarus), String("+7926*******"));
    }
    if (inputString.indexOf("OK") == -1) {
      mySerial.println("AT+CMGDA=\"DEL ALL\"");
      delay(1000);
    }
      digitalWrite(POMPA, LOW);
    inputString = "";

  }

}
void sms(String text, String phone)  // Процедура Отправка SMS
{
  Serial.println("SMS send started");
  mySerial.println("AT+CMGS=\"" + phone + "\"");
  delay(500);
  mySerial.print(text);
  delay(500);
  mySerial.print((char)26);
  delay(500);
  Serial.println("SMS send complete");
  delay(2000);
}
