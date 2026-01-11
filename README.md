The **Minimix** is a compact digital mixing console based on the **LPC804** microcontroller.

## Objectives

* **Audio**: Signal processing via ADC/DAC converters.

* **Volume**: Real-time output gain control using linear faders.

* **Equalization**: 3-band frequency adjustment (High, Mid, Low) via rotary potentiometers.

## Technical Constraints

* **Real-time**: 40 kHz sampling rate for two channels.

* **Execution**: Limited to 375 cycles per sample at 15 MHz, making floating-point operations impossible.

* **Optimization**: Heavy calculations are moved outside the critical loop or simplified using bit shifts instead of multiplications.

## Signal Processing

1. **Filtering**: Frequency extraction via IIR integrators using bit shifts.

2. **Mixing**: Weighted sum of bands per channel.

3. **Output**: Result is clamped and sent to the DAC register.

## Limitations

* High noise levels due to low ADC/DAC resolution and non-soldered connections.


* Future improvements: Higher frequency microcontroller, dedicated audio converters, and a custom PCB.

---

**Author**: Samuel Ogulluk (ENS Paris-Saclay).
