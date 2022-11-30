# Window functions code & examples

## Table of Contents

* [Hann window](#hann-window)
* [Hamming window](#hamming-window)
* [Blackman window](#blackman-window)
* [Blackman-Harris window](#blackman-harris-window)
* [Gaussian window](#gaussian-window)
* [Triangular window](#triangular-window)
* [Bartlett window](#bartlett-window)
* [Cosine window](#cosine-window)
* [Cosine window (numpy compatible)](#cosine-window-numpy-compatible)
* [Bartlett-Hann window](#bartlett-hann-window)
* [Bohman window](#bohman-window)
* [Lanczos window](#lanczos-window)
* [Flat top window](#flat-top-window)
* [Kaiser window](#kaiser-window)

## Hann window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_hann(output.size());
plot_save("window_hann", output, options + ", title='Hann window'");
```
Result
![window_hann](img/window_hann.svg)

## Hamming window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_hamming(output.size());
plot_save("window_hamming", output, options + ", title='Hamming window'");
```
Result
![window_hamming](img/window_hamming.svg)

## Blackman window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_blackman(output.size());
plot_save("window_blackman", output, options + ", title='Blackman window'");
```
Result
![window_blackman](img/window_blackman.svg)

## Blackman-Harris window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_blackman_harris(output.size());
plot_save("window_blackman_harris", output, options + ", title='Blackman-Harris window'");
```
Result
![window_blackman_harris](img/window_blackman_harris.svg)

## Gaussian window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_gaussian(output.size());
plot_save("window_gaussian", output, options + ", title='Gaussian window'");
```
Result
![window_gaussian](img/window_gaussian.svg)

## Triangular window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_triangular(output.size());
plot_save("window_triangular", output, options + ", title='Triangular window'");
```
Result
![window_triangular](img/window_triangular.svg)

## Bartlett window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_bartlett(output.size());
plot_save("window_bartlett", output, options + ", title='Bartlett window'");
```
Result
![window_bartlett](img/window_bartlett.svg)

## Cosine window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_cosine(output.size());
plot_save("window_cosine", output, options + ", title='Cosine window'");
```
Result
![window_cosine](img/window_cosine.svg)

## Cosine window (numpy compatible)

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_cosine_np(output.size());
plot_save("window_cosine_np", output, options + ", title='Cosine window (numpy compatible)'");
```
Result
![window_cosine_np](img/window_cosine_np.svg)

## Bartlett-Hann window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_bartlett_hann(output.size());
plot_save("window_bartlett_hann", output, options + ", title='Bartlett-Hann window'");
```
Result
![window_bartlett_hann](img/window_bartlett_hann.svg)

## Bohman window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_bohman(output.size());
plot_save("window_bohman", output, options + ", title='Bohman window'");
```
Result
![window_bohman](img/window_bohman.svg)

## Lanczos window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_lanczos(output.size());
plot_save("window_lanczos", output, options + ", title='Lanczos window'");
```
Result
![window_lanczos](img/window_lanczos.svg)

## Flat top window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_flattop(output.size());
plot_save("window_flattop", output, options + ", title='Flat top window'");
```
Result
![window_flattop](img/window_flattop.svg)

## Kaiser window

Code
```c++ linenums="1"
const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                            "log_freq=False, horizontal=False, normalized_freq=True";
univector<fbase, 64> output;

output = window_kaiser(output.size(), 2.5);
plot_save("window_kaiser", output, options + ", title='Kaiser window'");
```
Result
![window_kaiser](img/window_kaiser.svg)
