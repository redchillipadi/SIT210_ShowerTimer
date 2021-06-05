#include <avr/sleep.h>

// PIN CONNECTIONS
const int FLOW_PIN = 2;             // Signals from the Flow sensor are monitored on pin D2 (Int 0)
const int BLUETOOTH_PIN = 3;        // Signals from the Bluetooth module are monitored on pin D3 (Int 1)
const int SOLENOID_INPUT_A = 12;    // Control for the motor driver for the solenoid are from pins D12 and D13
const int SOLENOID_INPUT_B = 13;    // Setting one high and the other low applies a positive or negative voltage to the solenoid
const int VOLTAGE_PIN = A0;         // Monitor the battery voltage which is connected to A0

// GLOBAL VARIABLES FOR STORING STATE
volatile int flow_count;      // Stores the number of water flow signals received. Each is 1/450 L.
bool solenoid_open;           // Record whether the solenoid is currently open
unsigned long sensorStart;    // The time the sensors were last read (used to determine when to read again)
unsigned long showerStart;    // The time the shower was started (used to determine when to close the solenoid)
unsigned long watchdogStart;  // The time the last control signal was received (used to determine when to sleep)

// CONSTANTS
const float REFERENCE_VOLTAGE = 2.49;                             // The reference voltage to use (internal is 5V, external is calibrated to 2.49V)
const float ADC_RESOLUTION = 1024.0;                              // The number of gradations of the ADC.
const float VOLT_VOLTAGE_DIVIDER = 30000.0/(30000.0+200000.0);    // The resistor divider which reduces the voltage from the battery
const float PULSES_PER_LITRE = 450.0;                             // The number of pulses from the flow sensor per litre of flow
const unsigned long SOLENOID_PULSE_DURATION = 20;                 // Change the solenoid position with a 20 millisecond pulse
const unsigned long SENSOR_READ_INTERVAL = 1000;                  // Read the sensors every second (value in milliseconds)
const unsigned long SHOWER_DURATION   = 240000;                   // The shower will lock 4 minutes after being started (value in milliseconds)
const unsigned long WATCHDOG_DURATION = 10000;                    // If the solenoid is closed, the device will sleep after 10 seconds of inactivity (value in milliseconds)

// setup - Set up the ATmega328P, with global variables and pins in a known state
// Params: None
// Returns: Nothing
//
// The solenoid will be closed, and the timers reset so the sensors are ready to be read,
// and the system will go to sleep after 5 minutes of inactivity
void setup() {
  Serial.begin(9600); // For transferring readings every second, the default baud rate is sufficient

  solenoid_open = false;                  // On boot, ensure the solenoid is closed
  digitalWrite(SOLENOID_INPUT_A, LOW);    // Put the solenoid controller pins into a known state
  digitalWrite(SOLENOID_INPUT_B, LOW);
  pinMode(SOLENOID_INPUT_A, OUTPUT);      // and have them ready for output
  pinMode(SOLENOID_INPUT_B, OUTPUT);
  writeSolenoid(solenoid_open);           // Send a signal to the solenoid so it also is in a known state

  // Setting analogReference to EXTERNAL must be called before reading from the analog pins to prevent a short circuit
  pinMode(VOLTAGE_PIN, INPUT);            // The system will read the divided battery voltage from the voltage pin
  analogReference(EXTERNAL);              // and use the external voltage reference for calibration

  flow_count = 0;                         // On boot, the measured flow is 0L
  pinMode(FLOW_PIN, INPUT);               // Use the pin for input, and register the interrupt for handling flow signals.
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), measureWaterFlow, RISING);

  pinMode(BLUETOOTH_PIN, INPUT);          // Similarly, set the Bluetooth monitor pin to input and ensure it is low.
  digitalWrite(BLUETOOTH_PIN, LOW);

  sensorStart = millis() - SENSOR_READ_INTERVAL;  // The sensors are ready to be read again now
  showerStart = millis() - SHOWER_DURATION;       // Set the shower start time to 4 minutes before now, which corresponds to the state where the solenoid is closed.
  watchdogStart = millis() - WATCHDOG_DURATION;   // Set the watchdog start time so that the system will go to sleep until it is woken by the controller.
}


// writeSolenoid - Set the solenoid valve to the desired state
// Params: Open - a boolean that represents whether the solenoid should be opened (otherwise it is closed)
// Returns: Nothing
//
// Send a pulse to SOLENOID_INPUT_B to close, or SOLENOID_INPUT_A to open
// Then leave both pins LOW to prevent drawing power while the latched solenoid maintains the open or closed state
void writeSolenoid(bool Open)
{
  digitalWrite(SOLENOID_INPUT_A, Open ? LOW : HIGH);
  digitalWrite(SOLENOID_INPUT_B, Open ? HIGH : LOW);

  // Wait to allow time for the solenoid valve to change its position
  float startTime = millis();
  while ((millis() - startTime) < SOLENOID_PULSE_DURATION);
  
  digitalWrite(SOLENOID_INPUT_A, LOW);
  digitalWrite(SOLENOID_INPUT_B, LOW);
}

// measureWaterFlow - Interrupt handler to record another signal from the flow sensor
// Params: None
// Returns: None
//
// The interrupt handler is called each time 1/450 L of flow is detected.
// Increment the counter each time. This will be read and reset in the main loop each second.
void measureWaterFlow()
{
  flow_count++;
}


// readFlowRate - Get the volume of water which has been measured since the last call
// Params: None
// Returns: None
//
// Read the current value of the flow count variable, and convert it to Litres
// If this is called every second, the reading is in L/sec.
float readFlowRate()
{
  float volume = (float)flow_count / PULSES_PER_LITRE;
  flow_count = 0;
  return volume;
}


// readVoltage - Return the battery voltage in Volts
// Params: None
// Returns: the calculated voltage
//
// First undo the effect of the resistor divider to get the original voltage,
// then convert the count to a voltage using the reference voltage and ADC resolution
float readVoltage()
{
  float count = analogRead(VOLTAGE_PIN);
  float voltage = count / VOLT_VOLTAGE_DIVIDER * REFERENCE_VOLTAGE / ADC_RESOLUTION;
  return voltage;
}

// loop - Perform regular processing while the ATmega328P is awake
// Params: None
// Returns: Nothing
//
// There are a number of tasks:
// - Read the sensors every second and send them to the Raspberry Pi via Bluetooth
// - Respond to commands from the raspberry pi
// - Turn the solenoid off after the shower has lasted 4 minutes
// - and go to sleep after 5 minutes
void loop() {
  unsigned long currentTime = millis();

  // Check if it is time to turn off the shower or go to sleep for inactivity
  // The method of comparison used will still function correctly when millis overflows once every 50 days
  if (solenoid_open && (currentTime - showerStart) >= SHOWER_DURATION)
  {
    solenoid_open = false;
    writeSolenoid(solenoid_open);
    watchdogStart = currentTime;
  }
  if (!solenoid_open && (currentTime - watchdogStart) >= WATCHDOG_DURATION)
  {
    goToSleep();
  }

  // Check for the open command from the Raspberry Pi
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'o') {
      solenoid_open = true;
      writeSolenoid(solenoid_open);
      showerStart = currentTime;
      watchdogStart = currentTime;        
    }
  } 


  // Check if it is time to update the sensor values
  if ((currentTime - sensorStart) >= SENSOR_READ_INTERVAL) {
    // Read all the sensors
    float flow = readFlowRate();
    float voltage = readVoltage();

    // Send the current readings to the raspberry pi
    char buffer[54];
    char flowString[8];
    char voltageString[7];
    // Convert each floating point reading into a string
    dtostrf(flow, 2, 3, flowString);          // The flow rate is reported to 3 decimal places
    dtostrf(voltage, 2, 2, voltageString);    // The voltage is reported to 2 decimal places
    // Create and send the string via bluetooth
    snprintf(buffer, sizeof(buffer), "Flow %s L/s | Volts %s V | Solenoid %s", flowString, voltageString, solenoid_open ? "Open" : "Closed");
    Serial.println(buffer);

    sensorStart = currentTime;
  }
}


// goToSleep - Put the system to sleep
// Params: None
// Returns: Nothing
//
// Called after a period of inactivity to put the system to sleep
// It enables interrupt handlers so that it will be woken again if flow or bluetooth communication is detected
// and resets the watchdog timer on waking up so that it will not be put back to sleep immediately by the main loop
void goToSleep()
{
  sleep_enable();                       // Enable sleep mode
  // Remove the usual interrupt handler from the flow pin
  detachInterrupt(digitalPinToInterrupt(FLOW_PIN));
  // Then assign it to the wakeOnFlow handler
  // The pin is LOW when inactive and rises when a signal is detected.
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), wakeOnFlow, RISING);
  // Also assign the bluetooth monitoring pin to trigger wakeOnBluetooth
  // This pin will be HIGH when inactive and becomes LOW during transmission
  attachInterrupt(digitalPinToInterrupt(BLUETOOTH_PIN), wakeOnBluetooth, LOW);
  sei();                                // Ensure interrupts are enabled otherwise it will not be possible to wake again
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // This sleep mode uses the lowest current, but preserves the register values
  sleep_cpu();                          // Put the ATmega328P to sleep
  sleep_disable();                      // After waking up, disable the sleep mode
  watchdogStart = millis();             // and restart counting the period of inactivity before it sleeps again
}


// wakeOnFlow - Called when flow is detected while the system is asleep
// Params: None
// Returns: Nothing
//
// First remove the wake interrupt handlers and restore the usual flow measurement interrupt
// Then record the water flow
void wakeOnFlow() {
  detachInterrupt(digitalPinToInterrupt(FLOW_PIN));
  detachInterrupt(digitalPinToInterrupt(BLUETOOTH_PIN));
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), measureWaterFlow, RISING);
  flow_count=1;
}


// wakeOnBluetooth - Called when bluetooth transmission is detected while the system is asleep
// Params: None
// Returns: Nothing
//
// First remove the wake interrupt handlers and restore the usual flow measurement interrupt
// The received input will be in the buffer and will be handled by the main loop now the system is awake again
void wakeOnBluetooth() {
  detachInterrupt(digitalPinToInterrupt(FLOW_PIN));
  detachInterrupt(digitalPinToInterrupt(BLUETOOTH_PIN));
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), measureWaterFlow, RISING);
}
