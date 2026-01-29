# 3-Phase Motor Controller

This project documents my learning process designing a **3-phase motor controller** with variable frequency output.  
The goal is to build an **open-source controller** and improve it incrementally over time.

**Status:** Learning project / Work in Progress  
This project is **not production-ready** and is intended for educational use only.

## Current Hardware (Early Prototype)

- 1× ESP32
- 1× BLDC Motor
- 6× RFP30N06LE MOSFETs
- 1× 100 µF DC bus capacitor
- Jumper wires
- Bench power supply

Note: Gate drivers, protection circuitry, dead-time enforcement, and current sensing are not yet implemented and will be added in future revisions.

## Overview

The ESP32 will generate a **3-phase SPWM output** to drive a BLDC motor.  
Motor speed will be controlled by adjusting the output frequency based on a throttle input.

## Current Schematic

The schematic currently shows **only the basic 3-phase inverter power stage**.  
It does **not** yet include:
- High-side / low-side gate drivers
- Shoot-through protection or dead-time control (currently handled by MCU)
- Current or voltage sensing

These features will be added as the project progresses.

![alt text][logo]

[logo]: https://github.com/adam-p/markdown-here/raw/master/src/common/images/icon48.png](https://github.com/adriaanjj/3-Phase-Motor-Controller/blob/main/assets/schematic.png
