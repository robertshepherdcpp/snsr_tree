## snsr_tree

a tree with all the sensors you need to stay healthy!!

<img width="328" height="475" alt="image" src="https://github.com/user-attachments/assets/9ef073f2-db80-4791-a6e0-8c11aea2455d" />


## inspiration for this project
- i have always wanted some sensors for my room, however, they are expensive and they involve constantly checking the readings.
- so i thought it wold be cool to make my own, where all these sensors are merged into one thing. And if certain levels where too high, it would actually tell me to change them!
- i wanted it to be something that sits in my room, hence i decided to make it the shape of a tree (to blend in with the plants on my desk)!

## how to use
- first get the pcb and the case printed, and then buy all the parts (these are shown in the BOM - total cost: $70.33)
- then paint the case green like shown in the 3d renders.
- you have to download the audio files (mp3) from this repository, and then save them onto an sd card (32)
- then insert the sd card into the sd card holder on the pcb
- then assemble the case (using screws going through the pcb aswell)
- plug into a computer and flash the firmware (in this repository) to the xiao-esp32-c6 (easiest way to do this is via the arduino ide)
- then keep plugged into the computer - as the computer is the project's source of power.
- then use the project! and listen out for warnings!

## what it is / how it works
- the snsr_tree will take readings of different levels (co2, humidity, temperature, light)
- and based on those levels it will tell you to change these levels or it can do it for you! (when it is too dark).
- keeping you alert as to the different levels in your room!

## screenshots
the 3d case:

<img width="604" height="727" alt="image" src="https://github.com/user-attachments/assets/5e178699-db0e-4ed0-b584-fc82eaa0c928" />

the schematic:

<img width="1052" height="667" alt="image" src="https://github.com/user-attachments/assets/5284efcd-0a71-4794-8b01-a1a6073c6a65" />

the pcb wiring:

<img width="762" height="992" alt="image" src="https://github.com/user-attachments/assets/9339fa26-4eca-4ddf-9e34-3add251a383d" />

pcb 3d model:

<img width="589" height="653" alt="image" src="https://github.com/user-attachments/assets/c8002940-372b-4830-a289-6d6207d92df7" />


## Features:
- Wifi Communication for RTC
- Communication via the I2C protocol
