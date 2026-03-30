# Sheath Circulating Current Monitoring System (Python Companion)

This is a Web console frontend and companion backend demonstration program for a high-voltage cable Sheath Circulating Current online monitoring system, built with [Plotly Dash](https://dash.plotly.com/). This program is designed for data visualization, alarm management, and device ledger management.

## 🌟 Core Features

* **Real-time Monitoring (Device Monitor)**
  * Dynamically displays real-time current waveforms of cable channels (including peak, valley, and RMS values).
  * Harmonic analysis bar charts (1st, 2nd, 3rd, 5th, 7th harmonics) and Total Harmonic Distortion (THD).
  * When data exceeds limits, related values and waveform markers will trigger a red flashing alarm animation.
* **Alarm Management (Alarm Management)**
  * **Alarm Threshold Configuration**: Supports independent upper and lower limit threshold settings for RMS, peak/valley current, phase offset, zero-cross offset, and various harmonics.
  * **Real-time Alarm List**: Records all over-limit events and provides detailed information on time, device, specific alarm items, and over-limit values.
  * **AI Analysis Assistant**: For a single alarm record, users can expand an "AI Diagnostic Report" with one click, providing troubleshooting suggestions such as checking grounding loops and sensor wiring.
* **Device Management (Device Management)**
  * Provides complete device ledger CRUD (Create, Read, Update, Delete) functionalities.
  * Supports registering basic cable information (model, length, laying method, tunnel name, phase, etc.).
  * Supports configuring communication parameters (IP/485 mode, Modbus slave ID, baud rate, TCP port, subnet mask, gateway, etc.).
  * Supports toggling channel enable status and configuring CT ratio/offset.

## ⚠️ Current Version Notes & Limitations

1. **Simulated Data & Alarm Mechanism**: Currently, the system is not directly connected to real hardware acquisition devices. Therefore, all Modbus monitoring data displayed on the interface **are built-in simulated data**. All monitored data items can be configured with upper and lower thresholds. Once the current data exceeds the set range, the corresponding data will not only flash a red marker as a warning but also automatically generate and save a corresponding alarm message to the alarm list.
2. **Data Persistence & Historical Records**: Because this companion demo program **has not yet been connected to a database**, all saved information (such as newly created device ledgers, modified alarm thresholds, etc.) **cannot be saved permanently**. Once the webpage is refreshed or reset, the data will revert to the default initial state. In particular, the alarm information is only valid for the current real-time simulated values and cannot be saved long-term. Currently, the historical data query function has not been developed and will be gradually implemented in future updates.

## 📂 Project Structure

This Web application mainly consists of the following four core Python files:

* `circulating_current.py`: The entry point file of the program, responsible for initializing the Dash application instance, injecting global CSS animation styles, and starting the local server.
* `current_views.py`: The view layer, containing the frontend UI layout code of the application, utilizing Plotly chart components and Dash HTML components to build monitoring panels, tables, and forms.
* `current_callbacks.py`: The logic control layer, registering all interactive callback functions (such as page routing switches, form data saving, alarm threshold updates, chart data refreshes, etc.).
* `current_data.py`: The data and model layer, defining default alarm thresholds, device menu structures, and housing the logic for generating advanced simulated waveform data (`get_advanced_mock_data`) and alarm events.

## ⚙️ Dependencies & Installation

Running this project requires a Python 3 environment. Please ensure the following dependencies are installed:

bash
pip install dash plotly numpy


## Quick Start Guide
Open Terminal: Launch your command line interface and navigate to the project directory:

Bash
cd 2_Python_Companion
Run the Application: Execute the main Python script:

Bash
python circulating_current.py
Access the Dashboard: Once the terminal indicates the server is running, open your web browser and go to:
http://127.0.0.1:8050

Development Mode: The application currently runs with debug=True enabled. Any code changes saved will trigger an automatic browser refresh (Hot Reload).

## 💡 System Operations Manual
Once the dashboard is loaded, you can explore the core features following these steps:

*  1. Real-time Monitoring
Navigation: The system defaults to the Device Monitor interface.

Switching Views: Select different cables (e.g., HV Cable 1, HV Cable 2) from the left sidebar to toggle real-time waveforms and harmonic bar charts.

Alert Indicators: If a metric (such as RMS or specific harmonic orders) exceeds the pre-set threshold, the value background will trigger a red flashing animation to signal an active alarm.

* 2. Alarm Management & AI Analysis
Navigation: Click Alarm Management in the top navigation bar.

Alarm History: Select Alarm List from the sidebar to view records automatically generated by threshold violations.

AI Diagnostics: Click the AI Analysis button next to any record to expand a detailed troubleshooting recommendation provided by the AI engine.

Threshold Configuration: Select Alarm Config to adjust upper and lower limits for monitoring parameters. Changes take effect immediately across the system upon saving.

* 3. Asset & Device Inventory
Navigation: Click Device Management in the top navigation bar.

Registration: Use the Add Device section to register new cables, locations, and Modbus communication parameters. Click Save Device to commit to the database.

Inventory Overview: Select Device List to view all registered assets, their current online status, and enabled channel configurations.