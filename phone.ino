// Variable definition
//----------------------------------------------------------------
// Shift register
//Pins connected to 74HC595 shiftregister
int latchPin = 8;
int clockPin = 12;
int dataPin = 11;
int N_EN = 7;

// Static array
byte shift1[13];
byte shift2[13];
int number = 0;

// Comparision table ShiftRegister => Headset keys
byte headsetkey[13];

// Rotary phone decoder
int needToPrint = 0;
int count;
int in = 2;
int lastState = LOW;
int trueState = LOW;
long lastStateChangeTime = 0;
int cleared = 0;

int earpeace = 3;
boolean earpeaceFlag = false;

const int dialHasFinishedRotatingAfterMs = 100;
const int debounceDelay = 10;
const int DialTimeout = 1500;
long lastDialTime = 0;
boolean checkForDial = false;

// Spekaer Enable
int speaker = 13;

// Phone Bell
int bellOut1 = 5;
int bellOut2 = 6;
int bellEN = 9;

// Headset LED's
boolean activeLEDFlag = false;
boolean ringLEDFlag = false;

boolean ringing = false;

// Standalone ringing
boolean standaloneRing = false;
int standaloneRingIN = 4;

// Setup
//----------------------------------------------------------------
void setup() {
  // initialize serial:
  Serial.begin(9600);
  
  // Shiftregisters outputs
  shift1[0] = 0; 
  shift1[1] = B00000001;
  shift1[2] = B10000000;
  shift1[3] = B01000000;
  shift1[4] = B00100000;
  shift1[5] = B00010000;
  shift1[6] = B00001000;
  shift1[7] = B00000100;
  shift1[8] = B00000010;
  shift1[9] = 0;
  shift1[10] = 0;
  shift1[11] = 0;
  shift1[12] = 0;
  shift2[0] = 0; 
  shift2[1] = 0;
  shift2[2] = 0;
  shift2[3] = 0;
  shift2[4] = 0;
  shift2[5] = 0;
  shift2[6] = 0;
  shift2[7] = 0;
  shift2[8] = 0;
  shift2[9] = B00000001;
  shift2[10] = B10000000;
  shift2[11] = B01000000;
  shift2[12] = B00100000;
  
  // Bluetooth headset keys to shiftregister pins
  headsetkey[0] = 9;
  headsetkey[1] = 6;
  headsetkey[2] = 12;
  headsetkey[3] = 2;
  headsetkey[4] = 7;
  headsetkey[5] = 11;
  headsetkey[6] = 3;
  headsetkey[7] = 8;
  headsetkey[8] = 10;
  headsetkey[9] = 4;
  headsetkey[10] = 1;  // RESET (earpeace right)
  headsetkey[11] = 5;  // Take phone (earpeace left)
    
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(N_EN, OUTPUT);
  pinMode(earpeace, INPUT);
  pinMode(speaker, OUTPUT);
  
  pinMode(bellOut1, OUTPUT);
  pinMode(bellOut2, OUTPUT);
  pinMode(bellEN, OUTPUT);  
  
  pinMode(standaloneRingIN, INPUT);
  
  // Disable the shiftregister output, gets activated after the register
  // got written
  digitalWrite(N_EN, HIGH);
  
  // Reset headset by pressing the hangup key
  pressKey(10);
}


// Main loop
//----------------------------------------------------------------
void loop() {
  readRotaryPhone();
  readHeadsetLED();
  readEarpeace();
  
  // Standalone ring check
  if (standaloneRing == true){
    if (digitalRead(standaloneRingIN) == HIGH){
      ringBell();
    }
  }
}

// Helper functions
//----------------------------------------------------------------

// Rings the bell
void ringBell(){
  digitalWrite(bellEN, HIGH);
  digitalWrite(bellOut1, LOW);
  digitalWrite(bellOut2, HIGH);
  delay(35);
  digitalWrite(bellOut1, HIGH);
  digitalWrite(bellOut2, LOW);
  delay(35);
  digitalWrite(bellEN, LOW);
}

// Headset LED checks (does it ring?, is it active?)
void readHeadsetLED(){
  int activeLED = analogRead(A0);
  //Serial.println(activeLED);
  if (activeLED < 300 ){
    if (activeLEDFlag == false){
      Serial.println("active");
    }
    activeLEDFlag = true;
  }else{
    if (activeLEDFlag == true){
      Serial.println("active END");
    }
    activeLEDFlag = false;
  }
  
  int ringLED = analogRead(A1);
  if (ringLED < 200 ){
    if (ringLEDFlag == false){
      Serial.print(ringLED);
      Serial.println("ringing");
      ringing = true;
    }
    if (activeLEDFlag == true){  // only ring if active LED is also on
      ringBell();
    }
    ringLEDFlag = true;
  }else{
    if (ringLEDFlag == true){
      Serial.print(ringLED);
      Serial.println("ringing END");
      if (activeLEDFlag == false){  // reset when active is also false
        ringing = false;
        if (digitalRead(earpeace) == HIGH){  // Reset when the earpeace is on the hoock
          pressKey(10);
          delay(20);
          pressKey(10);
          delay(30);
          pressKey(10);
        }
      }
    }
    ringLEDFlag = false;
  }
}


// Earpeace
void readEarpeace(){
  if (digitalRead(earpeace) == HIGH){
    if (earpeaceFlag == false){
      // Earpeace gets hanged
      digitalWrite(speaker, LOW);
      Serial.println("Speaker LOW");
      pressKey(10);
      delay(20);
      pressKey(10);
    }
    earpeaceFlag = true;
  }else{
    if (earpeaceFlag == true){
      // Earpeace taken off
      if (ringing == true){
        pressKey(11);
      }else{
        pressKey(10);
      }
      Serial.println("Speaker HIGH");
      digitalWrite(speaker, HIGH);
    }
    earpeaceFlag = false;
  }
}

// Rotary phone decoder
// --------------------
void readRotaryPhone(){
  int reading = digitalRead(in);
  
  if ((millis() - lastStateChangeTime) > dialHasFinishedRotatingAfterMs) {
    // the dial isn't being dialed, or has just finished being dialed.
    if (needToPrint) {
      // if it's only just finished being dialed, we need to send the number down the serial
      // line and reset the count. We mod the count by 10 because '0' will send 10 pulses.
      count = count % 10;
      if (digitalRead(earpeace) == HIGH){
        if (count == 0){
          Serial.println("ON/OFF phone");
          pressKeyLong(10);
        }
        if (count == 9){
          Serial.println("Ring bell test");
          for (int c=0; c<10; c++){
            ringBell();
          }
        }
        if (count == 8){
          if (standaloneRing == false){
            Serial.println("Standalone ring ON");
            standaloneRing = true;
            for (int c=0; c<2; c++){
              ringBell();
            }
          }else{
            Serial.println("Standalone ring OFF");
            standaloneRing = true;
            for (int c=0; c<1; c++){
              ringBell();
            }
          }
        }
      }else{
        pressKey(count);
        lastDialTime = millis();
        checkForDial = true;
      }
      needToPrint = 0;
      count = 0;
      cleared = 0;
    }
  } 
  
  // Check if we should press the dial key (the last digit was entered > DialTimeout ago)
  if (checkForDial == true){
    if ((millis() - lastDialTime) > DialTimeout){
      pressKey(11);
      checkForDial = false;
    }
  }
  
  if (reading != lastState) {
    lastStateChangeTime = millis();
  }
  if ((millis() - lastStateChangeTime) > debounceDelay) {
    // debounce - this happens once it's stablized
      if (reading != trueState) {
        // this means that the switch has either just gone from closed->open or vice versa.
        trueState = reading;
        if (trueState == HIGH) {
        // increment the count of pulses if it's gone high.
        count++; 
        needToPrint = 1; // we'll need to print this number (once the dial has finished rotating)
        // Reset dial timeout
        if (checkForDial == true){
          lastDialTime = millis();
        }
      } 
    }
  }
  lastState = reading;
}


// Shift registers
// ---------------
void pressKey(int key){
  number = headsetkey[key];
  digitalWrite(N_EN, LOW);
  registerWrite(number);
  delay(200);
  registerWrite(0);
  digitalWrite(N_EN, HIGH);
}

void pressKeyLong(int key){
  number = headsetkey[key];
  digitalWrite(N_EN, LOW);
  registerWrite(number);
  delay(3000);
  registerWrite(0);
  digitalWrite(N_EN, HIGH);
}

void registerWrite(int number) {
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, shift2[number]);
    shiftOut(dataPin, clockPin, MSBFIRST, shift1[number]);
    digitalWrite(latchPin, HIGH);
}
