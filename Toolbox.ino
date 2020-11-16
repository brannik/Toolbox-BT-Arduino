#include <EEPROM.h>
#include <SoftwareSerial.h>

String drlDeffState = "";
String drlDelay = "";
String interDeffState = "";
String interDelay = "";

String drlState = "";
String interState = "";
String lastDrlState = "";
String lastInterState = "";

// debug strings
String command = "";
String lastMsg = "";
String message = "";

bool firstStart;
SoftwareSerial bluetooth(10, 11); // RX, TX

const int buzzer = 9;
const int drlRelay = 2;
const int interRelay = 4;

bool delayRunningDrl = false; // true if still waiting for delay to finish
bool delayRunningInter = false;
bool frun;

// to do
// if there is no bluetooth read from EEPROM
// if there is bluetooth save new data to EEPROM
void setup(){
  Serial.begin(9600);
  bluetooth.begin(9600);
  pinMode(buzzer, OUTPUT);
  pinMode(drlRelay,OUTPUT);
  pinMode(interRelay,OUTPUT);
  firstStart = true;
  frun = true;
  delayRunningDrl = true;
  delayRunningInter = true;
  establishContact();
  
}

void loop(){
    if(firstStart == true){
      //bluetooth.print("1");
      if (bluetooth.available() > 0){
        bluetooth.print("2");
        String myString = bluetooth.readString();
        writeEeprom(myString);
        command = getValue(myString, '&', 0);
        checkCommand(command,myString);
      }else{
        String myString = getEepromData();
        command = getValue(myString, '&', 0);
        checkCommand(command,myString);
      }

      // delayed start 
      firstStart = false; 
    }else{
      if (bluetooth.available() > 0){
        String myStringTwo = bluetooth.readString();
        command = getValue(myStringTwo, '&', 0);
        checkCommand(command,myStringTwo);
      }
      char drlDEL[20];
      drlDelay.toCharArray(drlDEL, sizeof(drlDEL));
      int drlDelInt = atoi(drlDEL);
      char interDEL[20];
      interDelay.toCharArray(interDEL, sizeof(interDEL));
      int interDelInt = atoi(interDEL);
      
      static uint32_t oldtimeDrl=millis();

      if (delayRunningDrl && (millis()-oldtimeDrl) > drlDelInt*1000) {
        delayRunningDrl = false;
        oldtimeDrl = millis();
        int as = prepareDRL(drlDeffState);
        digitalWrite(drlRelay,as);
        tone(buzzer,1500);
        delay(250);
        noTone(buzzer);
        //Serial.println("Delayed DRL");
      }

      static uint32_t oldtimeInter=millis();

      if (delayRunningInter && (millis()-oldtimeInter) > interDelInt*1000) {
        delayRunningInter = false;
        oldtimeInter = millis();
        int as = prepareINTER(interDeffState);
        digitalWrite(interRelay,as);
        tone(buzzer,1500);
        delay(250);
        noTone(buzzer);
        //Serial.println("Delayed");
      }
    }
    delay(100);
}

void writeEeprom(String data){
  //EEPROM ADRESS
  //drlDeffState - 100
  //drlDelay - 101
  //interDeffState - 102
  //interDelay - 103
  //EEPROM.write();
  String tmpDrlState,tmpInterState,tmpDrlDelay,tmpInterDelay;
  int bDrlState,bInterState;
  tmpDrlState = getValue(data, '&', 1);
  tmpDrlDelay = getValue(data, '&', 2);
  tmpInterState = getValue(data, '&', 3);
  tmpInterDelay = getValue(data, '&', 4);
  
  bDrlState = convertValue(tmpDrlState);
  bInterState = convertValue(tmpInterState);

  EEPROM.write(100,bDrlState);
  EEPROM.write(101,tmpDrlDelay.toInt());
  EEPROM.write(102,bInterState);
  EEPROM.write(130,tmpInterDelay.toInt());
  
}
int convertValue(String val){
  if(val.equals("ON")){
    return 1; 
  }else{
    return 0;
  }
}
String getEepromData(){
  int drlDelay,interDelay;
  String drlState,interState;
  char retData[250];
  drlState = valToString(EEPROM.read(100));
  drlDelay = EEPROM.read(101);
  interState = valToString(EEPROM.read(102));
  interDelay = EEPROM.read(130);
  
  char drlStateCon[5];
  char interStateCon[5];
  drlState.toCharArray(drlStateCon, sizeof(drlStateCon));
  interState.toCharArray(interStateCon, sizeof(interStateCon));
  sprintf(retData, "2&%s&%d&%s&%d&end", drlStateCon,drlDelay,interStateCon,interDelay);
  Serial.println(retData);
  return retData;
}
String valToString(int val){
  if(val == 1){
    return "ON";
  }else{
    return "OFF";
  }
}
void establishContact() {
  int i=0;
  for(i=0;i<10;i++){
    if(bluetooth.available()>0){
      i=10;
    }else{
      if(i==10){
        // read EEPROM
        String myStringTwo = getEepromData();
        command = getValue(myStringTwo, '&', 0);
        checkCommand(command,myStringTwo);
      }
    }
    delay(500);
  }
}
int prepareDRL(String txt){
  if(txt.equals("ON")){
    return HIGH;
  }else{
    return LOW;
  }
}
int prepareINTER(String tet){
  if(tet.equals("ON")) return HIGH;
  if(tet.equals("OFF")) return LOW;
}

 
void checkCommand(String data, String comm){
  if(data == "1"){
    drlState = getValue(comm, '&', 1);
    interState = getValue(comm, '&', 2);
    //Serial.println("DRL      -> " + drlState);
    //Serial.println("INTERIOR -> " + interState);
    // if there is changed state do action
    if((drlState != lastDrlState) || (interState != lastInterState)){
     // Serial.println(">>> " + drlState + " << >> " + interState);
      int drl = prepareDRL(drlState);
      int inter = prepareINTER(interState);      
      digitalWrite(drlRelay,drl);
      digitalWrite(interRelay,inter);
      lastDrlState = drlState;
      lastInterState = interState;
      tone(buzzer,1500);
      delay(250);
      noTone(buzzer);
    }
  }
  if(data == "2"){
    drlDeffState = getValue(comm, '&', 1);
    drlDelay = getValue(comm, '&', 2);
    interDeffState = getValue(comm, '&', 3);
    interDelay = getValue(comm, '&', 4);
    //Serial.println("DRL DEFF STATE      -> " + drlDeffState);
    //Serial.println("DRL DELAY           -> " + drlDelay);
    //Serial.println("INTERIOR DEFF STATE -> " + interDeffState);
    //Serial.println("INTERIOR DELAY      -> " + interDelay);
    writeEeprom(comm);
    // if there is changed state do action

  }
}
String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
