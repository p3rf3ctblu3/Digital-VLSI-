# Digital-VLSI

This repository contains VHDL and C programs created for the 4th-year Digital VLSI class (Spring 2026) at the National Technical University of Athens. 

These designs are simulated in Vivado 2022.2 and include Vitis scripts for deployment on a Zybo Z7 Xilinx FPGA board.

## Repository Contents

### 1. Debayering Filters
These designs convert raw camera sensor data (Bayer patterns) into standard RGB color:
* **Static Debayering Filter:** A fixed-pattern pipeline design.
* **Reconfigurable Debayering Filter:** An adaptable design that can handle different Bayer nxn grid formats (256x256 up to 1024x1024) at runtime.

### 2. FIR Filter
* **Direct Form FIR Filter:** A standard Finite Impulse Response filter implementation with pipeline registers to optimize speed.

### 3. Systolic Architectures
Parallel, hardware-friendly structures for math operations:
* **Systolic Multiplier:** A 2D array structure for fast multiplication.
* **Systolic Adder:** A structured, pipelined adder array to reduce delay.

### 4. Hardware Architecture Foundations
Basic hardware designs illustrating the differences between:
* **Behavioral Modeling:** High-level algorithmic descriptions.
* **Structural Modeling:** Low-level logic gate and component wiring.
* **RTL Architecture:** Synthesizable register-to-register designs.
