#define buzzer        13
#define triggerPin1    8
#define triggerPin2    9
#define echoPin1       10
#define echoPin2       11
#define IRSignalPin      12

#include <Wire.h>
#include <Keypad.h>
#include <IRremote.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // 0x27 là địa chỉ của lcd 16x2
RTC_DS1307 RTC;

unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long getTime1 = 0;
unsigned long getTime2 = 0; 
int initDistance1 = 0;
int initDistance2 = 0;
int currentDistance1 = 0;
int currentDistance2 = 0; 
int numInside = 0;
String turnTake = "";
int timeOut = 0;
uint32_t dataRemote = 0;

const byte ROWS = 4; 
const byte COLS = 4; 
char password[4];
char openDoor[]="123";
char closeDoor[]="312";
int indexPass = 0;
int countPress = 0;

bool flagCheckCorrect = 0;
char MatrixKey[ROWS][COLS] = 
{
  {'1','2','3','4'},
  {'5','6','7','8'},
  {'9','A','B','C'},
  {'D','E','F','G'}
};
byte rowPins[ROWS] = {3,2,1,0};
byte colPins[COLS] = {7,6,5,4}; 
Keypad Mykeys = Keypad( makeKeymap(MatrixKey), rowPins, colPins, ROWS, COLS); 

void setup () {
  Serial.begin(9600);
  pinMode(buzzer, OUTPUT);
  pinMode(triggerPin1, OUTPUT);
  pinMode(triggerPin2, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(echoPin2, INPUT);

  IrReceiver.begin(IRSignalPin); 

  lcd.init(); 
  lcd.backlight(); 
  lcd.begin(20, 4);
  lcd.clear();
    
  Wire.begin();
  Wire.beginTransmission(0x68);// địa chỉ của ds1307
  Wire.write(0x07); // 
  Wire.write(0x10); // 
  Wire.endTransmission();
 
  RTC.begin();
  if (!RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  RTC.adjust(DateTime(2024, 5, 9, 11, 50, 0));
 
  delay(500);
  getInitialDistances();
  printInitialDistances();
  printInitialScreen();
}

void loop () {
  if(flagCheckCorrect) countNumberInside();
  lcdDisplayTime();
  keyProcess();
  IRProcess();
}

void lcdDisplayTime() {
  currentMillis = millis();
  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;
    DateTime now = RTC.now();
    lcd.setCursor(0, 0);
    lcd.print(now.hour(), DEC);
    lcd.print(":");
    lcd.print(now.minute(), DEC);
    lcd.print(":");
    lcd.print(now.second(), DEC);
    lcd.print(" "); 
  }
}

void keyProcess() {
  char EnterKey = Mykeys.getKey();
  if (EnterKey) {
    password[indexPass] = EnterKey; 
    lcdDisplayPass();
    digitalWrite(buzzer, HIGH);
    delay(150);
    digitalWrite(buzzer, LOW);
    indexPass ++;
    countPress ++;
  }
  if (countPress == 3)  {
    if (!strcmp(password, openDoor)) {
      indexPass = 0;
      lcdPrint("Correct! Open!       ");
      flagCheckCorrect = 1;
    }
    else if (!strcmp(password, closeDoor)) {
      indexPass = 0;
      lcdPrint("Correct! Close!       ");
      flagCheckCorrect = 0;
    }
    else {
      indexPass = 0;
      lcdPrint("Uncorrect! Try again");
      flagCheckCorrect = 0;
    }
    countPress = 0;
    for(int k = 0; k < 3; k++)
    password[k] = '*';
    lcdDisplayPass();
  }
}

void lcdDisplayPass() {
  lcd.setCursor(0, 1);
  lcd.print("Pass: ");
  lcd.print(password);
}

void getInitialDistances() {
  currentDistance1 = getCurrentDistance(triggerPin1, echoPin1);
  initDistance1 = currentDistance1;
  currentDistance2 = getCurrentDistance(triggerPin2, echoPin2);
  initDistance2 = currentDistance2;
}

int getCurrentDistance(int triggerPin, int echoPin) {
  digitalWrite(triggerPin, LOW); 
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);  
  delayMicroseconds(10); 
  digitalWrite(triggerPin, LOW); 
  unsigned long getTime = pulseIn(echoPin, HIGH);
  return getTime / 2 / 29.412;
}

void countNumberInside() {
  currentDistance1 = 0;
  currentDistance2 = 0;
  currentDistance1 = getCurrentDistance(triggerPin1, echoPin1);
  currentDistance2 = getCurrentDistance(triggerPin2, echoPin2);
  if (currentDistance1 < initDistance1 - 10 && turnTake.charAt(0) != '1')
    turnTake += "1";
  else if (currentDistance2 < initDistance2 - 10 && turnTake.charAt(0) != '2')
    turnTake += "2";

  if (turnTake.equals("12")) {
    numInside ++;
    lcdDisplayNum();
    delay(500);
    turnTake = "";
  } else if (turnTake.equals("21") && numInside > 0) {
    numInside --;
    lcdDisplayNum();
    delay(500);
    turnTake = "";
  }
  if (turnTake.length() > 2 || turnTake.equals("11") || turnTake.equals("22") || timeOut > 200)
    turnTake = "";

  if (turnTake.length() == 1)
    timeOut ++;
  else
    timeOut = 0;
}

void lcdDisplayNum() {
  lcd.setCursor(9, 3);
  lcd.print(numInside);
}

void IRProcess() {
  if (IrReceiver.decode())  {
    dataRemote = IrReceiver.decodedIRData.decodedRawData;
    if (dataRemote > 0) {
      Serial.println(dataRemote); 
      IRHandle();
    }
    IrReceiver.resume(); // Cho phép nhận giá trị tiếp theo
  } 
}

void IRHandle() {
  if (dataRemote == 3125149440) {
    lcdPrint("Correct! Open!       ");
    flagCheckCorrect = 1;
  } else if (dataRemote == 3108437760) {
    lcdPrint("Correct! Close!       ");
    flagCheckCorrect = 0;
  }
}

void lcdPrint(String message) {
  lcd.setCursor(0, 2);
  lcd.print(message);
}

void printInitialDistances() {
  Serial.print(initDistance1); Serial.print("   ");
  Serial.println(initDistance2);
}

void printInitialScreen() {
  lcdPrint("Please Enter Pass!");
  delay(500);
  lcdPrint("To Control The Door!");
  delay(500);
  lcdPrint("Pass:               ");
  lcd.setCursor(0, 3);
  lcd.print("In Room: 0 left! ");
}

void printResetPw()
{
  lcd.setCursor(0, 1);
  lcd.print("Pass: ");
  lcd.print("***");
}