#include <PID_v1.h>

bool F = false;
bool Fc = false;
bool Fi = false;
double T;
double Output;  //pid output
double tempLastSample, tempLastFilament, tempLastNoFilament, tempLastStart;


PID myPID(&T, &Output, &To, Kp, Ki, Kd, DIRECT);


uint16_t adcData;
float Uadc;
const uint16_t ADC_RESOLUTION = 1024;
const float ADC_REF_VOLTAGE = 1.0;
const float VCC = 3.3;
// TODO: add parameter to configutation
bool therm2gnd = true;


void getVoltsFromAdc() {
  adcData = analogRead(PIN_THERMISTOR);
  Uadc = ADC_REF_VOLTAGE / (float) ADC_RESOLUTION * (float) adcData;
}


float getThermistorResistance() {
  // Accordong to Wemos d1 mini schematic
  // https://www.wemos.cc/en/latest/_static/files/sch_d1_mini_v3.0.0.pdf
  getVoltsFromAdc();
  if (therm2gnd) {
    // thermistor connected to Gnd schematic
    return  (Uadc * Rd * (R1 + R2)) / (R2 * VCC - Uadc * (R1 + R2 + Rd) );
  }
  // thermistor connected to Vcc schematic
  return Rd * (VCC * R2 - Uadc * (R1 + R2)) / (Uadc * (R1 + R2 + Rd));
}



float simpleKalman(float newVal) {
  // https://alexgyver.ru/lessons/filters/
  const float _err_measure = 0.8;  // approximate noise in measures
  const float _q = 0.02;   // speed of measure, (0.001-1) change by yourself
  float _kalman_gain, _current_estimate;
  static float _err_estimate = _err_measure;
  static float _last_estimate;
  _kalman_gain = (float)_err_estimate / (_err_estimate + _err_measure);
  _current_estimate = _last_estimate + (float)_kalman_gain * (newVal - _last_estimate);
  _err_estimate =  (1.0 - _kalman_gain) * _err_estimate + fabs(_last_estimate - _current_estimate) * _q;
  _last_estimate = _current_estimate;
  return _current_estimate;
}


void getHotendTemp() {
  const uint8_t NUMSAMPLES = 10;
  const uint8_t T0 = 25;  // nominal temp for thermistor
  const uint32_t R0 = 100000;  // thermistor resistance at T0
  const uint16_t BETA = 3950;  // the Beta coefficient of the NTC
  uint8_t i;
  float samplesSumRth = 0.0;
  float adcAverage, temperature;
  float Rth;  // thermistor resistance

  // take N samples in a row, with a slight delay
  // for (i=0; i<NUMSAMPLES; i++)
  // {
  //   samplesSumRth += getThermistorResistance();
  //   delay(10);
  // }
  // Rth = samplesSumRth / (float) NUMSAMPLES;
  Rth = getThermistorResistance();

  // T1 = 1 / (ln(Rth/R0) / B + 1 / T2)
  temperature = 1 / (log(Rth / R0) / BETA + 1/(T0 + 273.15));
  temperature -= 273.15;  // convert Kelvin temp to Celsius

  // T = (double) simpleKalman(temperature);
  T = (double) temperature;
}


void start(){
    if (tempLastStart==0){
      status = "working";
      V = Vo;
      tempLastStart = millis();
      if (tempLastStart==0) tempLastStart = 1;
    }
}


void stop(){
    status = "stopped";
    V = 0;
    tempLastStart = 0;
    ifttt();
}


void initHotend(){
  myPID.SetTunings(Kp, Ki, Kd);
  myPID.SetOutputLimits(0,Max);
  pinMode(LED_BUILTIN , OUTPUT);
  pinMode(PIN_FILAMENT , INPUT);
  if (status=="") start();
}


void hotendReadTempTask() {
  if (status == "stopped" && myPID.GetMode() == AUTOMATIC){
    myPID.SetMode(MANUAL);
    Output = 0;
  }
  if (status == "working" && myPID.GetMode() != AUTOMATIC){
    myPID.SetMode(AUTOMATIC);
  }
  if (millis() >= tempLastSample + 100)
  {
    getHotendTemp();
    if (T > Tm || isnan(T)){
      Output = 0;
    } else {
      myPID.Compute();
    }
    if (status == "working"){
      start();
      if (T > 150 || T > To + 20 ) {
        // target temperature ready
        digitalWrite(LED_BUILTIN , LOW);
      } else {
        // reaching tarjet temp
        digitalWrite(LED_BUILTIN , !digitalRead(LED_BUILTIN));
      }
    } else {
        digitalWrite(LED_BUILTIN , HIGH);
    }

    analogWrite(PIN_HEATER, Output);

    Fc = digitalRead(PIN_FILAMENT);

    if (Fc && !F) {
      tempLastFilament = millis();
      start();
    }

    if (!Fc && F) {
      tempLastFilament = 0;
      tempLastNoFilament = millis();
    }

    F = Fc;

    if (Fc && tempLastFilament > 0 && millis() >= tempLastFilament + 3*1000){
      Fi = true;
    }

    if (!Fc && Fi && tempLastNoFilament > 0 && millis() >= tempLastNoFilament + 500) { // no filament
      stop();
      tempLastNoFilament = 0;
      Fi = false;
    }

    if (!Fc && !Fi && tempLastStart > 0 && millis() >= tempLastStart + 5*60*1000) { // no filament for 5 min
      stop();
    }

    tempLastSample = millis();
  }
}
