#!/usr/bin/env python3
import os
import threading
import time
from datetime import datetime, timedelta
import logging
import schedule
from PWM import PWM

# The schedule of the timestamp and tunes to play
scheduled_tunes = []
# The PWM object uses channel 0 which is connected toGPIO 18
pwm = PWM(0)
# Set to True to quit the thread and service
quit = False

# Set the log file configuration
logging.basicConfig(format='%(asctime)s - %(message)s', level=logging.DEBUG, filename="/var/log/shower")


# Play a note at the given frequency (Hz) and duration (seconds)
# Use proportion to change between legato (1.0) and staccato (0.5) articulation
def play_note(frequency, duration, proportion):
	pwm.set_frequency(frequency)
	pwm.set_duty_cycle(0.5)
	time.sleep(duration * proportion)
	pwm.set_duty_cycle(0)
	time.sleep(duration * (1-proportion))


# Slide between the start and end frequency (Hz) over the given duration (seconds)
def play_slide(frequency_start, frequency_stop, duration):
	pwm.set_duty_cycle(0.5)
	for i in range(0, 1000):
		proportion = i/1000
		frequency = frequency_start - (frequency_start - frequency_stop) * proportion
		pwm.set_frequency(frequency)
		time.sleep(duration / 1001)
	pwm.set_duty_cycle(0)

# Play the starting tone
def play_start():
	logging.info("Playing Start tune")
	pwm.enable(True)
	play_note(220, 1.0, 0.9)
	play_note(220, 1.0, 0.9)
	play_note(440, 1.0, 0.9)
	pwm.enable(False)


# Play the tone when the water is warm
def play_warm():
	logging.info("Playing Water Warm tune")
	pwm.enable(True)
	play_note(440, 0.25, 0.8)
	play_note(440, 0.25, 0.8)
	time.sleep(0.5)
	play_note(440, 0.25, 0.8)
	play_note(440, 0.25, 0.8)
	pwm.enable(False)


# Play the tone when the shower has only one minute remaining
def play_nearly_done():
	logging.info("Playing Nearly Done tune")
	pwm.enable(True)
	play_note(440, 0.25, 0.8)
	play_note(440, 0.25, 0.8)
	play_note(440, 0.25, 0.8)
	play_note(440, 0.25, 0.8)
	play_note(440, 0.25, 0.8)
	pwm.enable(False)


# Play the tone when the water stops
def play_stop():
	logging.info("Playing Stop tune")
	pwm.enable(True)
	play_slide(440, 220, 1)
	pwm.enable(False)


# A dictionary mapping tune numbers to the functions that play them
tunes = {'1': play_start, '2': play_warm, '3': play_nearly_done, '4': play_stop}


# Add a tune to the schedule to play it at a given time
def schedule_tune(timestamp, tune):
	item = schedule.schedule(timestamp, tune)
	scheduled_tunes.append(item)


# Check the schedule to see if any tunes need to be played
# If so, schedule them for removal and play them
def handle_schedule():
	timestamp = datetime.now()
	to_delete = []
	for item in scheduled_tunes:
		if item.is_ready(timestamp):
			to_delete.append(item)
			play_tune(item.tune)
	for item in to_delete:
		scheduled_tunes.remove(item)


# Play the given tune (if it exists in the dictionary)
def play_tune(tune):
	func = tunes.get(tune)
	if func is not None:
		func()
	else:
		logging.error("Invalid scheduled tune")


# This thread ensures that any tunes added to the schedule are played at the appropriate time
# It needs to be run as a separate thread to the one monitoring the pipe as readline is blocking
# Check each second to see if any tunes need to be played
def handle_schedule_thread():
	while not quit:
		handle_schedule()
		time.sleep(1)


# Handle user input from the pipe
# If the enter Q, then quit this process (for debugging purposes)
# If not, then they have entered a tune number, so schedule the tune for the current time
def handle_input(input):
	if not input:
		return

	elif input == '1':
		schedule_tune(datetime.now(), '1')
		schedule_tune(datetime.now() + timedelta(minutes=3), '3')
		schedule_tune(datetime.now() + timedelta(minutes=4), '4')
	elif input == '2':
		schedule_tune(datetime.now(), '2')


def create_fifo(path):
	logging.info("Creating temporary music fifo buffer")
	os.mkfifo(path)
	os.chmod(path, 0o666)


def main():
	# Start the schedule handler thread
	handler = threading.Thread(target=handle_schedule_thread)
	handler.start()

	global quit

	try:
		logging.info("Starting the Music service")

		pwm.setup()

		# Monitor the pipe for input on the main thread, until quit is requested
		if not os.path.exists("/tmp/music"):
			create_fifo("/tmp/music")
		pipe = open("/tmp/music", 'r')

		while not quit:
			input = pipe.readline()[:-1]
			handle_input(input)

	finally:
		# Instruct the thread to quit and wait for it to do so
		quit = True
		handler.join()
		pwm.cleanup()

	logging.info("Stopping the Music service")


if __name__ == "__main__":
	main()
