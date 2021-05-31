#include <avr/sleep.h>

// The HM-10 Bluetooth 4.0 module is connected with
// VCC to 5V
// GND to Ground
// TXD connecting to RXD on the ATmega328P,
// RXD connecting to TXD (pin 1 on the ATmega328P), and also via a 220 Ohm resistor to INT1 (pin 3) to enable wake on Bluetooth.

const int BLUETOOTH_PIN = 3;  // The pin which is monitored for Bluetooth interrupts

// Use hardware serial for Bluetooth communication, which is better than Software Serial and altSoftwareSerial
// as it allows simultaneous bidirectional communication and has lower interrupt latency.

// On the ATmega328P, attachInterrupt only works on pins 2 or 3
const int FLOW_PIN = 2;       // The pin to monitor for water flow signals (each represents 1/450 L)
volatile int flow_count;      // Store the number of signals received in the last second

// The LM9110 is connected with
// Solenoid Input A to pin 12 on the ATmega328P.
// Solenoid Input B to pin 13 on the ATmega328P.

// The solenoid is connected with the lead closest to the water source to Pin 1, and the lead furthest away to Pin 4.
const int SOLENOID_INPUT_A = 12;
const int SOLENOID_INPUT_B = 13;
bool solenoid_open;
int solenoid_count;

const int LM335_PIN = A0;
// The GPIO output is divided by a 2.2k resistor and the chip,
// and calibrated using a trimpot to the adj. leg to account for variations in supply voltage
// Another possibility would be to use a reference voltage chip

const int VOLTAGE_PIN = A1;
// The input voltage is divided by a 30k and 7k5 resistor
// This gives 2.37V for 12V  and 1.00 V for 5V
// At 12V it draws 0.3 mA
const float REFERENCE_VOLTAGE = 2.5;
const float ADC_RESOLUTION = 1024.0;
const float TEMP_VOLTAGE_DIVIDER = 2200.0/(2200.0+820.0);

const long SHOWER_DURATION   = 240000; // 4 minutes in milliseconds
const long WATCHDOG_DURATION = 300000; // 5 minutes in milliseconds
unsigned long showerStart;
unsigned long watchdogStart;

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

  pinMode(BLUETOOTH_PIN, INPUT);
  digitalWrite(BLUETOOTH_PIN, LOW);

  showerStart = millis() - SHOWER_DURATION;
  resetWatchdog();
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
  float temperature = count * 100.0 / TEMP_VOLTAGE_DIVIDER * REFERENCE_VOLTAGE / ADC_RESOLUTION - 273.15;
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

  unsigned long currentTime = millis();
  if (solenoid_open && (currentTime - showerStart) > SHOWER_DURATION)
  {
    solenoid_open = false;
    writeSolenoid(solenoid_open);
  }
  if ((currentTime - watchdogStart) > WATCHDOG_DURATION)
  {
    goToSleep();
  }
  
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'O') {
      // Open the solenoid and start countdown
      solenoid_open = true;
      writeSolenoid(solenoid_open);
      setShowerCountdown();
      resetWatchdog();
    } else if (c == 'C') {
      // Close the solenoid
      solenoid_open = false;
      writeSolenoid(solenoid_open);
    } else if (c == 'W') {
      // Wake from sleep command
      // wakeOnBluetooth has already woken the arduino up
      // and called resetWatchdog so there is nothing left to do here
    } else if (c == 'S') {
      // Go to sleep now for testing purposes
      goToSleep();
    }
    // Invalid entries are silently ignored
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

void setShowerCountdown()
{
  showerStart = millis();
}

void resetWatchdog()
{
  watchdogStart = millis();
}

// Wake on bluetooth needs connecting pin 2 (RXD) to pin 5 (INT 1)

// The system draws 24.5 mA when connected with only the atmega328p chip. Using the arduino uno draws 32 mA
// The system spikes to about 140 mA on the meter during solenoid use but as it is a brief pulse I think this is inaccurate. It should use about 800 mA according to valves direct.
// According to https://rightbattery.com/118-1-5v-aa-duracell-alkaline-battery-tests/ I can expect each battery to give 2348 mAh at 100 mA load (this is longer than at higher loads)
// So 8 batteries should provide 12 V initially, and drop to 8 V over 767 hours, so the batteries should need replacement about once a month.

// Duracell also has rechargeable 2500 mAh batteries, which perhaps only provide 1.2V like other rechargeable batteries.
// If so they would output 9.6 v when fresh, dropping to 7.68v over 816 hours, which is 34 days.
// This would allow two sets to be interchanged for uninterrupted operation once a month

void goToSleep()
{
  // Enable sleep mode
  sleep_enable();
  detachInterrupt(digitalPinToInterrupt(FLOW_PIN));
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), wakeOnFlow, RISING);
  attachInterrupt(digitalPinToInterrupt(BLUETOOTH_PIN), wakeOnBluetooth, LOW);
  sei();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_cpu();
  sleep_disable();
  resetWatchdog();    // After wake up, reset the watchdog
}

void wakeOnFlow() {
  detachInterrupt(digitalPinToInterrupt(FLOW_PIN));
  detachInterrupt(digitalPinToInterrupt(BLUETOOTH_PIN));
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), measureWaterFlow, RISING);
  flow_count=1;
}

void wakeOnBluetooth() {
  detachInterrupt(digitalPinToInterrupt(FLOW_PIN));
  detachInterrupt(digitalPinToInterrupt(BLUETOOTH_PIN));
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), measureWaterFlow, RISING);
}
