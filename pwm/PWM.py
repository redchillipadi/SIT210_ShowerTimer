import os

# Add dtoverlay=pwm to /boot/config.txt and reboot enable hardware pwm in the kernel
# Depsite members of gpio group having permissions to write to the files,
# only the root user can change the period and duty_cycle
class PWM:
	# Set up the pwm driver
	def setup(self):
		# Export the pwm channel interface if it does not already exist
		if not os.path.exists("/sys/class/pwm/pwmchip0/pwm{self.channel}"):
			# Write the channel number to export to enable the pwm interface
			with open("/sys/class/pwm/pwmchip0/export", "w") as file:
				file.write(str(self.channel))
		# Write the period and duty cycle
		with open(f"/sys/class/pwm/pwmchip0/pwm{self.channel}/period", "w") as file:
			file.write(str(self.period))
		with open(f"/sys/class/pwm/pwmchip0/pwm{self.channel}/duty_cycle", "w") as file:
			file.write(str(self.duty_cycle))
		# Turn the PWM signal off initially
		self.enable(False)


	# Call with proportion [0, 1] to alter the duty cycle
	def set_duty_cycle(self, proportion):
		if proportion < 0 or proportion > 1:
			raise Exception(f"Duty Cycle Proportion {proportion} out of range")
		self.duty_cycle = round(self.period * proportion)
		with open(f"/sys/class/pwm/pwmchip0/pwm{self.channel}/duty_cycle", "w") as file:
			file.write(str(self.duty_cycle))


	# Call with frequency in Hz [1, 20000] to alter the period
	def set_frequency(self, hz):
		if hz < 1 or hz > 20000:
			raise Exception(f"Frequency {hz} Hz out of range")
		proportion = self.duty_cycle/self.period
		self.period = round(1000000000 / hz);
		if self.duty_cycle > self.period:
			# If the current duty cycle would be too large for the new period, set it lower first
			self.duty_cycle = round(self.period * proportion)
			with open(f"/sys/class/pwm/pwmchip0/pwm{self.channel}/duty_cycle", "w") as file:
				file.write(str(self.duty_cycle))
			with open(f"/sys/class/pwm/pwmchip0/pwm{self.channel}/period", "w") as file:
				file.write(str(self.period))
		else:
			# Otherwise set the period first and then adjust the duty cycle
			self.duty_cycle = round(self.period * proportion)
			with open(f"/sys/class/pwm/pwmchip0/pwm{self.channel}/period", "w") as file:
				file.write(str(self.period))
			with open(f"/sys/class/pwm/pwmchip0/pwm{self.channel}/duty_cycle", "w") as file:
				file.write(str(self.duty_cycle))


	# Call with True to enable output of the PWM signal, or False to turn it off
	def enable(self, turnOn):
		with open(f"/sys/class/pwm/pwmchip0/pwm{self.channel}/enable", "w") as file:
			file.write("1" if turnOn else "0")


	# Cleanup - turn the sound off
	def cleanup(self):
		self.enable(False)
		with open("/sys/class/pwm/pwmchip0/unexport", "w") as file:
			file.write(str(self.channel))


	# Set up default values. Writing them to the system pwm driver will be done during setup()
	def __init__(self, channel=0):
		self.channel = channel
		self.period = 10000000		# Default to 100 Hz signal
		self.duty_cycle = 0		# Default to 0% duty cycle
