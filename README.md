# ArduDrum

ArduDrum is an Arduino program that turns your microcontroller into a drum trigger module. All you need to do is connect some piezo discs to your board (as many as you want; these will act as your drum pads) and a TCRT5000 module. 
That's it! You'll have your very own electronic drum kit made with an Arduino.

## Installation

Since this is not a library but a standalone program just copy or download the code from the 2 main versions and upload it to your Arduino. To decide which version should you choose please read further ado.

## Usage

Before using this program, you need to set up a few things in advance. First and foremost, you'll require a serial bridge to send and receive MIDI signals with your Arduino to a MIDI port. 
In our case, we'll be using [Hairless MIDI](https://projectgus.github.io/hairless-midiserial/). This program will come in handy to connect our Arduino with the MIDI port later. Ensure you have this program installed. 

Following this, you'll need to establish a MIDI port through which you can connect with Arduino via Hairless MIDI and transmit signals to this port. For this purpose, 
I prefer to use [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html). loopMIDI will create a virtual MIDI cable in our computer. Like a physical one, we can connect this cable to a DAW which will process the MIDI signals coming from the port and create the sounds 
accordingly. 

Lastly, I've used MIDI.h library to produce the MIDI signals with Arduino. So you have to install this library from the IDE in the library manager. Search for MIDI Library by Francois Best. 
![image (1) (1)](https://github.com/Bocchhi/ArduDrum/assets/148692821/45f1f31a-509e-4ddb-bf29-49c159f2d5df)

Okay now we have everything we need, let's turn our Arduino into a drumkit. To make this happen we have to choose which program you should use. ArduDrum_Time and ArduDrum_Bounced both do the same thing with a small difference with sensing the bounce.
When there is a hit detected by piezo sensors, ArduDrum_Time and ArduDrum_Bounced both scan the values of piezo for a given time to detect the peak value. This peak value later will be used to calculate the velocity of the hit. After this scan
ArduDrum_Time will wait for a fixed cooldown duration to be able to detect another hit, on the other hand ArduDrum_Bounced will wait until all the values readen from the piezo disc are lower than the threshold for a given time.
