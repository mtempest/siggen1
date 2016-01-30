# siggen1
A signal generator, based on ATMEGA8

It can output square waves, triangle waves or sine waves.

OCP1A provides the output. Square waves are generated directly using the PWM function. You can adjust the duty cycle.
Triangle and sine waves are produced using PWM at 64 times the fundamental. 
Use an external RC filter when generating triangle and sine waves.

For square waves, the range is 0.25 Hz to 4 MHz. For triangle and sine waves, the range is 0.25 Hz to 62.5 kHz.

It uses a hacked-up version of Henning Karlsen's LCD5110_Graph library (which uses the CC BY-NC-SA 3.0 license).
The LCD library is too large to fit into the ATMEGA8's flash so I removed code I wasn't using until it fitted.
I also changed it to use the SPI peripheral instead of bit-banging.
Get the original version at http://electronics.henningkarlsen.com.
