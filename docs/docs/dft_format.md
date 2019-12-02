# DFT data layout

## CCS format:

index | real | imaginary
----- | ---- | ---------
0 | frequency[0].real() (DC offset) | **always 0 because of real input**
1 | frequency[1].real() | frequency[1].imag()
… | … | …
N/2-1 | frequency[N/2-1].real() | frequency[N/2-1].imag()
N/2 | **frequency[N/2].real() (Nyquist frequency)** | **always 0 because of real input**

## Perm format:

index | real | imaginary
----- | ---- | ---------
0 | frequency[0].real() (DC offset) | **frequency[N/2].real() (Nyquist frequency)**
1 | frequency[1].real() | frequency[1].imag()
… | … | …
N/2-1 | frequency[N/2-1].real() | frequency[N/2-1].imag()