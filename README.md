# Heart Rate Monitor with ECG Signal Processing and BLE

This project implements a low-cost heart rate monitor using an ECG sensor, an ESP32 microcontroller, and BLE (Bluetooth Low Energy) technology. The primary motivation behind this project was to create an affordable and efficient interface to monitor heart rate while virtually walking or running on a treadmill with **Zwift**. The heart rate data is transmitted in real-time over BLE using the Heart Rate Monitor (HRM) service, which can be integrated with apps like Zwift.

## Features

- **ECG Signal Processing**:
  - High-pass filtering to remove baseline wander.
  - Moving average smoothing for noise reduction.
  - Baseline correction for stabilizing the signal.
  - R-peak detection for calculating heart rate (BPM).
  - Outlier rejection using a moving average filter for BPM values.

- **BLE Heart Rate Monitor (HRM)**:
  - BLE service to transmit heart rate data in real-time.
  - BLE Heart Rate Measurement service (UUID: 180D).
  - Compatible with BLE-enabled devices such as smartphones or BLE hubs.

## Motivation

The primary goal of this project was to create a **low-cost interface** to monitor heart rate while virtually walking or running on a treadmill, specifically for use with **Zwift**. Zwift users can benefit from this solution to monitor their heart rate during virtual workouts without the need for expensive commercial heart rate monitors.

## Components

- **ESP32**: Used for signal processing and BLE transmission.
- **AD8232 ECG Sensor**: For real-time ECG monitoring.
- **BLE Capable Device**: Smartphone or BLE hub to receive heart rate data.

## Setup and Requirements

### Hardware Setup

1. **ECG Sensor (AD8232) Connections**:
   - Connect the AD8232 sensor to the ESP32 microcontroller as follows:
     - ECG Signal (Out) -> Pin 36 (ANALOG_PIN)
     - LO+ (Lead Off Positive) -> Pin 2
     - LO- (Lead Off Negative) -> Pin 15

2. **ESP32**:
   - Install the ESP32 board in Arduino IDE. Follow the instructions [here](https://docs.espressif.com/projects/arduino-esp32/en/latest/) to install ESP32 support.

### Software Setup

1. **Arduino IDE**:
   - Install the required libraries:
     - **ESP32 BLE Arduino**: To enable BLE functionality.
     - **Adafruit Sensor Libraries**: If using additional sensors (optional).

2. **Clone the Repository**:
   Clone this GitHub repository to your local machine:
   ```bash
   git clone https://github.com/mayankraichura/esp32-ble-heart-rate-monitor.git
   ```

3. **Open in Arduino IDE**:
   Open the `.ino` file in the Arduino IDE.

### Code Overview

- **ECG Signal Processing**: The code processes the ECG signal in real time, applies a high-pass filter to remove baseline drift, smooths the signal using a moving average filter, and performs baseline correction.
- **BPM Calculation**: R-peaks are detected based on a threshold, and the time interval between consecutive R-peaks (R-R interval) is used to calculate the BPM.
- **BLE Heart Rate Transmission**: The smoothed BPM value is transmitted over BLE using the Heart Rate Measurement service (UUID: 180D).

### Schematic

```
ECG Sensor (AD8232)
   +-----------+       +-----------------+       +--------+
   | ECG Signal|------>|   ESP32 Pin 36  |       |   BLE  |
   |   LO+     |------>|   ESP32 Pin 2   |       | Device |
   |   LO-     |------>|   ESP32 Pin 15  |       |        |
   +-----------+       +-----------------+       +--------+
```

### Running the Code

1. **Connect the hardware** according to the schematic above.
2. **Flash the ESP32**:
   - Compile and upload the code to the ESP32 using the Arduino IDE.
3. **Monitor the Signal**:
   - Open the Arduino Serial Plotter to visualize the ECG signal and the calculated BPM.
4. **Receive Heart Rate Over BLE**:
   - Use a BLE-capable device (such as a smartphone) to scan for the ESP32 and connect to the Heart Rate Monitor service.
   - Use any generic BLE scanner app or app specifically designed to view heart rate data.

### BLE Heart Rate Service

Once the ESP32 is running, it advertises the following BLE services and characteristics:
- **Service**: Heart Rate Service (UUID: `180D`)
- **Characteristic**: Heart Rate Measurement (UUID: `2A37`)

The heart rate data is sent as a BLE notification to any connected BLE client.

## Future Improvements

- Implement advanced filtering techniques (e.g., Kalman filter) to further improve ECG signal stability.
- Add additional health metrics (e.g., heart rate variability).
- Improve battery efficiency for long-term heart rate monitoring applications.

## Credits

This project was made possible with the help of **ChatGPT** and to some extant, **Claude AI**, which provided continuous guidance, troubleshooting, and code generation. Without it, this project would not have been possible.

## Contributing

Feel free to open issues or submit pull requests for bug fixes or enhancements.

## License

This project is licensed under the MIT License.

## Acknowledgements

- **ESP32 BLE Arduino**: For providing the BLE library to enable Bluetooth Low Energy functionality.
- **AD8232 ECG Module**: For real-time ECG monitoring.
- **Zwift**: For providing the motivation behind this project by offering a virtual workout platform.
