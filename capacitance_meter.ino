/*
 * Based on http://www.arduino.cc/en/Tutorial/RCtime
 */

const int OUT_PIN = A2; //Positive lead
const int IN_PIN = A0; //Negative lead

/* constants */
float Vref = 5.0;
const int MAX_ADC_VALUE = 1023;

/* preferences */
//Small cap is the pF range
const int SMALLCAP_SAMPLINGTIME = 200; //sampling for this amount of time in ms
const int SMALLCAP_DECIMALPLACES = 1; //number of decimal places 

//Large cap for the nF and uF range
const int LARGECAP_DECIMALPLACES = 1;
const int LARGECAP_SAMPLINGTIME = 200; //sampling for this amount of time in ms

/* calibration */
float IN_STRAY_CAP_TO_GND = 26.87;
float IN_CAP_TO_GND  = IN_STRAY_CAP_TO_GND;
float R_PULLUP = 38.19;  //in k ohms

/* Program Variables */
float capacitance; //pF
float nullCapacitance = 0; //pF
float val; //ADC

//used on RC
unsigned long t; //microSeconds
float v; //volts

void setup()
{
  pinMode(OUT_PIN, OUTPUT);
  //digitalWrite(OUT_PIN, LOW);  //This is the default state for outputs
  pinMode(IN_PIN, OUTPUT);
  //digitalWrite(IN_PIN, LOW);

  Serial.begin(115200);
  
  Vref = (float)readVcc()/1000.0; //
  Serial.print(F("Vref calibrated to "));
  Serial.println(Vref);
}

void help(){
        while(Serial.available()){ //clear buffer
          Serial.read();
        }
        
        Serial.println(F("\n Help"));
        Serial.println(F("c [capacitance] - Plug a known capacitor and enter its value in pico Farads to calibrate the pF range."));
        Serial.println(F("p [capacitance] - Plug a known capacitor and enter its value in micro Farads to calibrate the nF-uF range."));
        Serial.println(F("n               - To null out the current value. (it only affects the displayed value, it doesn't affect calibration)"));
        Serial.println(F("0               - To reset the null to 0."));
        Serial.println(F("?               - Show this."));
        
        while(!Serial.available()); //wait for input
        while(Serial.available()){ //clear buffer
          Serial.read();
        }
}

void loop()
{
    
    val = smallCap();
    
    if (val < 1000)
    {
        unsigned long start = millis();
        
        float avgVal = val;
        int samples = 1;
        
        do{
          avgVal = (avgVal + smallCap())/2;
          ++samples;
        }
        while(millis()-start<SMALLCAP_SAMPLINGTIME);
        
        val = avgVal;
      
        capacitance = avgVal * IN_CAP_TO_GND / (float)(MAX_ADC_VALUE - avgVal);
        
        float displayedCapacitance = capacitance - nullCapacitance;
        
        Serial.print(F("Capacitance Value = "));
        printCapacitance(displayedCapacitance, SMALLCAP_DECIMALPLACES);
        Serial.print(F(" ("));
        Serial.print(avgVal);
        Serial.print(F(") "));
        
        Serial.print(F(" Samples: "));
        Serial.print(samples);
        
        
        Serial.print(F(" Null: "));
        printCapacitance(-nullCapacitance, SMALLCAP_DECIMALPLACES);
        
        Serial.println("");
    }
    else
    {
      //Big capacitor - so use RC charging method
      bigCap();
    }
    
    while (millis() % 100 != 0);    
    
}


int smallCap(){
  
      pinMode(IN_PIN, INPUT);
      digitalWrite(OUT_PIN, HIGH);
      int val = analogRead(IN_PIN);
      digitalWrite(OUT_PIN, LOW);
      
      
      if (val < 1000){
        //Clear everything for next measurement
        pinMode(IN_PIN, OUTPUT);
      }
      
      return val;
}

void bigCap(){

 //Big capacitor - so use RC charging method
      
      unsigned long start = millis();
      
      int samples = 0;
      float avgVal = 0;
      
      unsigned long u1;
      unsigned long u2;
      
      int digVal;
      
      Serial.print(F("before reading "));
        Serial.println(volts(analogRead(OUT_PIN)));
      
      do{
        
        //discharge the capacitor (from low capacitance test)
        pinMode(IN_PIN, OUTPUT);
        delay(1);
  
        //Start charging the capacitor with the internal pullup
        pinMode(OUT_PIN, INPUT_PULLUP);
        u1 = micros();
        
        //Charge to a fairly arbitrary level mid way between 0 and 5V
        //Best not to use analogRead() here because it's not really quick enough
        
        double Vmin = readVcc();
        
        do{
          digVal = digitalRead(OUT_PIN);
          u2 = micros();
          t = u2 > u1 ? u2 - u1 : u1 - u2;
          
          double Vinst = readVcc();
          Vmin = Vinst<Vmin?Vinst:Vmin;
        }
        while ((digVal < 1) && (t < 400000L));
  
        pinMode(OUT_PIN, INPUT);  //Stop charging
        //Now we can read the level the capacitor has charged up to
        v = volts(analogRead(OUT_PIN));
        
        Serial.print(F("Vref = "));
        Serial.print(Vref);
        Serial.print(F("Vmin = "));
        Serial.println(Vmin);
        
        
        Serial.print(F("After reading "));
        Serial.println(volts(analogRead(OUT_PIN)));
        
        //delay(5000);
        Serial.print(F("Discharging..."));
        
        //Discharge capacitor for next measurement
        digitalWrite(IN_PIN, HIGH);
        int dischargeTime = (int)(t / 1000L) * 5;
        delay(dischargeTime);    //discharge slowly to start with
        pinMode(OUT_PIN, OUTPUT);  //discharge remainder quickly
        digitalWrite(OUT_PIN, LOW);
        digitalWrite(IN_PIN, LOW);
        
        while(volts(analogRead(OUT_PIN))>0.01);
        Serial.print(F("Done.\n"));
        
        Serial.print(F("After DISCH long "));
        Serial.println((analogRead(OUT_PIN)));
        
        
        //Calculate and print result
        capacitance = (-(float)t / R_PULLUP / log(1.0 - v / Vref))*1000;
                  
        
        
        if(avgVal==0)                        
          avgVal = capacitance;
        else
          avgVal = (avgVal + capacitance) / 2;
        
        ++samples;
      
      }
      while(millis()-start<LARGECAP_SAMPLINGTIME);
      
      capacitance=avgVal;
                       
      float displayedCapacitance = capacitance - nullCapacitance;

      Serial.print(F("Capacitance Value = "));
      
      printCapacitance(displayedCapacitance, LARGECAP_DECIMALPLACES);

      Serial.print(F(" ("));
      Serial.print(digVal == 1 ? F("Normal") : F("HighVal"));
      Serial.print(F(", t= "));
      Serial.print(t);
      
      Serial.print(F(" us, V= "));
      Serial.print(v);
      
      Serial.print(F("volts) Samples: "));
      Serial.print(samples);
      
      Serial.print(F(" Null: "));
      printCapacitance(nullCapacitance, LARGECAP_DECIMALPLACES);
      Serial.println("");
      
}

/* Convert ADC values to Volts */
float volts(int adcVal){
  return (float)adcVal*Vref/(float)MAX_ADC_VALUE;
}


/* Print in a readable format */
void printCapacitance(float pF, uint8_t precision){
  float value;
  char prefix;
  
  boolean neg = false;
  if(pF<0){
     pF*=-1;
     neg = true;
  }
  
  if(pF>pow(10,6)){ //uF
    value = pF/pow(10,6);
    prefix = 'u';
  }
  else if(pF>pow(10,3)){ //nF
    value = pF/pow(10,3);
    prefix = 'n';
  }
  else{ //pF
    value = pF;
    prefix = 'p';
  }
  
  if(neg) pF*=-1;
  
  Serial.print(value, precision);
  Serial.print(prefix);
  Serial.print("F");
}


void serialEvent(){
 while (Serial.available()) {
      // get the new byte:
      char cmd = Serial.read();
      
      //calibrate stray capacitance
      if(cmd=='C' || cmd=='c'){
      
        float ct = Serial.parseFloat();
        float c1 = (ct * (MAX_ADC_VALUE - val))/val;
        
        IN_STRAY_CAP_TO_GND = c1;
        IN_CAP_TO_GND = IN_STRAY_CAP_TO_GND;
        
        Serial.print("Stray cap calibrated to ");
        printCapacitance(c1, 2);
        Serial.print(" based on the known capacitor of ");
        printCapacitance(ct, 2);
        Serial.print("pF\n"); 
      }
      
      //calibrate pullup value
      if(cmd=='P' || cmd=='p'){
      
        float c = Serial.parseFloat();
        float cF= c/pow(10,6); //uF to F
        
        float tS = t/pow(10,6); //uS to S
        float r = tS/(cF*log(1-(v/Vref)))*-1;
        
        r /= 1000; //ohm to Kohms
        
        R_PULLUP = r;
        
        Serial.print("Calibrated pullup resistor to ");
        Serial.print(r);
        Serial.print("Kohms based on the known capacitor of ");
        printCapacitance(c*pow(10,6), 2);
        Serial.print("\n"); 
      }
      
      //null out the current value
      if(cmd=='N' || cmd=='n'){
        nullCapacitance = capacitance;
        Serial.print("Value Nulled to ");
        printCapacitance(nullCapacitance, 2);
        Serial.print("\n");
      }
      
      //reset the null to 0
      if(cmd=='0'){
        nullCapacitance = 0;
        Serial.println("Value Nulled to 0");
      }
     
      //help
      if(cmd=='?'){
        help();
      }
    }
}
//
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  //delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}
