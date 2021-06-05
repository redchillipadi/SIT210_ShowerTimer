#!/usr/bin/env python3
import pygatt
from pygatt.exceptions import NotConnectedError
import os
import sys
import logging

# Bluetooth communication occurs on handle 0x12 (18 decimal), using the given MAC address and UUID
HANDLE = 18
MAC = "90:e2:02:a0:2f:7f"
UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"

# The gatt object stores the connection to the pygatt library (and underlying gatttool process)
gatt = pygatt.GATTToolBackend();
# Buffer to store unprocessed input between interrupts
input_buffer = ""

# Set the log file configuration
logging.basicConfig(format='%(asctime)s - %(message)s', level=logging.DEBUG, filename="/var/log/shower")


# play_tune - Schedule the given tune
# Params: None
# Returns: Nothing
#
# Write the command to the music service
def play_tune(tune):
	if not os.path.exists("/tmp/music"):
		os.mkfifo("/tmp/music")
	pipe = open("/tmp/music", "a")
	pipe.write(f"{tune}\n")
	pipe.close()


# process_line - Process the data corresponding to one line of serial bluetooth communication
# Params:
#   line - the line to be processed
# Returns: Nothing
#
# Each line should contain the readings for the temperature, flow, voltage and solenoid state separated by |
# Convert them to variables and send them to the log file
#
# If the water has just become warm, signal the music service to play the warm water sound and set the variable
# so that it will not be played again for this shower
def process_line(line):
	readings = line.split('|')
	flow = None
	volts = None
	solenoid = None
	for reading in readings:
		if reading[:5] == ' Flow':
			flow = float(reading[6:].split(' ')[0])
		if reading[:6] == ' Volts':
			volts = float(reading[7:].split(' ')[0])
		if reading[:9] == ' Solenoid':
			if reading[10:14] == 'Open':
				solenoid = "Open"
			elif reading[10:16] == 'Closed':
				solenoid = "Closed"
	logging.info(f'Readings {flow} {volts} {solenoid}')


# process_buffer - Process the buffer containing the bluetooth communication data
# Each new line in the buffer represents a complete entry, so after a carriage return/new line
# the preceeding text can be processed
def process_buffer():
	global input_buffer
	lines = input_buffer.split('\r\n')
	if len(lines) > 1:
		for index in range(0, len(lines)-1):
			process_line(lines[index])
		input_buffer = lines[len(lines)-1]


# notification - Event handler when bluetooth communication is received from the solenoid controller
# Params:
#  handle - the GATT handle for the communication
#  values - The byte array received
#
# This function only responds to one handle, for receiving input
# The values received may not contain the full message, so subsequent entries are concatenated
# until the buffer contains enough to be processed
def notification(handle, values):
	if handle == HANDLE:
		global input_buffer
		decoded = values.decode('ascii')
		input_buffer += decoded
		process_buffer()


# subscribe - Use pygatt to create a bluetooth connection to the solenoid controller
# Raises NotConnectedError if the connection can not be established
# Subscribes so that bluetooth communication is forwarded to the event handler
def subscribe():
	gatt.start()
	global device
	device = gatt.connect(MAC)
	device.subscribe(UUID, notification)


# unsubscribe - Clean up for pygatt as the service is stopping
def unsubscribe():
	device.unsubscribe(UUID)
	gatt.stop()


# handle_input - Process the command received through the temporary file
# Params: line - the string containing the command
# Returns: Nothing
def handle_input(line):
	if line == 'O':
		# Schedule the starting, nearly done and stop tunes
		play_tune(1)
		# Start the shower - send the command to the shower-bluetooth service
		device.char_write(UUID, bytearray([0x77]))	# Wake the device with w
		device.char_write(UUID, bytearray([0x6f]))	# Send the command o


# main - The service runs this on startup
# Params: None
# Returns: Nothing
#
# Establish a connection,
# then wait for commands from the GUI service and
# respond to input from the solenoid controller
# If a connection cannot be established, quit and the service will restart
def main():
	try:
		logging.info("Starting the Shower Bluetooth service")
		# The quit global variable can be used to request the service to quit (used for debug purposes)
		global quit
		quit = False

		# Establish a connection to the solenoid controller
		subscribe()

		# Monitor the temporary file for commands from the GUI
		# First ensure it exists, then open it
		if not os.path.exists('/tmp/shower'):
			os.mkfifo('/tmp/shower')
		pipe = open('/tmp/shower', 'r')
		# Wait for and process input repeatedly
		while not quit:
			input = pipe.readline()[:-1]
			handle_input(input)
	except NotConnectedError:
		# If pygatt returns NotConnectedError, then quit the service. Systemd will restart it to reestablish a connection
		pass
	finally:
		logging.info("Stopping the Shower Bluetooth service")
		unsubscribe()


# Call the main function when the service is started
if __name__ == "__main__":
	main()

