
# Multi-ECU Automotive CAN Bus Communication Simulation

A software-based automotive Controller Area Network (CAN) simulation developed in C using Linux SocketCAN and a virtual CAN interface (`vcan0`).

This project demonstrates communication between four simulated Electronic Control Units (ECUs), message filtering, sensor-data decoding, fault detection, and automated integration testing.

## System Architecture

```text
       Engine ECU (0x100)
              |
       Brake ECU (0x200)
              |
      Battery ECU (0x300)
              |
       Virtual CAN Bus
            vcan0
              |
        Dashboard ECU
              |
    Monitoring and Warnings
```

All ECUs communicate over the same virtual CAN network.

## Technologies Used

- C programming
- Linux SocketCAN
- Virtual CAN (vcan0)
- Ubuntu on Windows Subsystem for Linux (WSL2)
- GCC compiler
- can-utils
- Bash scripting
- Git and GitHub

## ECU Functions

| ECU | CAN ID | Function |
|-----|--------|----------|
| Engine ECU | 0x100 | Transmits simulated engine RPM |
| Brake ECU | 0x200 | Transmits brake pressed/released status |
| Battery ECU | 0x300 | Transmits simulated battery voltage |
| Dashboard ECU | Receiver | Monitors messages and reports warnings |

## Implemented Features

- Multi-ECU communication over a virtual CAN bus
- CAN message filtering using SocketCAN
- Engine RPM monitoring
- Brake status monitoring
- Battery voltage monitoring
- High engine RPM warning above 4000 RPM
- Low battery voltage warning below 12.0 V
- Engine ECU communication timeout detection
- Communication recovery detection
- Invalid CAN frame rejection

## Project Structure

```text
CAN_Bus_Project/
├── engine_ecu.c
├── brake_ecu.c
├── battery_ecu.c
├── dashboard_ecu.c
├── tests/
│   ├── test_high_rpm.sh
│   ├── test_low_battery.sh
│   ├── test_normal_battery.sh
│   ├── test_brake_status.sh
│   ├── test_invalid_frame.sh
│   └── test_engine_recovery.sh
├── .gitignore
└── README.md
```

## Setup

The project was developed and tested on Ubuntu under WSL2.

Install the required tools:

```bash
sudo apt update
sudo apt install build-essential can-utils iproute2 -y
```

Create the virtual CAN interface:

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up
ip link show vcan0
```

If `vcan0` already exists, skip the `ip link add` command.

The virtual CAN interface may need to be recreated after restarting WSL.

## Compilation

```bash
gcc -Wall -Wextra engine_ecu.c -o engine_ecu
gcc -Wall -Wextra brake_ecu.c -o brake_ecu
gcc -Wall -Wextra battery_ecu.c -o battery_ecu
gcc -Wall -Wextra dashboard_ecu.c -o dashboard_ecu
```

## Running the Simulation

Open four Ubuntu terminals in the project directory.

Terminal 1:

```bash
./dashboard_ecu
```

Terminal 2:

```bash
./engine_ecu
```

Terminal 3:

```bash
./brake_ecu
```

Terminal 4:

```bash
./battery_ecu
```

The Dashboard displays incoming engine, brake, and battery messages.

## Example Output

```text
Dashboard: Engine RPM = 4500
WARNING: High engine RPM!
Dashboard: Brake PRESSED
Dashboard: Battery Voltage = 11.90 V
WARNING: Low battery voltage!
WARNING: Engine ECU communication timeout!
Engine ECU communication restored!
```

These messages are representative of observed simulation results.

## Automated Testing

Six Bash integration test scripts have been implemented.

| Test | Verified Behavior |
|------|-------------------|
| test_high_rpm.sh | High RPM warning |
| test_low_battery.sh | Low battery warning |
| test_normal_battery.sh | Normal battery voltage without false warning |
| test_brake_status.sh | Brake pressed and released |
| test_invalid_frame.sh | Invalid Engine CAN frame rejection |
| test_engine_recovery.sh | Engine timeout detection and recovery |

Run the tests individually:

```bash
./tests/test_high_rpm.sh
./tests/test_low_battery.sh
./tests/test_normal_battery.sh
./tests/test_brake_status.sh
./tests/test_invalid_frame.sh
./tests/test_engine_recovery.sh
```

All six scripts passed during development.

Stop other running ECU processes before executing tests to avoid interference.

## Limitations

- The project uses virtual CAN rather than physical CAN hardware.
- ECU data is simulated rather than collected from real sensors.
- Automated testing currently covers selected scenarios, not every edge case.
- Real-time performance and hardware-level CAN behavior have not been validated.

## Future Improvements

- Additional boundary-value and malformed-message tests
- Automated test-suite runner
- Improved fault handling and diagnostic reporting
- CAN message logging and performance measurements
- Physical CAN hardware integration when available

## Author

Ajithsing Desing

M.Sc. Electronics Embedded
ESIGELEC School of Engineering

## Project Purpose

Developed as a hands-on learning project to strengthen skills in embedded C, automotive communication protocols, Linux-based development, debugging, and automated testing.
