# AtariRaider
Atari Raider - Analog Joystick for Atari 800/XL/XE Computers

Just a nigh-useless analog joystick project for Atari 8-bit computers. My work is largely based on:
* Jeff Piepmeier's [Atari POKEY Actuation with Arduino](https://jeffpiepmeier.blogspot.com/2020/02/atari-pokey-actuation-with-arduino.html) blog post. My initial tests followed this pretty closesly. He doesn't state which diode he used. So Itried a few that I had on hand and the 1N4001 worked pretty well. Unfortunately, I was having a hard time getting stable values for two axis. A single axis using delays worked well. Adding a second axis introduced jitter. Most likely due to the way I was tracking the pulse delays.
* [Stingray](https://github.com/Danjovic/Stingray) a Nintendo Classic controller adapter with keypad emulation for Atari 5200. This project handled the pulse delays in using the Arduino UNO's TIMER2 interrupt. Using this gives much better results than my delay code in loop().


# Circuit
**_IMPORTANT_: Any circuits in this project are experimental. I am not a practicing electrical engineer. The circuit diagrams I put (will put) here are what I've tested on my Ataris. The circuits work for me, but I'm _not_ an expert at hardware design. It's possible that I'm doing something incorrectly that may damage the computer. USE THIS PROJECT AT YOUR OWN RISK**

* Arduino UNO R3 (tested on a clone. Will try Nano V3 clones soon)
* 1N4001 Diode
* R1 - 100 Ohm
* R2 - 10 kOhm
* R3 - 2.2 kOhm ,  R4 - 560 Ohm ( I don't have 2.7k )
* C1,C2 22nF capacitors




TODO: relearn KiCad and make a proper schematic.

PADDLE 0 POTGO check + PADDLE 0 PULSE
PADDLE 1 PULSE:
```txt
Atari Port 1 Pin 9 (POT X1) <------------------------------
                               |                          |
                        [ 100Ω Resistor ]          [ 1N4001 Diode ]
                               |                    (Cathode ==|<-- Anode) (stripe facing atari)
                               |                          |
        Arduino Pin D6 <-------+                          +-> Arduino Pin D3

Atari Port 1 Pin 5 (POT Y1) <--- [ 1N4001 Diode (Cathode --|<-- Anode) ] <-- Arduino Pin D4 (stripe facing atari)


       POT LINE (5V) ---+   
                        |
                        R 2.7k  (actually used 2.2k + 560 ~2.76k)
                        |
 D7 ------------------------ <- Reference Voltage (e.g., ~3.8V) 10k/(2.7k +10k )
                        |
                        R 10k 
                        |
                        v   
                       0V

```



