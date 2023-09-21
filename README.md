# About
- This is ESP-NOW receiving project in Arduino/PlatformIO, intented for short messaging in WiFi-Range.

## Related projects
[ESP-NOW OSC Station](https://github.com/yuskegoto/espnow_osc_station)
[ESP-NOW Rust Client](https://github.com/yuskegoto/espnow_rust_client)

## Hardware
I set up this project for ESP32. Button and Serial LED control is intented for [M5Atom](http://docs.m5stack.com/en/core/atom_lite). It is also possible to target the project to ESP32C3.

# Implemented Features
- Send and receive ESP-NOW message from my other project, espnow_osc_station.
- Error message when the ESP-NOW message not reached.

# References
## ESPNOW
ESPNOW Reference: https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/