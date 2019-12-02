# Expressions

Calling functions on arrays of data is performed lazily using C++ template expressions.
This allows better optimization and does not require saving temporary data.

For example, subtracting one univector from another gives expression type, not univector:

```c++
univector<int, 5> x{1, 2, 3, 4, 5};
univector<int, 5> y{0, 0, 1, 10, -5};

auto z = x - y; // z is of type expression, not univector. 
                // This only constructs an expression and does not perform any calculation
```

But you can always convert expression back to univector to get actual data:

```c++
univector<int, 5> x{1, 2, 3, 4, 5};
univector<int, 5> y{0, 0, 1, 10, -5};    

univector<int, 5> z = x - y;
```

!!! note
    when an expression is assigned to a `univector` variable, expression is evaluated
    and values are being written to the variable.

Same applies to calling KFR functions on univectors, this doesn't calculate value immediately. Instead, new expression will be created.

```c++
univector<float, 5> x{1, 2, 3, 4, 5};
sqrt(x);                                // only constructs an expression
univector<float, 5> values = sqrt(x);   // constructs an expression and writes data to univector  
```

Input expressions can be read from and output expressions can be written to. Class can be an input and output expression at same time. `univector` is an example of such class.

Data type of an input expressions can be determined by using `value_type_of<Expression>`. However, not all expressions have their types specified.
In such cases `value_type_of` will return special type `generic`.

Size (length) of an expression also can be specified or not. `counter` is an example of generic (untyped) expression without the size.