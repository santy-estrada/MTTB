#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

#include <Servo.h>

Servo ESC;     // create servo object to control the ESC

// Program variables ----------------------------------------------------------
const int HX711_dout = 3; //mcu > HX711 dout pin
const int HX711_sck = 2; //mcu > HX711 sck pin

float measuredWeight = 0;
long cont = -1;
//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;

float Sensibilidad = 0.066; //sensibilidad en Voltios/Amperio para sensor de 30A
float I = 0.0;

unsigned long Tpotencia = 4000;
unsigned long TApotencia = millis();
unsigned long TEnvio = 500;
unsigned long TAEnvio = millis();
unsigned long Tinicio;

boolean flag = true;
int potValue = 0;  // value from the analog pin


//Separador de datos
const char kDelimiter = ',';    


// SETUP ----------------------------------------------------------------------
void setup() {
  // Initialize Serial Communication
  Serial.begin(9600);

  //Cell Bar
  LoadCell.begin();
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
  }

  // Attach the ESC on pin 9
  ESC.attach(9,1000,2000); // (pin, min pulse width, max pulse width in microseconds) 
  ESC.write(0);    // Send the signal to the ESC
  delay(10000);
  Tinicio = millis();

  
}

// START OF MAIN LOOP --------------------------------------------------------- 
void loop()
{
  // Gather and process sensor data
  unsigned long TNpotencia = millis();
  unsigned long TNEnvio = millis();
  unsigned long tiempo = millis()-Tinicio;

  if(TNpotencia - TApotencia > Tpotencia){
    if(potValue > 180){
      flag = false;
    }

    if(flag){
      if(potValue > 90){
        potValue = potValue +40;
      }else{
        potValue = potValue +30;
      }
    }else{
      potValue = potValue -60;
    }
    ESC.write(potValue);    // Send the signal to the ESC
    TApotencia = TNpotencia;
  }

  if(TNEnvio - TAEnvio > TEnvio){ 
    processSensors();
    sendDataToSerial(tiempo);
    TAEnvio = TNEnvio;
  }

}

// SENSOR INPUT CODE-----------------------------------------------------------
void processSensors() 
{
  //Cell Bar
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      measuredWeight = (LoadCell.getData() < 0)? 0: LoadCell.getData();
      newDataReady = 0;
      t = millis();
    }
  }

  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  // Current Sensor
  I = get_corriente(500);//obtenemos la corriente promedio de 500 muestras 

  
}

// Add any specialized methods and processing code below

float get_corriente(int n_muestras)
{
  float voltajeSensor;
  float corriente=0;
  for(int i=0;i<n_muestras;i++)
  {
    voltajeSensor = analogRead(A0) * (5.0 / 1023.0);////lectura del sensor
    corriente=corriente+(voltajeSensor-2.5)/Sensibilidad; //Ecuación  para obtener la corriente
  }

  corriente=corriente/n_muestras;

  if(corriente < 0.1){
    corriente = 0;
  }
  return(corriente);
}


// OUTGOING SERIAL DATA PROCESSING CODE----------------------------------------
void sendDataToSerial(long tiempo)
{
  // Send data out separated by a comma (kDelimiter)
  // Repeat next 2 lines of code for each variable sent:

  Serial.print(measuredWeight);   //Peso
  Serial.print(kDelimiter);

  Serial.print(I);                //Corriente
  Serial.print(kDelimiter);
  
  Serial.print(tiempo);                //tiempo
  Serial.print(kDelimiter);
  
  Serial.println(); // Add final line ending character only once
}
