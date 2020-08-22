/*
  FoxRIP - Johnny July 2020
  
*/

//#define _TESTMODE         // Run Test Code or Main Loop
#define _SDEBUG           // Serial Debug
#define _NIGHTMODE        LOW 

#define _MOTION_INDICATOR // 
#define PUMP_TIME   3000
#define PUMP_DELAY  2500

#define NIGHT_THRESHOLD (300) 
#define NUMPUMPTURNS (2) 

#define _SECS(_A) (_A * 1000L) 
#define _MINS(_A) (_A * 60L * 1000L) 

#define MIN_DEBOUNCE_DELAY  _SECS(4)  
#define MAX_DEBOUNCE_DELAY  _MINS(10)

#define MOTION_DETECTOR 10 
#define NIGHTMODE_PIN 12      // pull down if you want it to wok in day and night

// Analog IN
#define TIMERDELAY_ADJUST 14  // Analog  (A0)
#define LIGHT_PIN 15         // Analog LDR pin (A1)
 

#define ARMED_PIN 2             // Lights UP whenever the Alarm is armed.

#define STATUS_PIN LED_BUILTIN
#define PUMP_PIN        7       // Analog OUT (PWM)
#define MOTION_INDICATOR_PIN      5       // Indicates if we have motion or not (always showing)
#define MODE_INDICATOR_PIN        3            

#define STATUSLEDDELAY 250 

#define STARTDELAY      10000

void motionIndicator(void) ;
void modeIndicator(void) ;


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize i/o pins
  Serial.begin(115200) ;
  
  pinMode(STATUS_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(ARMED_PIN, OUTPUT);
  pinMode(MOTION_INDICATOR_PIN, OUTPUT);
  pinMode(MODE_INDICATOR_PIN, OUTPUT);

  digitalWrite(STATUS_PIN,LOW) ;
  digitalWrite(PUMP_PIN,LOW) ;
  digitalWrite(ARMED_PIN,LOW) ;
  digitalWrite(MOTION_INDICATOR_PIN,LOW) ;
  digitalWrite(MODE_INDICATOR_PIN,LOW) ;

  
  pinMode(MOTION_DETECTOR,INPUT) ;
  pinMode(LIGHT_PIN,INPUT) ;                // 
  pinMode(NIGHTMODE_PIN,INPUT_PULLUP) ;
  pinMode(TIMERDELAY_ADJUST,INPUT) ;      // timer adjust after

  delay(1000) ;
  
  Serial.print("Pin A0 =") ;
  Serial.println(A0) ;
  Serial.print("Pin A1 =") ;
  Serial.println(A1) ;
    
  Serial.print("Built in LED =") ;
  Serial.println(LED_BUILTIN) ;
  Serial.println(_24HourMode() ? "24Hour Mode" : "NightMode ") ;


  startupDelay(STARTDELAY) ;
  

}


void startupDelay(int sDelay) {
    int pulseDelay = 100 ;
    int numOfIndicators = 5 ;
    
    int numLoops = max(1,sDelay / (2 * 5 * pulseDelay))   ;
    for(int i = 0 ; i < numLoops ; i++) {
      
      pinToggle(STATUS_PIN,pulseDelay) ;
      pinToggle(ARMED_PIN,pulseDelay) ;
      pinToggle(MODE_INDICATOR_PIN,pulseDelay) ;
      pinToggle(MOTION_INDICATOR_PIN,pulseDelay) ;
      pinToggle(PUMP_PIN,pulseDelay) ;
    }
    
}

void loop() {
#ifndef _TESTMODE
  loopMain() ;
#else
  loopTest() ;
#endif
}


void loopTest() {

#ifdef _SDEBUG
  Serial.println("*** loopTest ***") ;
  Serial.println(_24HourMode() ? "*** 24 HOUR MODE ***" : "*** NIGHT MODE ***") ;
  Serial.println(isNight() ? "***NIGHTTIME ***" : "*** DAYTIME ***") ;
 #endif

  calculateDebounceDelay() ;
  motionIndicator() ;
  modeIndicator() ;
  
  pinToggle(STATUS_PIN,250) ;

  if (isNight()) {
     digitalWrite(ARMED_PIN,HIGH) ;
  } else {
    if (_24HourMode()) {
      digitalWrite(ARMED_PIN,HIGH) ;
      delay(50) ;
      digitalWrite(ARMED_PIN,LOW) ;
      delay(450) ;    
    } else {
      pinToggle(ARMED_PIN,250) ;        
    }
    
  }
  
  pinToggle(PUMP_PIN,250) ;
  
      
}


// the loop function runs over and over again forever
void loopMain() {
 
#ifdef _TESTMODE
    Serial.println("*** loopMain ***") ;
#endif

    pinToggle(STATUS_PIN,STATUSLEDDELAY) ;  

#ifdef _MOTION_INDICATOR    
      motionIndicator() ;
#endif
    modeIndicator() ;

    if (_24HourMode() || isNight()) {
      digitalWrite(ARMED_PIN,HIGH) ;

#ifdef _SDEBUG
      Serial.println("WE ARE NOW ARMED...CHECKING MOTION") ;      
#endif
      if (motionDetected()) {
        digitalWrite(STATUS_PIN,HIGH) ;
        // Turn Pump on Several times
        for(int i = 0 ; i < NUMPUMPTURNS ; i++) {
          pumpOn(PUMP_TIME) ;
          delay(PUMP_DELAY) ;
        }
        debounceAlarm() ;
        digitalWrite(STATUS_PIN,LOW) ;
      }
    } else {
      test() ;
      digitalWrite(ARMED_PIN,LOW) ;
    }

              
}


bool _24HourMode() {
  return !nightMode() ;
}


bool nightMode() {
  return (digitalRead(NIGHTMODE_PIN) == _NIGHTMODE) ;
}



bool isNight() {
 
  // READ IN ADC and see if it has reached a threshhold value
  // High readings == DAYTIME, Low readings == NIGHTTIME 
  int val = analogRead(LIGHT_PIN) ;

#ifdef _SDEBUG
  Serial.print("isNight Reading ") ;
  Serial.print(val) ;
  Serial.print(" - compare with threshold ") ;
  Serial.println(NIGHT_THRESHOLD) ; 
#endif

  return (val < NIGHT_THRESHOLD) ;
}

long calculateDebounceDelay() {
  int val = analogRead(TIMERDELAY_ADJUST) ;
  long del = map(val, 0, 1023, MIN_DEBOUNCE_DELAY ,MAX_DEBOUNCE_DELAY) ; 
 
#ifdef _SDEBUG
  Serial.print("ADC debounceAlarm - Delay Reading : ") ;
  Serial.println(val) ;
  Serial.print("Mapped Delay : ") ;
  Serial.println(del) ;

  Serial.print("min(") ;
  Serial.print(MIN_DEBOUNCE_DELAY) ;
  Serial.print(") max(") ;
  Serial.print(MAX_DEBOUNCE_DELAY) ;
  Serial.println(")") ;
    
#endif

  return del ;  
}


void debounceAlarm() {

#ifdef _SDEBUG
  Serial.println("debounceAlarm") ;
#endif
  int pulseDelay = 100 ;
  long del = calculateDebounceDelay() ;
  long numPulses = del / (2*pulseDelay) ;
  for(int i = 0 ; i < numPulses ; i++) {
    digitalWrite(ARMED_PIN,HIGH) ;
    delay(pulseDelay) ;
    digitalWrite(ARMED_PIN,LOW) ;
    delay(pulseDelay) ;
  }
}


void pumpOn(int pumpFor) {
#ifdef _SDEBUG
  Serial.println("Pump ** ON **") ;
#endif  
  
  digitalWrite(PUMP_PIN,HIGH) ;
  delay(pumpFor) ;
  digitalWrite(PUMP_PIN,LOW) ;
  
#ifdef _SDEBUG
  Serial.println("Pump ** OFF **") ;
#endif
}



void motionIndicator() {
  if (motionDetected()) {
     digitalWrite(MOTION_INDICATOR_PIN,HIGH) ;
  } else {
     digitalWrite(MOTION_INDICATOR_PIN,LOW) ;
  }
}

void modeIndicator() {
  if (_24HourMode()) {
     digitalWrite(MODE_INDICATOR_PIN,HIGH) ;
  } else {
     digitalWrite(MODE_INDICATOR_PIN,LOW) ;
  }
}

bool motionDetected() {
  bool motionDetected = (digitalRead(MOTION_DETECTOR) == HIGH) ;
  
#ifdef _SDEBUG
  Serial.println(motionDetected ? "*** MOTION DETECT****" : "ALL QUIET IN THE  GARDEN!") ; 
#endif
  
  return motionDetected ;
}

void pinToggle(int pin,int del) {
  digitalWrite(pin, HIGH);   // turn the pin on (HIGH is the voltage level)
  delay(del);                       // wait 
  digitalWrite(pin, LOW);    // turn the pin off by making the voltage LOW
  delay(del);   // wait 



} 

void test() {
  debugAnalogPin(" LDR ",LIGHT_PIN) ;
  Serial.print(" THRESHOLD FOR LDR ") ;
  Serial.println(NIGHT_THRESHOLD) ;
  debugAnalogPin(" TimerAdjust ",TIMERDELAY_ADJUST) ;
  debugPin(" Motion ",MOTION_DETECTOR) ;
  debugPin(" Night Mode ",NIGHTMODE_PIN) ;
}

int debugPin(const char *pinName, int pin) {
  int val = digitalRead(pin) ;
  Serial.print(pinName) ;
  Serial.println(val) ;
  return val ;    
}

int debugAnalogPin(char *pinName, int pin) {
  int val = analogRead(pin) ;

#ifdef _SDEBUG  
  Serial.print(pinName) ;
  Serial.println(val) ;
#endif

  return val ;  
}

      
