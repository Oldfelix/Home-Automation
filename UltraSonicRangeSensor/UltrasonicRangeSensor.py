#!/usr/bin/python
# Measure distance using an ultrasonic module in a loop.
#
# Author : wingit2 Date : 12/17/2016 Import required Python libraries -----------------------
import time
import RPi.GPIO as GPIO

  
print ("Distance Measurement in Progress")

# Define functions -----------------------

def measure():
  GPIO.setmode(GPIO.BCM) # Use BCM GPIO references instead of physical pin numbers GPIO.setup(TRIG,GPIO.OUT)
  GPIO.setup(ECHO,GPIO.IN)

  # This function measures a distance
  GPIO.output(GPIO_TRIGGER, False)
  
  print ("Waiting For Sensor To Settle")
  time.sleep(2)
      
  GPIO.output(GPIO_TRIGGER, True)
  time.sleep(0.00001)
  GPIO.output(GPIO_TRIGGER, False)
  start = time.time()
  while GPIO.input(GPIO_ECHO)==0:
    start = time.time()
  while GPIO.input(GPIO_ECHO)==1:
    stop = time.time()
  elapsed = stop-start
  distance = (elapsed * 34300)/2
   
  return distance

  def measure_average():
   print ("This function takes 3 measurements and returns the average.")
  distance1=measure()
  time.sleep(0.1)
  distance2=measure()
  time.sleep(0.1)
  distance3=measure()
  distance = distance1 + distance2 + distance3
  distance = distance / 3
  return distance

# ----------------------- Main Script ----------------------- Define GPIO to use on Pi
GPIO.setup(TRIG,GPIO.OUT)
GPIO.setup(ECHO,GPIO.IN)


GPIO_TRIGGER = 7
GPIO_ECHO = 12

print "Ultrasonic Measurement"
# Set pins as output and input
GPIO.setup(GPIO_TRIGGER,GPIO.OUT) # Trigger GPIO.setup(GPIO_ECHO,GPIO.IN) # Echo
# Set trigger to False (Low)
GPIO.output(GPIO_TRIGGER, False)
# Wrap main content in a try block so we can catch the user pressing CTRL-C and run the GPIO cleanup function. This will also prevent the user 
# seeing lots of unnecessary error messages.
#try:
#  while True:
#
#    distance = measure_average()
#    print "Distance : %.1f" % distance
#    time.sleep(1) 
#    except KeyboardInterrupt:
#
    # User pressed CTRL-C Reset GPIO settings
#
GPIO.cleanup()
