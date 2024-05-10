# Keypad and Seven-Segment Decoder Implementation 

## Overview

This VHDL implementation reads the keys pressed from the PMOD keypad module and displays the corresponding value on both the seven-segment displays (SSDs). The code is designed to provide visual feedback by using a delay between switching the left and right segments, allowing the user to distinguish between the displays.

## Functionality

- **Keypad Input:** The code reads key presses from the PMOD keypad module as input operands.
- **Seven-Segment Display:** The corresponding values are displayed on both SSDs.
- **Delay Adjustment:** Initially, a delay of 250 ms is used between switching the left and right segments. This delay can be adjusted to change the appearance of the display.
  - For example, decreasing the delay to 10 ms may make it appear as if both segments are lit simultaneously.
- **Arithmetic Operations:** The code performs arithmetic operations based on inputs from the keypad and displays the result on the SSDs.
  - Supported operations include addition (+), subtraction (-), multiplication (*), division (/) and factorial , which are selected using push buttons on the board.
  - Inputs are operands from the keypad, and the output is displayed on the SSDs.

## Inputs and Outputs

- **Inputs:** Operands from the keypad.
- **Outputs:** Displayed arithmetic operation results on the seven-segment displays.

## Usage

This VHDL code provides a practical implementation of interfacing a keypad and SSDs in a digital design project. It can be used to create a user-friendly interface for inputting operands and displaying results of arithmetic operations.

## License

This VHDL code is provided under an open-source license, allowing for unrestricted use, modification, and distribution.

