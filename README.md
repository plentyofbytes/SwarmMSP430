# SwarmMSP430
This project provides some source code to interface with Swarm M138 Satellite Modem via UART.

This code has been proven on MSP430FR5994 16-bit microcontroller, but you can easily port it over to whatever controller you're using.
This code has been proven to work using UART, Timer, and Message code from my other repositories. If you want, you can add that code to this project to see for yourself.
Code is kept organized this way, so changes don't need to be multiplied across all repositories.

Check out my other repositories for a more complete picture. If you just want the Swarm stuff, this will work.
If you have no UART, SPI, Timer, or other code, my other repositories will definitely help you out.

All of my repositories are for MSP430:

-> Timer Pool Paradigm - My Timer Paradigm uses a pool of Timers to pick from as needed dynamically. Request, Start, Stop, and Kill Timers as needed. Timers use callbacks to fire Software Interrupts (SWIs) as you would see in 32 bit MCUs with RTOS's. Finding this level of sophistication on a 16-bit MCU is a treat.

-> Message System Paradigm - My Serial Transfer Paradigm allows for smooth and reliable Interrupts using Message Structs. The Message Structs provide an easy-to-use, coherant way of sending and receiving data. Message Structs are Requested, Used, and Freed/Killed as needed. Memory for Message Structs are allocated in RAM using Message.h header file.

Please leave feedback and submit issue tickets. 

-- MicroTechEE

