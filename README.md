# attiny-freqcount
Count frequencies up to 150 MHz with an ATtiny817!

With the Core Independent Peripherals and the Event System of a modern AVR MCU like the ATtiny817 it is possible to do a lot with asynchronous signals, i.e. signals that not synchronized to the MCU clock. 
It is also possible to use many peripherals for other tasks than what the chip-designers had in mind.

This project demonstrate some undocumented features of these chips:

- The XOSC32K-clock, through the TOSC1-pin, can go much, much higher than the 32 kHz that it is specified for. Here it is fed by a 10 MHz precision reference clock.
- The TCD counter, when clocked asynchronous through the EXTCLK-pin, can be counting up to some 150 MHz.

The principle of operation, for the frequency counter implemented, is as follows;

The PIT, clocked at 10 MHz, generates interrupts at a rate of 10MHz/128 = 78.125 kHz. Between theses interrupts (and at all times) the TCD-counter is counting pulses on the EXTCLK-pin. At every PIT-interrupt the value of the TCD counter is captured (through the asynchronous Event System) and the counted value since last interrupt is accumulated. For this to be done fast enough, the AVR core is clocked with the full 20 MHz of the internal RC-oscillator. After 78125 interrupts and accumulations, or exactly one second, an uint32_t holds the frequency in 1 Hz resolution. This is then printed out through the UART.

With a resolution of 12 bits in the TCD timer/counter it is possible to count at most 4096 pulses before two overflows can occur. With its values read and further accumulated at a rate of 78.125 kHz, the theoretical maximum frequency that can be counted is thus 78125*4096 = 320 MHz. In reality, due to the delays in the logic, frequencies up to slightly above 150 MHz have been counted to the exact Hz. This is while the datasheet of the chip mentiones 20 MHz as the highest clock frequency for the TCD.
Note however, that the 150 MHz achieved mandates a supply voltage at slightly more than 5V.
