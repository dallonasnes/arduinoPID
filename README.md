# arduinoPID

This code was used on an Arduino to serve as a PID temperature controller, setting the temperature and rate of temperature change for the furnace in Millikan basement used for superconductor fabrication.

This program reads the voltage across the two thermocouple leads and calibrates that to a temperature in degree C. By using the Arduino PID library, an updated analog output is computed based on the desired setpoint. The output is then converted to a digital signal to be written to the solid state relay to heat up the furnace. This is performed by alternating high and low signals to the solid state relay, the timing of which is determined by the intensity of the output variable as compared to its maximum value of 5000. The program loops until arriving at the desired setpoint, then holds the set point for the desired amount of maintenance time, after which point the solid state relay is set to low and the furnace slowly ramps back to room temperature.

Details of how to use and how to edit the program are listed below.
The program is uploaded as a C++ file but must be saved as an .ido file to be run on Arduino.

Equipment used: Arduino Uno, Adafruit MAX31856 Thermocouple Board, Adafruit RGB LCD Shield + superconductor fabrication setup & instructions located in Millikan Basement

**Be sure to test to ensure proper functionality before running at high temperatures**
**MONITOR THE TEMPERATURE READING CLOSELY**

My email: da002014@mymail.pomona.edu

How to use:

Input temperatures and desired rate of temperature increase are already specified for Heating Routine 1 and Heating Routine 2.

To select between one of those, set the selectRoutine variable on Line 33.

After that, download onto the Arduino and run.

How to edit:

You can edit the heating routine followed by the furnace by changing the appropriate values in lines 54-77. Alternatively, an additional heating routine could be created and assigned a new selectRoutine variable.

Parameters for the PID routine (line 36) are described in depth by following this link to Brett Beauregard’s website: http://brettbeauregard.com/blog/2011/04/improving-the-beginners-pid-introduction/
Kp, Ki, Kd parameters can be tuned to adjust output curve.

More info found at: https://github.com/br3ttb/Arduino-PID-Library/
				https://playground.arduino.cc/Code/PIDLibrary

Description of functions:
	heat: First, one must call the myPID.compute() function, which will generate an analog Output value. This value is then divided by 5000, the maximum possible Output variable, to rate the output as a percentage of it’s max intensity. To write this as a digital signal to the solid state relay (SSR), high is written for a period of (fraction * 100ms). Following that, a low is written for (1-fraction) *100ms.

	timeToTemp: This function is used to implement a rate of temperature change as described in the superconductor fabrication lab instructions. This is done by comparing the current setpoint to the first desired setpoint (after which the selected maintenance routine will begin). If the current setpoint is less than the next ideal temperature to be reached, the setpoint will be updated by the desired rate every minute (denoted by 60000ms). Following each of those, the updated setpoint will be used to again compute the PID output value and then re-run the heat routine.
	Throughout the timeToTemp routine, it compares the current setpoint to the desired operating temperatures. After reaching the first desired operating temperature, the setpoint is set to that operating temperature and the ideal operating temperature is changed to the subsequent operating temperature. To ensure that a program is not run twice for the first ideal operating temperature, the hasItRun variable is used. HasItRun is initialized to zero and, after the temperature reaches the first ideal operating temperature, it is set to 1, indicating that the maintenance routine has already been run for that temperature and it is now time to move toward the second ideal operating temperature. After passing all of the comparisons, the program will then call the reduceToRoomTemp function which sets the SSR to off and allows the furnace to slowly return to room temperature.

	maintainTemp: This program maintains the operating temperature of the furnace, plus/minus the amount of oscillations of the program. The fluctuations in temperature may be significant (roughly 20 °C each direction in my case). Perhaps that can be improved by adjusting the Kp, Ki and Kd variables. The timer function is performed by grabbing the time at the beginning of the maintainTemp function and right before entering the while loop. If the time has been less than the desired maintenance time in minutes, a new PID output value is computed, the heat routine is called, and then the time is again grabbed. The two times are continually compared so as to not entire the while loop when the total time period (in ms) exceeds the maintenance parameter in minutes.

	reduceToRoomTemp: This function writes a low to the SSR and holds it there, allowing the furnace to slowly decrease to room temperature.
