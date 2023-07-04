# WakeOnWireless

## Overview

WoW - WakeOnWireless - Wake a remove device using wireless.

IoT device to remotly power up a device. This is particular useful for wireless connected computers, whose wireless card don't support the **wol** magic packet. 
In this implementation an ESP acts as a MQTT client, consuming **wake** events from a given topic, in this case 'wow/push', and shorts the power switch circuit for a brief amount of time, emulating the button switch, switching the device ON.

In the figure bellow:
- **J1** represent the input pins where the power switch cables from the computer case must be connected to.
- **J2** represent the output pins to connect with the motherboard power switch.

<img width="451" alt="wol" src="https://user-images.githubusercontent.com/9936714/73220573-a9346e80-4156-11ea-9cc3-16674fdf1ef2.png">


  Written by [@snackk](https://github.com/snackk)
<img width="1120" alt="Screenshot 2023-07-02 at 15 06 13" src="https://github.com/snackk/WakeOnWireless/assets/9936714/241ba884-4c6b-4ccb-81da-3e6c4793b971">
