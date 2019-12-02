# How to convert sample type

## Convert one channel of audio

If input and output formats are both known at compile time

```c++
const float input[7] = {0.f, 0.25f, -0.25f, 0.5f, -0.5f, 1.f, -1.f};
int16_t output[7];
convert(output, input, 7);
println(make_univector(output, 7));
```

If input format is not known at compile time

```c++
const void* input;
int16_t* output;
size_t size;
audio_sample_type in_type = audio_sample_type::i24; // known at runtime
convert(output, input, in_type, size);
```

If output format is not known at compile time

```c++
const float* input;
void* output;
size_t size;
audio_sample_type out_type = audio_sample_type::i24; // known at runtime
convert(output, out_type, input, size);
```

## Interleaving

```c++
const float* inputs[];
int16_t* output;
size_t channels;
size_t size;
interleave(output, inputs, channels, size);
```

## Deinterleaving

```c++
const float* input;
int16_t* outputs[];
size_t channels;
size_t size;
deinterleave(outputs, input, channels, size);
```

## Convert single audio sample

```c++
i24 s = convert_sample<i24, f32>(1.f);
```

## Supported formats

Constant | Type | Description | Range
-------- | ---- | ----------- | -----
audio_sample_type::i8  | i8  | 8-bit signed  | -127..+127
audio_sample_type::i16 | i16 | 16-bit signed | -32767..+32767
audio_sample_type::i24 | i24 | 24-bit signed | -8388607..+8388607
audio_sample_type::i32 | i32 | 32-bit signed | -2147483647..+2147483647
audio_sample_type::i64 | i64 | 64-bit signed | -9223372036854775807..+9223372036854775807
audio_sample_type::f32 | f32 | 32-bit IEEE   | -1..+1
audio_sample_type::f64 | f64 | 64-bit IEEE   | -1..+1
