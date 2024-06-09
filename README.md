# TeensySpresso :coffee:
Teensy 3.2 PID controller, PCB, and enclosure design for a single-boiler Rancilio Silvia espresso machine.

## :robot:
All of the existing Open Source solutions for an integrated PID controller were too big to fit in my single-boiler Rancilio Silvia v2. I put this together and glued it on above the steam wand's valve. I didn't want to change the look or feel of the machine at all, so I have not included a physical display in the design. Instead, a web-UI will be hosted from the ATWINC1500.
The heating element's control circuit bypasses the OEM brew thermistor which leaves the steam and safety circuits intact.

## TODO
- [ ] Install and test :sweat_smile:
- [x] LED pad spacing
- [x] Relay footprint
- [x] Update schematic to connect WINC_EN to a pin... oops
- [x] Enable WINC1500 WiFi module :signal_strength:
  - [x] Host WiFi from module, enable incoming connections
  - [x] Update firmware to 19.6.1 by creating new BSP for teensy or using supported board. My firmware version is 19.4.4
- [x] Assemble print of v2
- [ ] MQTT
  - [x] deploy MQTT broker to cloud
  - [x] MQTT client in Teensy connects to MQTT broker in cloud 
  - [ ] access by domain rather than IP
- [ ] Host Web-UI for monitoring and control :chart:
  - [ ] 
  - [ ] Host Next.js app on free service (Vercel?)
  - [ ] Monitor w/ charts
  - [ ] Configurable temperature setpoint
  - [ ] Configurable mode (brew temp, steam temp)
- [ ] Docs :scroll:
  - [x] Circuit diagram
  - [ ] Pics
  - [x] BOM
- [ ] Results

##
Enjoy the brew. Cheers to productivity!
:coffee:
