//*****************************************************************************
//  Name    : VSerPy (serial connection to ios (via shift out ics)                         
//  Author  : Andreas & Bernhard Ihrig
//  Year    : 2014
//  Version : 0.3                                            
//  Notes   : arduino program for serial connection to ios (via shift out ics)                        
//*****************************************************************************

// definition of serial input format
//
// control shutter:
//   "cxx:y"
//   where 
//     xx - number of shutter with leading zero
//     y - action to perform (0 => open, 1 => close=)
//
//   special actions: 
//     first floor: "cOG:y"
//     groud floor: "cEG:y"
//     all: "cA:y"
//
// read data:
//   "rxx"
//   where
//     xx - number of shutter with leading zero
//
//   special actions: 
//     all: "rA"

// "hard" constants
// constant describing number of used shift out ics, limiting the number of shift outs
const int nbrShiftIcs = 6;
const int nbrPerShiftIc = 4;
// shutter numbers for ground floor (GF)
const int shuttersGFCount = 6;
const int shuttersGF[shuttersGFCount] = {1,2,3,4,5,6};
// shutter numbers for first floor (FF)
const int shuttersFFCount = 6;
const int shuttersFF[shuttersFFCount] = {7,8,9,10,11,12};

// global variables
// pin connected to ST_CP of 74HC595
int latchPin = 5;
// pin connected to SH_CP of 74HC595
int clockPin = 7;
// pin connected to DS of 74HC595
int dataPin = 6;

// shutters Queue
int shuttersQueue[nbrShiftIcs]; // TODO: is this really needed twice?

// shift ic the shutter is connected to -> which byte has to be used (from 0 to 5)
int shiftIc = 0;
// number on shift ic (from 0 to 3 => 4 possible for 4 shutters)
int nbrOnShiftIc = 0;
// shutter number: which shutter should be changed (from 1 to 24)
// value to put into shutters_queue
int transferValue = 0;
// counter for loops
int i = 0;

// serial input
const int serialInputLength = 20;
char serialInput[serialInputLength + 1];
// control-/read-flag (values: control => true, read => false
boolean crFlag = true;


// ---------- main functions ----------

void setup() {
  // set pins to output to control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  // set up shuttesQueue for the values to be send to the shiftouts and preset with zeros
  int shuttersQueue[nbrShiftIcs]; // TODO: is this really needed twice?
  setAllValuesTo(0);
  
  // set up serial connection to get inputs
  Serial.begin(9600);
}

void loop() {
  //Serial.println("new loop");
  
  // if there is data on serial input
  if(Serial.available() > 0)
  {
    // read serial data and write to shuttersQueue
    interpretSerialInput();
  
    if(crFlag) {
      // put values from shuttersQueue to shift ics
      sendShuttersQueue();
    }
  
    // clear shuttersQueue
    setAllValuesTo(0);
  }
  
  // wait 50ms until next loop
  delay(50);
}


// ---------- shutters logic ----------

// write transfer value to shutters queue
// parameters:
//   int shutter - number of shutter to control
//   boolean action - flag for action ( false (or 0) => open, true (or 1) => close)
void controlShutter(int shutter, boolean action){
  // get shift ic, nbr on ic and value
  shiftIc = getShiftNbr(shutter);
  nbrOnShiftIc = getNbrOnShiftIc(shutter, shiftIc);
  transferValue = getTransferValue(nbrOnShiftIc, action); // TODO: better description // action to define direction, power of 2 because of byte format
  
  // write calculated value to shuttersQueue (sum up to set multiple ports per ic)
  shuttersQueue[shiftIc] = shuttersQueue[shiftIc] + transferValue;
}

// wrapper for all ground floor shutters
// parameters:
//   boolean action - (see above)
void controlAllGF(boolean action){
  for(i = 0; i < shuttersGFCount; i++) {
    controlShutter(shuttersGF[i], action);
  }
}

// wrapper for all first floor shutters
// parameters:
//   boolean action - (see above)
void controlAllFF(boolean action){
  for(i = 0; i < shuttersFFCount; i++) {
    controlShutter(shuttersFF[i], action);
  }
}

// wrapper for all shutters
// parameters:
//   boolean action - (see above)
void controlAll(boolean action){
  controlAllGF(action);
  controlAllFF(action);
}

// get number of shift ic
// parameters:
//   int shutter - number of shutter
int getShiftNbr(int shutter){
  return int((shutter-1)/nbrPerShiftIc);
}

// get number on shift ic
// take shutter number, subtract (preceeding) shift ports (nbrPerShiftIc * shiftIC) and decrease by 1
// parameters:
//   int shutter - number of shutter
//   int shiftIC - number of shift ic
int getNbrOnShiftIc(int shutter, int shiftIC){
  return (shutter - nbrPerShiftIc * shiftIC - 1);
}

// get transfer value
// parameters:
//   int nbrOnShiftIC - shutter number on ic
//   boolean action - (see above)
int getTransferValue(int nbrOnShiftIc, boolean action) {
  // each 2 shift ports are grouped to one shutter (open, close)
  // the ports are addressed via bits of a byte
  // therefore we have to use the power of two
  return int(ceil(
             pow(2, 2 * nbrOnShiftIc + action)
            ));
  // pin usage on 74HC595: open: 15,2,4,6 und close: 1,3,5,7
}


// ---------- serial input logic ----------

// read serial input and analyse commands to control shutters
void interpretSerialInput() {
  // read serial input
  readSerialInput();
  //Serial.println(serialInput);
  
  // analyse input
  if(serialInput[0] == 'c') {
    crFlag = true;
    if(serialInput[1] == 'O' && serialInput[2] == 'G') {
      if(serialInput[4] == '0') controlAllFF(0);
      else if(serialInput[4] == '1') controlAllFF(1);
    }
    else if(serialInput[1] == 'E' && serialInput[2] == 'G') {
      if(serialInput[4] == '0') controlAllGF(0);
      else if(serialInput[4] == '1') controlAllGF(1);
    }
    else if(serialInput[1] == 'A') {
      if(serialInput[3] == '0') controlAll(0);
      else if(serialInput[3] == '1') controlAll(1);
    }
    else {
      int shutter = readNbr(serialInput[1], serialInput[2]);
      //Serial.println(shutter);
      if(shutter >= 0) {
        if(serialInput[4] == '0') controlShutter(shutter, 0);
        else if(serialInput[4] == '1') controlShutter(shutter, 1);
      }
    }
  }
  else if(serialInput[0] == 'r') { // read data from shuttersQueue and send per serial connection
    crFlag = false;
    readShuttersQueue();
    if(serialInput[1] == 'A') {
      sendArraySerial();
    }
    else {
      int shutter = readNbr(serialInput[1], serialInput[2]);
      if(shutter >= 0) {
        sendSingleSerial(shutter);
      }
    }
    Serial.println();
  }
}

// get number for given chars
// parameters:
//   char c1 - first digit
//   char c0 - second digit
// returns:
//   int value or -1
int readNbr(char c1, char c0) {
  int c1z = int(c1)-48;
  int c0z = int(c0)-48;
  if( c1z <= 10 && c1z >= 0 && c0z <= 10 && c0z >= 0) return 10*c1z+c0z;
  else return -1;
}

// get max. 20 bytes from serial input
void readSerialInput() {
  i = 0;
  while (i < serialInputLength) {
    if(Serial.available() > 0) {
      serialInput[i] = Serial.read();
      //serialInput[i+1] = '\0';
    }
    else {
      serialInput[i] = '\0';
    }
    i += 1;
  }
}

// serial send state information for shutter
// parameters:
//   int shutter - number of shutter
void sendSingleSerial(int shutter) {
  shiftIc = getShiftNbr(shutter);
  nbrOnShiftIc = getNbrOnShiftIc(shutter, shiftIc);
  int state = -1;
  if( (shuttersQueue[shiftIc] & getTransferValue(nbrOnShiftIc, 1)) > 0 ) { // check if shutter is opened via bit wise AND
    state = 1;
  }
  else if( (shuttersQueue[shiftIc] & getTransferValue(nbrOnShiftIc, 0)) > 0 ) { // check if shutter is closed via bit wise AND
    state = 0;
  }
  
  Serial.print("s");
  Serial.print(shutter);
  Serial.print(":");
  Serial.print(state);
  Serial.print(";");
}

// wrapper for sendSingleSerial to send all shutter states
void sendArraySerial() {
  for(i = 0; i < nbrShiftIcs * nbrPerShiftIc; i++){
    sendSingleSerial(i);
  }
}

/*void eraseInputCharUntil(int index) {
  for( i = 0; i < index && i < serialInputLength; i++) {
    serialInput[i] = ' ';
  }
}*/

// ---------- shift out ic logic ----------

void setAllValuesTo(int value){
  for(i = 0; i < nbrShiftIcs; i++){
    shuttersQueue[i] = value;
  }
}

// use shift library to send value to shift ic
// parameters:
//   int value - one byte to send
void sendShiftData(int value){
  shiftOut(dataPin, clockPin, LSBFIRST, value); 
}

// send all bytes of shutters queue
void sendShuttersQueue(){
  // ground latchPin and hold low for as long as you are transmitting
  digitalWrite(latchPin, LOW);
  
  Serial.print("Send to shift ic: ");
  for(i = 0; i < nbrShiftIcs; i++){
    sendShiftData(shuttersQueue[i]);
    Serial.print(shuttersQueue[i]);
    Serial.print(";");
  }
  Serial.println();
  
  // return the latch pin high to signal chip that it
  // no longer needs to listen for information
  digitalWrite(latchPin, HIGH);
}

// read bytes from shift ics (not implemented yet!)
int readShiftData(int value){
  // TODO: implement (?)
}

// wrapper to read all bytes from shift ics
void readShuttersQueue(){
  for(i = 0; i < nbrShiftIcs; i++){
    shuttersQueue[i] = readShiftData(i);
  }
}

