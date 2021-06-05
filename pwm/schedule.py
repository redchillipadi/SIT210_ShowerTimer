# Class to store the schedule of upcoming music to play
# It stores the timestamp when it should be activated, and the number of the tune to play
# By calling is_ready with the current timestamp, it will return True if the tune should be played

class schedule:
  def is_ready(self, timestamp):
    return timestamp >= self.timestamp

  def __init__(self, timestamp, tune):
    self.timestamp = timestamp
    self.tune = tune
