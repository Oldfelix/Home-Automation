## Convert RadioShack's Three Pin Ultrasonic Range Sensor into a Cheaper Four Pin System 
 
 If you have recently purchased RadioShack's popular Ultrasonic Range Sensor for thirty dollars then go find your receipt and return it because there are much cheaper options out there. For example this HC-SR04 Ultrasonic Distance Sensor is almost identical, except it's only six dollars.
 Radioshack's sensor uses 3 pins to connect to your Arduino while cheaper brands use 4 pins and often do not come with any support files. This tutorial will show you how to convert the code from Radioshack's 3 pin system into a code for a 4 pin system.

 
## 1.   The RadioShack sensor comes with the code already written to make the hardware work.
		However, the code that they give you is long and difficult for most beginners to 
		understand so I've written a simpler code that might make understanding it easier.

```/*************************************
Ultrasonic Range Sensor 3pin 
Sean Poulter
Created: 12/21/13
Last Updated: 1/1/14
*************************************/

const int sig = 4;

void setup(){
  
  Serial.begin(9600);
}
  
  void loop() {
    
    long duration, inches, cm;
  
  pinMode(sig, OUTPUT);  
  digitalWrite(sig, LOW);
  delay(2);
  digitalWrite(sig, HIGH);
  delay(10);
  digitalWrite(sig, LOW);
  
  pinMode(sig, INPUT);
  duration = pulseIn(sig, HIGH);
  
  inches = microsecondsToInches(duration);
  cm = microsecondsToCentemeters(duration);
  
  Serial.print(inches);
  Serial.print(" in, ");
  Serial.print(cm);
  Serial.print(" centemeters");
  Serial.println();
  delay(100);
  }
  
  long microsecondsToInches(long microseconds) {
    
    return microseconds / 74 /2;
  }
  
  long microsecondsToCentemeters(long microseconds) {
    
    return microseconds / 29 / 2;
  }
```
## 2.	Once you've looked at the problem from a new, and simpler, 
		perspective you'll find that adding the additional pin is remarkably easy. 
		You just need to understand the differences in hardware.
      
	- In the three pin system, the signal pin is used to send pulses to the microphone. In the code this is shown by...
```
pinMode(sig, OUTPUT);  
  digitalWrite(sig, LOW);
  delay(2);
  digitalWrite(sig, HIGH);
  delay(10);
  digitalWrite(sig, LOW);
```
	- The same pin, "sig", is also used to read the pulses back from the return microphone. 
	This is shown directly after the above code with...

```pinMode(sig, INPUT);
  duration = pulseIn(sig, HIGH);
```
	- You may have seen it by now, the difference between the 3 pin system and the 4 pin is simple. 
	- In the 3 pin system the sig pin operates both the "out" and "in" pulses, while in the 4 pin system 
		the "out pulses" are sent through the trig pin and the "in pulses" are sent through the echo pin. 
		(That's probably why the 4 pin system is so much cheaper.) Once we know the differences, we can 
		modify the above two sections of code.
        
## 3.  Any variable under "pinMode" output has to be modified to "trig" and any variable under 
		"pinMode" input has to be modified to "echo". The final code becomes...

       
```/************************************

Ultrasonic Range Sensor 4 Pin

Sean Poulter
Created: 12/21/13
Last Updated: 1/1/14
************************************/
```
```const int Trig = 2;
const int Echo = 4;
```
```void setup(){
  pinMode(Trig, OUTPUT);
  pinMode(Echo, INPUT);
  Serial.begin(9600);
}
```
```  void loop() {
    
    long duration, inches, cm;
    
  digitalWrite(Trig, LOW);
  delay(2);
  digitalWrite(Trig, HIGH);
  delay(10);
  digitalWrite(Trig, LOW);
  
  duration = pulseIn(Echo, HIGH);
  
  inches = microsecondsToInches(duration);
  cm = microsecondsToCentemeters(duration);
```  
  Serial.print(inches);
  Serial.print(" in, ");
  Serial.print(cm);
  Serial.print(" centemeters");
  Serial.println();
  delay(100);
  }
  
  long microsecondsToInches(long microseconds) {
    
    return microseconds / 74 /2;
  }
  
  long microsecondsToCentemeters(long microseconds) {
    
    return microseconds / 29 / 2;
  }
```
## 4. Any problem you come across can be broken down into simpler parts, 
	as was done here. Developing strong problem solving skills is the key to 
	success in programming, engineering, and all of the academic world.

