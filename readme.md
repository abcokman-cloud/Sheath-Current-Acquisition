# Sheath Circulating Current Acquisition Unit

## 1. Project Introduction
This project provides the firmware for a high-voltage cable sheath circulating current acquisition unit. Its main features include high-frequency data sampling, real-time RMS (Root Mean Square) calculation, digital filtering, and reliable data transmission via an Ethernet module.
This project provides robust firmware for a high-voltage cable sheath circulating current acquisition unit, designed to monitor cable insulation status and prevent potential faults in real-time. 

**Key Features & Capabilities:**
* **High-Speed Multi-Channel Sampling:** Utilizes STM32 ADC and DMA for continuous, non-blocking high-frequency data acquisition across 4 independent circulating current channels.
* **Advanced DSP & Power Quality Analysis:** Leverages the ARM Cortex-M4 FPU and a custom DSP library for high-precision True RMS calculation, digital filtering, harmonic analysis (THD, up to 7th order), vector sum, and peak/valley detection.
* **Hardware Temperature Interface:** While a TCXO/OCXO is recommended to eliminate temperature drift, a built-in hardware temperature detection interface (via ADC14) is implemented to support future software-based drift compensation if needed.
* **Robust Ethernet Connectivity:** Integrates the W5500 hardware TCP/IP stack with a resilient state machine, supporting Modbus TCP protocol and high-throughput transparent waveform data transmission.
* **Flexible Modbus Configuration:** Features a comprehensive register map for dynamic adjustment of CT ratios, hardware offsets, network parameters, and system commands, backed by persistent Flash storage.

## 2. Hardware Environment
* **MCU**: STM32F407VG (ARM Cortex-M4 with FPU)
* **Key Peripherals**: Sheath circulating current transformer (Ratio 1000A:1A or 200A:1A), W5500 Ethernet module, analog signal conditioning, power management module, etc.
* **Debugging Tool**: CMSIS-DAP / J-Link / ST-Link

## 3. Software Development Environment
* **IDE**: Keil MDK-ARM v5.25 or higher
* **Underlying Library**: STM32 Standard Peripheral Library (SPL) / HAL

## 4. Project Directory Structure
```text
┣ 📂 USER                      (Application & Business Logic)
┃ ┣ 📄 main.c                  (Main entry and core task scheduling)
┃ ┣ 📄 stm32f4xx_it.c          (Interrupt service routines)
┃ ┣ 📄 sys_config.c            (System config, Modbus register map & Flash operations)
┃ ┣ 📄 current_monitoring.c    (ADC continuous sampling & DMA transfer scheduling)
┃ ┣ 📄 ct_ratio.c              (CT ratio calculation & physical value conversion)
┃ ┗ 📄 protocol_dispatcher.c   (Network payload routing & Modbus parsing)
┣ 📂 HARDWARE                  (External Hardware Drivers)
┃ ┣ 📄 w5500_driver.c          (W5500 SPI driver & robust TCP server state machine)
┃ ┣ 📄 sensor.c                (Hardware temperature monitoring interface)
┃ ┗ 📄 led.c                   (System status LED indicator)
┣ 📂 SYSTEM                    (System-Level Components)
┃ ┣ 📄 delay.c                 (Hardware delay utilities)
┃ ┣ 📄 sys_tick.c              (System tick timer management)
┃ ┗ 📄 usart.c                 (Debug UART and printf retargeting)
┣ 📂 CORE                      (ARM Cortex-M Core Files)
┃ ┣ 📄 startup_stm32f40_41xxx.s(Startup assembly code)
┃ ┗ 📄 system_stm32f4xx.c      (System clock configuration)
┣ 📂 Libraries                 (Compiled Static Libraries)
┃ ┣ 📄 Sheath_DSP.lib          (Core DSP library for high-precision RMS & digital filtering)
┃ ┗ 📄 Wiznet_Network.lib      (W5500 hardware TCP/IP stack & robust networking library)
┗ 📂 FWLIB                     (STM32 Standard Peripheral Library)
  ┣ 📄 misc.c                  (NVIC interrupt controller driver)
  ┣ 📄 stm32f4xx_adc.c         (ADC driver for continuous sampling)
  ┣ 📄 stm32f4xx_dma.c         (DMA driver for memory transfer)
  ┣ 📄 stm32f4xx_gpio.c        (GPIO pin configuration driver)
  ┣ 📄 stm32f4xx_rcc.c         (System clock and reset driver)
  ┣ 📄 stm32f4xx_spi.c         (SPI communication driver for W5500)
  ┗ 📄 stm32f4xx_usart.c       (UART communication driver for debugging)
```

## 5. Hardware Pin Allocation
| Peripheral | STM32 Pin | Function | Remarks |
| :--- | :--- | :--- | :--- |
| **Status LED** | `PA0` | GPIO_OUT | System running status indicator |
| **Debug UART** | `PA9` / `PA10` | USART1 TX / RX | Used for `printf` debugging logs (115200 8N1) |
| **W5500 SPI** | `PB12` - `PB15` | SPI2 / GPIO | W5500 network communication interface (Isolated from PA) |
| **ADC Sampling** | `PA4` - `PA7` | ADC1_IN4 - IN7 | 4-channel analog inputs for continuous circulating current sampling |

## 6. Compilation & Download
1. Navigate to the `USER/` directory and double-click `*.uvprojx` to open the project in Keil.
2. Click the **Build** button on the toolbar (or press `F7`) to compile. Ensure there are 0 errors.
3. Connect your debugger and click the **Download** button (or press `F8`) to flash the firmware into the MCU.

## 7. Version History
* **v1.0** (2026-03-24): Initial release. Completed the underlying driver framework, memory mappings, and basic testing.

## 8. Contact & Customization Services
If you are interested in this project or require customized development for other hardware platforms or specific business functions, feel free to contact me:
* **Email**: abcokman@gmail.com