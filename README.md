# WakeOnWireless

# ESP IoT Device with WiFi and MQTT

## 🔧 Hardware Features

- **ESP2866 microcontroller** - Main processing unit with WiFi capabilities
- **CP2102 USB-to-Serial bridge** - For programming and debugging
- **AMS1117-3.3V voltage regulator** - Stable power supply
- **Reset and Boot switches** - Manual control options
- **Status LEDs** - Visual feedback indicators
- **Voltage level translation** - Multi-voltage I/O support

### Schematic
<img width="1080" height="638" alt="Screenshot 2025-08-16 at 21 31 58" src="https://github.com/user-attachments/assets/f5fdf168-3a61-4828-9b06-8eeec86aec22" />

## 🚀 Getting Started

### Prerequisites

- PlatformIO on Visual Studio
- ESP2866 board package
- Required libraries:
  - WiFi
  - WebServer
  - MQTT (PubSubClient)
  - SPIFFS (for web files)

### Hardware Setup

1. Connect your ESP2866 board via USB
2. Ensure all components are properly connected according to the schematic
3. Verify power supply connections (3.3V and 5V rails)

### Software Installation

1. Clone this repository:
git clone [repository-url]
cd esp32-iot-device

2. Open the project in Arduino IDE or PlatformIO

3. Install required libraries through Library Manager

4. Upload the filesystem (SPIFFS) containing web files:
- For Arduino IDE: Tools -> ESP32 Sketch Data Upload
- For PlatformIO: pio run --target uploadfs

5. Compile and upload the main firmware

## 🌐 Web Interface

The device provides several web pages:

### Setup Pages
- **WiFi Setup** (`wifi_setup.html`) - Configure network credentials
- **MQTT Setup** (`mqtt_setup.html`) - Configure MQTT broker settings
- **Success Pages** - Confirmation of successful configuration

### Main Interface
- **Dashboard** (`index.html`) - Main control and monitoring interface
- Real-time status updates
- Device control buttons
- Configuration management

## 📁 Project Structure

```
esp32-iot-device/
├── data/ # SPIFFS filesystem files
│ ├── index.html # Main dashboard
│ ├── wifi_setup.html # WiFi configuration page
│ ├── mqtt_setup.html # MQTT configuration page
│ ├── wifi_setup_success.html
│ ├── style.css # Stylesheet for all pages
│ └── favicon.jpg # Website icon
├── src/ # Source code (main firmware)
├── hardware/ # Schematic and PCB files
├── docs/ # Documentation
└── README.md # This file
```

## ⚙️ Configuration

### WiFi Setup
1. Power on the device
2. Connect to the ESP2866 access point
3. Navigate to the setup page
4. Enter your WiFi credentials
5. Device will restart and connect to your network

### MQTT Configuration
- **Broker Address**: Your MQTT broker IP/hostname
- **Port**: Usually 1883 (non-SSL) or 8883 (SSL)
- **Username/Password**: If required by your broker
- **Topic**: Custom topic for device communication

## 🎨 Web Interface Styling

The interface features:
- **Responsive grid layout** - Adapts to different screen sizes
- **Modern card-based design** - Clean, organized information display
- **Consistent color scheme** - Professional blue and white theme
- **Interactive buttons** - Hover effects and state indicators
- **Mobile-friendly** - Touch-optimized controls

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

  Written by [@snackk](https://github.com/snackk)
