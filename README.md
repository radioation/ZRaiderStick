**_IMPORTANT_**: This is still a work in progress. I'm occasionally seeing it get stuck and I'm still sorting that out. Expect changes.

# Z Raider Stick
Z Raider - Analog Joystick for Atari 800/XL/XE Computers

Just a nigh-useless analog joystick project for Atari 8-bit computers. It's meant to 
work with my [port](https://github.com/radioation/StarRaidersMods) of the Atari 5200 
Star Raiders analog controls to the 8-bit version. 

Z Raider may work with other computers and consoles but I've not tested this. I've done a little work with the C64, but it's not ready yet.


My work is largely based on:
* Jeff Piepmeier's [Atari POKEY Actuation with Arduino](https://jeffpiepmeier.blogspot.com/2020/02/atari-pokey-actuation-with-arduino.html) blog post. My initial tests followed this pretty closesly. He doesn't state which diode he used. So I tried a few that I had on hand and the 1N4001 worked pretty well. Unfortunately, I was having a hard time getting stable values for two axis. A single axis using delays worked well. Adding a second axis introduced jitter. Most likely due to the way I was tracking the pulse delays.
* Danjovic's [Stingray](https://github.com/Danjovic/Stingray). This is Nintendo Classic controller adapter with keypad emulation for the Atari 5200. This project handled the pulse delays in using the Arduino UNO's TIMER2 interrupt. Using this technique gives much better results than my delay code in loop().


# Circuit
**_IMPORTANT_: Any circuits in this project are experimental. I am not a practicing electrical engineer. The circuit diagrams I put (will put) here are what I've tested on my Ataris. The circuits work for me, but I'm _not_ an expert at hardware design. It's possible that I'm doing something incorrectly that may damage the computer. USE THIS PROJECT AT YOUR OWN RISK**

* Arduino UNO R3 or Nano V3. I've tested on Elegoo clones.
* D1, D2 - 1N4001 Diode
* D3 - 1N5817 Schottky Diode
* R1 - 100 Ohm
* R2 - 10 kOhm
* R3 - 2.7 kOhm ( I used 2.2k + 560 ~= 2.76kOhm. The main thing is to get a voltage divider in this range for the comparator )
* C1,C2 - 22nF
* RV1, RV2 - 10 kOhm ( basic thumbstick, I'm currently looking at RKJXV1224005 thumbsticks in KiCAD )

  
Atari 800/XL/XE Schematic:
![ZRaider schematic](images/ZRaider_schematic.png)
