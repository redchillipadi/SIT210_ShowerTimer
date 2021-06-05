#!/usr/bin/env python3
from datetime import datetime, timedelta
import requests

# This file is run every hour by cron.hourly and checks the last hour of the log file for leaks and low battery status (< 9V)
# It then emails the administrator if issues were detected

LOGFILE = "/var/log/shower"
DURATION = 3600

# API key for sending data to IFTTT
KEY = "pVB7OrWxiV6nd-7Kvz4m5"

# Global variables used to store state
volume = 0	# The volume of unexpected water detected
battery = 99	# Dummy value to ensure that the first update to the battery overwrites this value

# Check the values from the reading in the log
# Params:
#   timestamp - the time of the log entry
#   input - The reading portion of the log entry (space separated temp, flow, voltage and solenoid state)
# Returns: Nothing
#
# Increment the volume of water detected (if the flows occur while the solenoid is closed)
# and save the lowest battery voltage detected (if it is lower than the current lowest reading)
def check_values(timestamp, input):
	readings = input.split(' ')
	flow = float(readings[0])
	volts = float(readings[1])
	solenoid = None
	if readings[2] == 'Open':
		solenoid = True
	elif readings[2] == 'Closed':
		solenoid = False
	else:
		raise ValueError()

	if not solenoid and flow > 0:
		global volume
		volume += flow / 60.0
	global battery
	if volts < battery:
		battery = volts

# Send an alert to IFTTT
# Params:
#   event - The name of the event
#   value - The value of the parameter associated with the event
# Returns: Nothing
#
# The alert will use an IFTTT webhook to trigger an email to the administrator in the event of
# low battery (where value is the lowest voltage detected) or
# unexpected water flow (where value is the volume of water detected)
def trigger_alert(event, value):
	url = f"https://maker.ifttt.com/trigger/{event}/with/key/{KEY}"
	requests.post(url, params={"value1":f"{value}"})


# main - Process the log file and trigger alerts
# Params: None
# Returns: Nothing
#
# Open the log file and read it line by line
# All readings with the last hour are checked
# The administrator will be alterted to unexpected water flows, or low battery voltage
def main():
	input_file = open(LOGFILE, 'r')
	end_time = datetime.now()
	start_time = end_time - timedelta(seconds=DURATION)

	for line in input_file:
		try:
			sections = line[:-1].split(' - ')
			timestamp = datetime.strptime(sections[0].split(',')[0], '%Y-%m-%d %H:%M:%S')
			if timestamp >= start_time and timestamp <= end_time:
				if sections[1][:9] == 'Readings ':
					check_values(timestamp, sections[1][9:])
		except ValueError:
			pass

	global volume
	if volume > 0:
		trigger_alert("LeakDetected", volume)

	global battery
	if battery < 9:
		trigger_alert("BatteryLow", battery)

# When the script is run call the main function
if __name__ == "__main__":
	main()
