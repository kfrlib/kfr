# How to mix stereo channels

## L/R to M/S

Input/output data. [See how to pass data to KFR](types.md)
```c++
univector<float> left;
univector<float> right;
univector<float> middle;
univector<float> side;
```
Code
```c++
unpack(middle, side) = mixdown_stereo(left, right, matrix_halfsum_halfdiff());
```

1. `mixdown_stereo` function takes arrays representing L and R channels and returns values multiplied by the given matrix. 
1. `matrix_halfsum_halfdiff` specifies that $(L+R)/2$ and $(L-R)/2$ must be returned
1. `unpack` writes M+S values to the given arrays

## M/S to L/R

Code
```c++
unpack(left, right) = mixdown_stereo(middle, side, matrix_sum_diff());
```
1. `matrix_sum_diff` specifies that $M+S$ and $M-S$ must be returned, effectively reverting back L and R channels


## Downmix to mono

Input/output data.
```c++
univector<float> left;
univector<float> right;
univector<float> mono;
```
Code
```c++
mono = (left + right) * 0.5f;
```
or, depending on what results you want to get
```c++
mono = left + right;
```