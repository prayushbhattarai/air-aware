# AirAware

AirAware is a portable particulate matter monitor designed to make air quality visible and understandable. The device measures airborne particles using a Plantower PMS5003 sensor and displays the results through a simple interface powered by an ESP32-C3.

The project combines PCB design, embedded programming, environmental sensing, and classroom education.

Project started: July 2026

## Overview

AirAware was built around a simple question:

What does the air around us actually contain?

Most people cannot see changes in particulate matter concentration as they happen. AirAware provides a portable way to measure those changes and display them in a format that is easy to understand.

The current device measures particulate matter and displays:

* Particle count
* PM2.5 concentration
* A visual air-quality indicator

The interface uses a simple facial indicator to help younger students interpret changes in particle concentration without needing to understand every measurement immediately.

## Hardware

The current AirAware device uses:

* ESP32-C3 SuperMini
* Plantower PMS5003 particulate matter sensor
* 2.0-inch 320 × 240 ST7789 display
* 3.7 V lithium-polymer battery
* Pololu S13V15F5 5 V boost regulator
* Custom 2-layer PCB

## How It Works

The PMS5003 pulls air through its sensing chamber and measures suspended particulate matter.

The sensor sends measurement data to the ESP32-C3 through UART.

The ESP32-C3 processes the sensor readings and updates the ST7789 display with the current particle measurements and visual indicator.

Basic data flow:

PMS5003
↓
UART communication
↓
ESP32-C3
↓
Data processing
↓
ST7789 display

## Display

The interface was designed to stay simple enough for classroom use.

The main screen emphasizes particle count while also showing PM2.5 concentration.

A facial indicator changes based on the measured particle level:

🙂 Lower particle concentration

😐 Moderate particle concentration

☹ Higher particle concentration

The device also includes a startup screen while the PMS5003 completes its initial warm-up period.

## Educational Use

AirAware was designed as both an engineering project and an educational tool.

Students use the devices in small groups to:

* Observe real-time particulate matter measurements
* Compare measurements from different environments
* Test how activities affect airborne particles
* Record experimental data
* Calculate averages
* Compare results between groups
* Discuss possible sources of particulate matter

The goal is to connect environmental science with measurements students collect themselves.

### firmware

Contains the code running on the ESP32-C3.

### hardware/kicad

Contains the editable KiCad project files:

* Schematic
* PCB layout
* KiCad project

### hardware/production

Contains files used to manufacture the PCB, including Gerber and drill files.

