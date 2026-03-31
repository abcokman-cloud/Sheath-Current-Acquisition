### 🚀 Project Overview
I am thrilled to announce that the core development phase of the **Sheath Circulating Current Monitoring System** is now complete! This project successfully bridges the gap between industrial-grade hardware sensing and modern Web data visualization.

The system is now fully operational, covering the entire pipeline: from real-time signal acquisition on **STM32** and edge algorithm processing to intelligent data display on a **Python Dash** host dashboard.

---

### 🏗️ System Architecture
The project is organized into two core repositories/modules:

#### 1. Core Firmware (`1_Firmware` - STM32 Based)
* **Hardware-Level Acquisition**: High-speed sampling of sheath current signals using the multi-channel high-rate ADC of the STM32.
* **Edge Computing Algorithms**: Real-time calculation of **RMS (Root Mean Square)**, **Peak-to-Valley** values, and **Phase Offsets** on the MCU to significantly reduce server-side overhead.
* **Industrial Communication Protocols**: Implementation of standard **Modbus RTU/TCP** protocols to ensure seamless integration with gateways or host computers.
* **Sampling Optimization**: Deeply optimized sampling strategies to ensure the accuracy of zero-sequence logic determination.

#### 2. Host Companion (`2_Python_Companion` - Dash Based)
* **Real-time Monitoring Dashboard**: Built with **Plotly Dash**, featuring dynamic rendering of current sine waves and harmonic distribution charts (1st, 3rd, 5th, and 7th harmonics).
* **Intelligent Alarm Engine**: A configurable threshold system supporting "Red-Flash" visual alerts when data exceeds safety limits.
* **AI Diagnostic Assistant**: An integrated analysis module that generates diagnostic reports with one click, offering suggestions for grounding loop checks and sensor wiring. *(Note: The dedicated web-based AI analysis version is under development).*
* **Asset Management**: A comprehensive CRUD interface for managing cable assets and Modbus communication parameters.

---

### 📺 Key Feature Highlights

| Functional Module | Technical Description | Status |
| :--- | :--- | :--- |
| **Waveform Visualization** | High-fidelity rendering of current waveforms and Total Harmonic Distortion (THD). | ✅ Completed |
| **Harmonic Analysis** | Precise extraction of specific harmonic orders for predictive insulation health monitoring. | ✅ Completed |
| **AI Assisted Diagnostics**| Logic-algorithm-based diagnostic reports to facilitate rapid field maintenance. | ✅ Completed |
| **Modbus Ledger** | Flexible management of Slave IDs, IP mapping, CT ratios, and offsets. | ✅ Completed |

---

### 🛠️ How to Run the Demo
To experience the frontend UI and data engine:

1. **Navigate to directory**: 
   ```bash
   cd 2_Python_Companion
Install dependencies:

Bash
pip install dash plotly numpy
Run the application:

Bash
python circulating_current.py
Access via browser: http://127.0.0.1:8050/

Additionally, you can visit the external demo page at http://217.142.227.124:8050/.

📝 Development Reflections & Roadmap
This project represents a major milestone in merging my Electrical Engineering background with a Modern Software Stack.

Next Steps: Implementation of a persistent database (e.g., PostgreSQL/TimescaleDB) for long-term historical trend analysis, and the rollout of dedicated AI analysis features.

Future Vision: Exploring the deployment of TensorFlow Lite for Microcontrollers (TFLM) to the STM32 side for edge-side AI anomaly detection.

I welcome any feedback or suggestions regarding the architecture design or the UI/UX experience!

#STM32 #Python #Dash #SmartGrid #IoT #ElectricalEngineering
