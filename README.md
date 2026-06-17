# snsr_tree
a tree with all the sensors you need to stay healthy!
 
<img width="328" height="475" alt="image" src="https://github.com/user-attachments/assets/9ef073f2-db80-4791-a6e0-8c11aea2455d" />

## inspiration
- i have always wanted some sensors for my room, however, they are expensive and involve constantly checking the readings.
- so i thought it would be cool to make my own, where all these sensors are merged into one thing — and if certain levels were too high, it would actually tell me to change them!
- i wanted it to be something that sits in my room, so i decided to make it the shape of a tree (to blend in with the plants on my desk).

## how to use
1. get the PCB and case printed, and buy all the parts (listed in the BOM — total cost: $70.33)
2. paint the case green as shown in the 3D renders
3. download the audio files (mp3) from this repository and save them onto an SD card (32GB)
4. insert the SD card into the SD card holder on the PCB
5. assemble the case using screws, which also go through the PCB
6. plug into a computer and flash the firmware (in this repository) to the XIAO-ESP32-C6 — easiest via the Arduino IDE
7. keep plugged into the computer, as it is the project's power source
8. use the project and listen out for warnings!

## what it is / how it works
- the snsr_tree takes readings of different environmental levels: CO2, humidity, temperature, and light
- based on those readings, it will alert you to change certain levels — or in some cases do it for you (e.g. when it's too dark)
- keeping you informed about the conditions in your room!
  
## features
- WiFi communication for RTC (real-time clock sync)
- I2C protocol for sensor communication

## screenshots
 
the 3D case:
 
<img width="604" height="727" alt="image" src="https://github.com/user-attachments/assets/5e178699-db0e-4ed0-b584-fc82eaa0c928" />

the schematic:
 
<img width="1052" height="667" alt="image" src="https://github.com/user-attachments/assets/5284efcd-0a71-4794-8b01-a1a6073c6a65" />

the PCB wiring:
 
<img width="762" height="992" alt="image" src="https://github.com/user-attachments/assets/9339fa26-4eca-4ddf-9e34-3add251a383d" />

PCB 3D model:

<img width="589" height="653" alt="image" src="https://github.com/user-attachments/assets/c8002940-372b-4830-a289-6d6207d92df7" />
