# How to apply Fast Fourier Transform

This article shows how to use Fast Fourier Transform and how to apply forward and inverse FFT on complex and real data using the KFR framework.

KFR DFT supports all sizes, KFR automatically chooses the best algorithm to perform DFT for the given size.

For power of 2 sizes, it uses Fast Fourier Transform.

## Quick example

### Complex input/output

Apply the FFT to the complex input data: [See how to pass data to KFR](types.md)

```c++
// prepare data (0, 1, 2, ..., 255)
univector<complex<double>, 256> data = cexp(
        linspace(0, c_pi<double, 2>, 256) * make_complex(0, 1));
univector<complex<double>, 256> freq;
 
// do the forward FFT
freq = dft(data);
```
To perform the inverse FFT:
```c++
data = idft(freq);
```

Scaling is not performed by KFR. To get output in the same scale as input, divide it by `size`.

### Real input, complex output

Frequency data are stored in [CCS or Perm format](dft_format.md).

The size of the output data is equal to `size/2+1` for CCS and `size/2` for Perm format.

For the inverse FFT, you have to prepare frequency data in the CCS or Perm format as well.

For CCS format, you must ensure that freq[0] and freq[N/2] are real numbers to get correct real result.

```c++
data = irealdft(freq);
// or to get the properly scaled result:
data = irealdft(freq) / data.size();
```

!!! note
    Real to complex and complex to real transforms are only available for even sizes.
    This is caused by the way real DFT is calculated. Pair of real values are interpreted as complex for high performance, so there is limitation for real DFT size, it must be even.
    Use complex transform and data conversion instead.
    ```c++
    const dft_plan<double> dft(N); // N is odd
    univector<complex<double>> output(N);
    univector<double> input;
    univector<u8> temp(dft.temp_size);
    dft.execute(output, univector<complex<double>>(input), temp);
    ```

## Creating FFT plan

Implementation of FFT requires twiddle coefficients to be prepared before actual processing occurs. If FFT will be performed more than once, then it makes sense to store the coefficients and reuse it every time.

## FFT Plan caching

If you are using `dft`, `idft`, `realdft` or `irealdft` functions, all plans will be kept in memory, so the next call to these functions will reuse the saved data.

You can manually get the plan from the cache (or create a new if it doesn’t exist in the cache):

```c++
dft_plan_ptr<T> dft = dft_cache::instance().get(ctype<T>, size);
```

!!! note
    All functions related to the FFT caching are protected with mutex and are thread safe

`ctype` is a special template variable intended just to pass a type to the function.

`dft_plan_ptr` is an alias for `std::shared_ptr<const dft_plan<T>>`

To clear all saved plans and free memory, call the clear function:

```c++
dft_cache::instance().clear();
```

!!! note
    As of version 1.3.0, functions convolve and correlate use these cache internally.

## Examples using plan

### Complex-to-complex transform

You can create a plan manually without using the cache to get more control over it.

`dft_plan` class is intended to store all data needed for FFT.

```c++
template <typename T> // data type, float or double
struct dft_plan
{
    dft_plan(size_t size);
    void execute(complex<T>* out, 
                 const complex<T>* in, 
                 u8* temp, 
                 bool inverse = false) const;
    void execute(univector<complex<T>, N>& out, 
                 const univector<const complex<T>, N>& in, 
                 univector<u8, N2>& temp, 
                 bool inverse = false) const;
    ...
};
```

`dft_plan` has constructor that takes FFT size as an argument.

Input and output arguments for calls to the execute function must have types `complex<T>*` or `univector<complex<T>, N>`.

You can create a plan and use it as many times as needed. After creating a plan, all access to it is thread-safe, because executing the FFT doesn’t modify its internal data,
so you can share this plan across threads in your application without protecting the plan with locks and mutexes.

```c++
dft_plan<double> plan(1024);
univector<complex<double>, 1024> in;
univector<complex<double>, 1024> out;
// here fill `in` array with our data (samples)
univector<u8> temp(plan.temp_size);
plan.execute(out, in, temp, false); // direct FFT
// `out` now contains frequencies which have to be processed
plan.execute(in, out, temp, true);  // inverse FFT
// `in` now contains processed data (samples)
...
// process new data
plan.execute(out, in, temp, false); // direct FFT
```

### Real to complex and complex to real transform

```c++
dft_plan_real<double> plan(1024); // dft_plan_real for real transform
univector<double, 1024> in;
univector<complex<double>, 1024> out;
// here fill `in` array with our data (samples)
univector<u8> temp(plan.temp_size);
plan.execute(out, in, temp); // direct FFT
// `out` now contains frequencies which have to be processed
plan.execute(in, out, temp); // inverse FFT
// `in` now contains processed data (samples)
```

