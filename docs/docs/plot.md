# How to plot filter impulse response


## Show plot

```c++
univector<float> data;
// Fill data
plot_show("name", data, "title='name', div_by_N=True");
```

## Save plot

```c++
univector<float> data;
// Fill data
plot_save("name", data, "title='name', div_by_N=True");
```

The output will be saved as `name.svg`