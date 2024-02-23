# ArduDrum

ArduDrum is an Arduino program that turns your microcontroller into a drum trigger module. All you need to do is connect some piezo discs to your board (as many as you want; these will act as your drum pads) and a TCRT5000 module. 
That's it! You'll have your very own electronic drum kit made with an Arduino.

## Installation

Since this is not a library but a standalone program just copy or download the code from the 2 main versions and upload it to your Arduino. To decide which version should you choose please read further ado.

Before using this program, you need to set up a few things in advance. First and foremost, you'll require a serial bridge to send and receive MIDI signals with your Arduino to a MIDI port. 
In our case, we'll be using [Hairless MIDI](https://projectgus.github.io/hairless-midiserial/). This program will come in handy to connect our Arduino with the MIDI port later. Ensure you have this program installed. 

Following this, you'll need to establish a MIDI port through which you can connect with Arduino via Hairless MIDI and transmit signals to this port. For this purpose, 
I prefer to use [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html). loopMIDI will create a virtual MIDI cable in our computer. Like a physical one, we can connect this cable to a DAW which will process the MIDI signals coming from the port and create the sounds 
accordingly. 

Lastly, I've used MIDI.h library to produce the MIDI signals with Arduino. So you have to install this library from the IDE in the library manager. Search for MIDI Library by Francois Best. 

![image (1) (1)](https://github.com/Bocchhi/ArduDrum/assets/148692821/45f1f31a-509e-4ddb-bf29-49c159f2d5df)

Okay now we have everything we need, let's turn our Arduino into a drumkit. To make this happen we have to choose which program you should use. ArduDrum_Time and ArduDrum_Bounced both do the same thing with a small difference in sensing the bounce.
When there is a hit detected by piezo sensors, ArduDrum_Time and ArduDrum_Bounced both scan the values of piezo for a given time to detect the peak value. This peak value will later be used to calculate the velocity of the hit. After this scan
ArduDrum_Time will wait for a fixed cooldown duration to be able to detect another hit, on the other hand, ArduDrum_Bounced will wait until all the values read from the piezo disc are lower than the threshold for a given time. To make it more clear,
let's give an example. For ArduDrum_Time, if I set the cooldown length for 30ms, after the hit, the program makes sure there won't be any other trigger until the 30ms duration is completed. After the cooldown is complete you can trigger the piezo again. 
This prevents the noise and multiple triggers from one hit but it's not completely safe. For ArduDrum_Bounced if I set the cooldown length to 10ms, after the hit, the program won't trigger until all the values read from the 
piezo disc are lower than the threshold for an uninterrupted given time. Like all the values let's say have to be lower than 150 for an uninterrupted 10ms duration. This makes sure the bounce is completed in a well-mannered and safe way. 
But it sacrifices more ready time to detect hits. So, when you are using the program if you experience more triggers than expected, use ArduDrum_Bounced to ensure the bounce is complete, otherwise, stick with the ArduDrum_Time to have the ability to trigger
the piezo at more frequent intervals. If you make two hits at a very close range and only trigger the program once, you need to use ArduDrum_Time to pick the close-range hits, etc. Pick the one that best suits your needs.

P.S. If there is an object on piezo discs that applies force constantly to it and squishes with its weight. Like, even a small rubber. The triggering would be more accurate and ArduDrum_Time probably won't have the problem which is triggering multiple times.

## Usage

To use the program, you need to edit a few things in the code as your needs. For this reason, you need to have a general understanding of how the code works. Don't let what I say scare you. I designed the code in a way as flexible and user-friendly as possible so
it's really easy to get into the editing part. First and foremost you have to decide what parts of a drumkit you would have. After this, you need to determine which part of the kit is using 2 piezo sensors to detect hits. Think of this as some part of the kit like
the snare and the ride makes 2 different sounds. So in the program, I've used 2 different piezos dedicated to detecting each different hit and making the sound accordingly. To give you an example, I've dedicated one piezo in the snare to detect the rim shots and the other
one to detect the head. After creating the general layout of your kit it's time to editing part. To explain this with an example, let's say your kit has a snare a ride 2 toms, and a hi-hat.  
