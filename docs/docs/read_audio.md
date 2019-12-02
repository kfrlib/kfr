
# How to read WAV file

## Read whole audio file into array of `univector`

```c++
// Open file as sequence of float`s, conversion is performed internally
audio_reader_wav<float> reader(open_file_for_reading("audio.wav"));
univector2d<float> audio = reader.read_channels();
```

## Get file information

```c++
audio_reader_wav<float> reader(open_file_for_reading("audio.wav"));
println("Sample Rate  = ", reader.format().samplerate);
println("Channels     = ", reader.format().channels);
println("Length       = ", reader.format().length);
println("Duration (s) = ", reader.format().length / reader.format().samplerate);
println("Bit depth    = ", audio_sample_bit_depth(reader.format().type));
```

## Read all data as separate channels:

```c++
univector2d<float> audio1 = reader.read_channels();
```

## Read specific number of samples:

```c++
univector2d<float> audio2 = reader.read_channels(reader.format().length / 2);
```

## Read interleaved audio
```c++
univector<float> audio3 = reader.read(1024);
```

## Read interleaved audio to preallocated buffer
```c++
std::vector<float> audio4;
reader.read(audio4.data(), audio4.size());
```

## How to read FLAC file

Just replace class name by `audio_reader_flac`.

```c++
audio_reader_flac<float> reader(open_file_for_reading("audio.flac"));
```