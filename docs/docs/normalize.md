# How to normalize audio

## One channel

Input/Output data: [See how to pass data to KFR](types.md)
```c++
univector<float> audio;
```

Code
```c++
audio /= absmaxof(audio);
```

1. `absmaxof` function calculates the absolute maximum of the given array (`max(abs(x[i])...)`)
1. `operator/=` performs inplace division for array
    
Equivalent pure C++ code
```c++
float absmax = 0;
for (size_t i = 0; i < audio.size(); ++i)
{
    absmax = std::max(absmax, std::abs(audio[i]));
}
for (size_t i = 0; i < audio.size(); ++i)
{
    audio[i] = audio[i] / absmax;
}
```

## Stereo audio

Input/Output data:
```c++
univector<univector<float>, 2> stereo;
```

Code
```c++
const float peak = absmaxof(concatenate(stereo[0], stereo[1]));
unpack(stereo[0], stereo[1]) = pack(stereo[0], stereo[1]) / pack(peak, peak);
```

1. `concatenate` function concatenates two arrays as if they were written sequentially in memory (but without any actual memory copies)
1. `absmaxof` function calculates the absolute maximum of the concatenated arrays
1. `pack` function combines two arrays as if it were a single array containing pairs of values
1. `operator/` divides pairs by the peak value
1. `unpack` function unpacks the pairs to two target arrays