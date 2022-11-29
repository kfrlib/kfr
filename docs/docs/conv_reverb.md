## How to apply Convolution Reverb

### Mono version

Input/Output data: [See how to pass data to KFR](basics.md)
```c++
univector<float> audio;
univector<float> impulse_response;
```
Code
```c++
convolve_filter<float> reverb(impulse_response);
reverb.apply(audio);
```

!!! note
    `convolve_filter` uses [Filter API](auto/filter.md) and preserves its internal state between calls to `apply`.
    Audio can be processed in chunks.
    Use `reset` function to reset its internal state.

    `convolve_filter` has zero latency.

    Internally, [FFT](dft.md) is used for performing convolution

### True stereo version

Formula

$$
\begin{aligned}
    out_{L} &= in_{L} * ir_{LL} + in_{R} * ir_{RL}\\
    out_{R} &= in_{L} * ir_{LR} + in_{R} * ir_{RR}
\end{aligned}
$$

where $*$ is convolution operator.

Input/Output data:
```c++
univector<float, 2> stereo_audio;
univector<float, 4> impulse_response;
// impulse_response[0] is left to left
// impulse_response[1] is right to left
// impulse_response[2] is left to right
// impulse_response[3] is right to right
```
Code
```c++
// Prepare filters
convolve_filter<float> reverb_LL(impulse_response[0]);
convolve_filter<float> reverb_LR(impulse_response[1]);
convolve_filter<float> reverb_RL(impulse_response[2]);
convolve_filter<float> reverb_RR(impulse_response[3]);

// Allocate temp data
univector<float> tmp1(stereo_audio[0].size());
univector<float> tmp2(stereo_audio[0].size());
univector<float> tmp3(stereo_audio[0].size());
univector<float> tmp4(stereo_audio[0].size());

// Apply convolution
reverb_LL.apply(tmp1, audio[0]);
reverb_RL.apply(tmp2, audio[1]);
reverb_LR.apply(tmp3, audio[0]);
reverb_RR.apply(tmp4, audio[1]);

// final downmix
audio[0] = tmp1 + tmp2;
audio[1] = tmp3 + tmp4;
```
