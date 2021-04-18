// Consider using hardware serial if flashing over USB port is not required
// This is better than SoftwareSerial as it allows simultaneous bidirectional communication and has lower interrupt latency
// but it can only use pins D8 and D9, and consumes D10 as a PWM timer.
// It is not compatible with ATtiny85 (which lacks the 16 bit timer), so miniturisation attempts will need ATMega328P chip.
// #include <AltSoftSerial.h>
// AltSoftSerial btSerial;

const int SOLENOID_INPUT_A = 12;
const int SOLENOID_INPUT_B = 13;
bool solenoid_open;
int solenoid_count;

// On the uno, attachInterrupt only works on pins 2 or 3
const int FLOW_PIN = 2;
volatile int flow_count;

const int LM335_PIN = A0;
// The GPIO output is divided by a 2.2k resistor and the chip,
// and calibrated using a trimpot to the adj. leg to account for variations in supply voltage
// Another possibility would be to use a reference voltage chip

const int VOLTAGE_PIN = A1;
// The input voltage is divided by a 30k and 7k5 resistor
// This gives 2.37V for 12V  and 1.00 V for 5V
// At 12V it draws 0.3 mA
const float REFERENCE_VOLTAGE = 2.5;
const float ADC_RESOLUTION = 1024;


// With the Arduino Uno, AltSoftSerial requires D8 connected to TXD, and D9 connected to RXD on the bluetooth module
// but using Hardware Serial requires D0 connected to TXD and D1 connected to RXD on the bluetooth module

void setup() {
  // TODO: The HM-10 can support 115200, and the Serial library probably can't support the maximum of 230400
  // according to http://www.martyncurrey.com/hm-10-bluetooth-4ble-modules/
  Serial.begin(9600);
  // Commands are sent at 9600 baud regardless of the baud rate.
  // The rate is set by sending AT+BAUDx (where x=0 for 9600, 4 for 115200 or 8 for 230400)
  // and the device should return AT+Set:x
  // Then flush the command, change to the correct speed and discard any received junk in the buffer
  // But something is not quite right as the phone only prints junk

  solenoid_count = 0;
  solenoid_open = false;
  digitalWrite(SOLENOID_INPUT_A, LOW);
  digitalWrite(SOLENOID_INPUT_B, LOW);
  pinMode(SOLENOID_INPUT_A, OUTPUT);
  pinMode(SOLENOID_INPUT_B, OUTPUT);
  writeSolenoid(solenoid_open);

  pinMode(LM335_PIN, INPUT);
  pinMode(VOLTAGE_PIN, INPUT);
  analogReference(EXTERNAL);

  flow_count = 0;
  pinMode(FLOW_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), measureWaterFlow, RISING);
}

// Send a 20 ms pulse to SOLENOID_INPUT_B to close, or SOLENOID_INPUT_A to open
// Then leave both pins LOW to prevent drawing power while the latched solenoid maintains the open or closed state
void writeSolenoid(bool Open)
{
  digitalWrite(SOLENOID_INPUT_A, Open ? LOW : HIGH);
  digitalWrite(SOLENOID_INPUT_B, Open ? HIGH : LOW);
  delay(20);
  digitalWrite(SOLENOID_INPUT_A, LOW);
  digitalWrite(SOLENOID_INPUT_B, LOW);
}

void measureWaterFlow()
{
  flow_count++;
}

// Call this once per second to get the flow rate in L/sec
float readFlowRate()
{
  float volume = (float)flow_count / 450.0;
  flow_count = 0;
  return volume;
}

// A count of 0 = 0 Kelvin = -273.15 C.
// The voltage across the sensor increases by 0.01V/C, and should be 2.9815V at 25 C = 298.15 K
// This is passed through a voltage divider so that only half the value is read by the arduino
// This means the arduino pin should receive 1.49075 V at 298.15 K
// Using the reference voltage of 2.94V (+/- 4%), which is equivalent to a count of 1024
// this gives a nominal 0.002431642 Volts per count at the arduino pin, and so
// 25C = 298.15K = 2.9815 V at the sensor = 1.49075V at the ardino pin = a count of 613.06107151
// Converting backwards, a count of 613.06107151 = 298.15 K = 0.4863284 K/count
float readTemperature()
{
  float count = analogRead(LM335_PIN);
  float temperature = count * 100.0 * 2.0 * REFERENCE_VOLTAGE / ADC_RESOLUTION - 273.15;
  return temperature;
}

// The input to the voltage pin is divided using a voltage divider setup of a 30k and 7k5 resistor, which divides the input voltage by 5
// When the supply voltage is 12.45 V this will give the reference voltage of 2.49 V (+/- 4%), which is equivalent to a count of 1024
// and a count of 0 represents 0V, giving a nominal 0.012158203 volts per count
float readVoltage()
{
  float count = analogRead(VOLTAGE_PIN);
  float voltage = count * 5.0 * REFERENCE_VOLTAGE / ADC_RESOLUTION;
  return voltage;
}

void loop() {
  float temperature = readTemperature();
  float flow = readFlowRate();
  float voltage = readVoltage();

    // If we don't use the external reference, correct for the measured voltage
  //temperature = ((temperature + 273.15) * voltage / 5.0) - 273.15;
  
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'O') {
      solenoid_open = true;
      writeSolenoid(solenoid_open);
    } else if (c == 'C') {
      solenoid_open = false;
      writeSolenoid(solenoid_open);
    }
  }

  char buffer[57];
  char tempString[7];
  char flowString[8];
  char voltageString[7];
  // Using this floating point adds about 3k program storage space
  // and buffer space. Rewrite this to use a better protocol
  dtostrf(temperature, 3, 1, tempString);
  dtostrf(flow, 2, 3, flowString);
  dtostrf(voltage, 2, 2, voltageString);
  
  sprintf(buffer, "Temp %s C | Flow %s L/s | Volts %s V | Solenoid %s", tempString, flowString, voltageString, solenoid_open ? "Open" : "Closed");
  Serial.println(buffer);

  delay(1000);
}
