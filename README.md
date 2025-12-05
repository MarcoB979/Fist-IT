# Fist-IT

Fist-IT is a firmware and hardware project for advanced stepper motor control, designed for integration with M5 remote and gyro-based controllers. It features:

- ESP32-based stepper control
- M5 remote wireless communication
- Gyro safety and angle control
- Highly configurable speed and sensation (acceleration) mapping
- PlatformIO project structure

## Project Structure

- `src/` - Main source code
- `include/` - Header files
- `lib/` - External libraries
- `Backups/` - Backup and legacy code
- `test/` - Test code

## Getting Started

1. Clone this repository
2. Open with [PlatformIO](https://platformio.org/) in VS Code
3. Connect your ESP32 device
4. Build and upload the firmware

## PlatformIO Commands

- Build: `PlatformIO: Build`
- Upload: `PlatformIO: Upload`
- Monitor: `PlatformIO: Monitor`

## License

MIT License

---

For more details, see the code and comments in `src/main.cpp`.
