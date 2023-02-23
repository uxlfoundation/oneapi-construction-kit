# Abacus

Abacus is Codeplay's kernel library for compute languages.

Key features include;

- OpenCL 1.2 floating point math functions.
- Heavily optimized for GPU, DSP and vector architectures.
- Satisfies the high precision requirements for OpenCL conformance.
- High code quality and documentation for easy maintainability.

## Release Notes

### Jam Roly Poly - 12th July 2016

The Jam Roly Poly release marks a broad stabilization of the Abacus codebase -
the code is shipping in many more platforms now, we have wide testing support
from a plethora of architectures, and the code is working perfectly well for our
customers.

- Huge tidy up of the code, unifying a number of built up conventions.
- Added flush-to-zero support throughout Abacus.
- Fixed some CTS-untested corner cases in our algorithms.

### Iced Bun - 9th October 2015

The highlight of the Iced Bun release finalized our double support by having
explicitly vectorized variants of all the double functions on offer.

- full double support enabled (all vectorizations return correctly).
- added `native_*` builtins for float.
- super fast new algorithm for `acos`/`asin` double bringing 1.5-10x speed up.
- spliced out some of the non-IP functionality into `abacus_detail_*` headers. A
  customer of Abacus really needed, for ease of use, these to be in a header to
  allow for function inlining, code size reduction, and code shipping ease.

### Hairst Bree - 17th July 2015

The Hairst Bree release was a broadening of the scope of the math library, we've
brought in nearly all of the functionality required to fully implement the
OpenCL runtime, the only things missing are atomics (which required specific
software solutions on each platform), and `vload`/`vstore` for half. These
implementations are platform agnostic, they'll work out of the box on whatever
platform required.

- 5.6x performance improvement for `hypot(float, float)`.
- We now generate our abacus headers via some CMake wizardry.
- Abacus now includes most of the OpenCL runtime, everything except atomic
  builtins and `vload`/`vstore` for half.
- Initial support for double floating point is in the process of being added.
- Abacus builds to a C++ static and shared library.

### Granny Sookers - 13th November 2014

The Granny Sookers release has been a spring clean release - we've spent a good
deal of time simplifying the maths library, cleaning up the headers, and trying
to make the product much more usable for our customers. But of course - we've
snuck in some really cool little features too;

- C++ compilation mode
- Monolithic headers separation begun
- Performance improvements for `pow`, `powr` & `pown` functions

Previously, we referred to this product at Codeplay's Math library, but the
library has become so much more than that. To match the evolving focus of the
library and to not constrain us, we've decided on a rename. Welcome to the
wonderful world our new product, Abacus.

## Hardware Requirements

Abacus has been designed as carefully as possible to allow for low level
computation on our customers hardware, while also as much as possible trying to
abstract what we can away from the underlying hardware.

We do make certain assumptions on what the underlying hardware is capable of
though.

### Basic Type Requirements

We use the following types, though they may not be natively supported:

- signed 8-bit integer
- signed 16-bit integer
- signed 32-bit integer
- signed 64-bit integer
- unsigned 8-bit integer
- unsigned 16-bit integer
- unsigned 32-bit integer
- unsigned 64-bit integer
- 16-bit floating point
- 32-bit floating point
- 64-bit floating point
- 1/2/3/4/8/16 element vectors of the above types

### Denorm Support

Abacus fully supports denormal numbers for our floating point functions.

### Rounding Modes

Our default rounding mode is Round-to-Even.

### ULP Requirements for Basic Operations

Since Abacus was incepted as a kernel library to provide OpenCL builtins on GPU
hardware, we follow the requirements OpenCL places on target hardware for basic
operations.

#### What is ULP?

ULP stands for Unit in the Last Place, or Unit of Least Precision, is the
spacing between floating-point numbers, used as a measurement of accuracy of
floating point numbers. More information can be found on
[Wikipedia](https://en.wikipedia.org/wiki/Unit_in_the_last_place).

For example, say we would like to compute the ULP of 3.14159, we first extract
the exponent using `ilogb`, then plug that into the equation `ULP(x) = 2^(e-p+1)`
(where `p` is `24` if we're dealing with FP32):

```cpp
float number = 3.14159f;
int exponent = std::ilogb(number); // 1
float ulp = std::ldexp(1.0f, exponent - 23); // 2.38419e-07
```

We can also use the alternative definition where `ULP(x)` is the distance
between the two closest straddling floating-point numbers `a` and `b` (i.e.
`a <= x <= b` and `a != b`) to verify this:

```cpp
float number = 3.14159f;
float a = number; // `number` is the same as `a`, due to FP rounding.
float b = std::nextafter(number, 4.0f); // next floating point value towards 4.
float ulp = b - a; // 2.38419e-07
```

##### Calculating the ULP error of an FP16 function

Lets say we have implemented an FP16 function `sinpi(x)`, which calculates
`sin(pi * x)`, and for a specific input, it calculates the following result:

```
sinpi(1.234) = -0.67041 [0xb95d]
```

We know that the _true_ FP16 value is `-0.671387 [0xb95f]`. This is the
infinitely precise result of `sinpi(1.234)` rounded to FP16. We can calculate
the ULP error by working out the difference between the actual and expected
value, then divide by the ULP of the expected result (where `p` is 11 for FP16
in the ULP equation):

```
result = -0.67041
expected = -0.671387
expected_exp = ilogb(expected) = -1
ulp_diff = (result - expected) / 2^(expected_exp - 11 + 1) = 2.000896
```

If the specification says that `sinpi(x)` must be within 2 ULP of the exact
answer, then this result would not be conformant, as the ULP difference is
greater than 2.

#### 16-bit Floating Point

We assume that IEEE 754 compliant floating point numbers are available on the
hardware, with the following ULP guarantees.

- `x + y`, `x` and `y` are 16 bit floating point numbers, correctly rounded.
- `x - y`, `x` and `y` are 16 bit floating point numbers, correctly rounded.
- `x * y`, `x` and `y` are 16 bit floating point numbers, correctly rounded.
- `x / y`, `x` and `y` are 16 bit floating point numbers, <= 2.5 ULP.

#### 32-bit Floating Point

We assume that IEEE 754 compliant floating point numbers are available on the
hardware, with the following ULP guarantees.

- `x + y`, `x` and `y` are 32 bit floating point numbers, correctly rounded.
- `x - y`, `x` and `y` are 32 bit floating point numbers, correctly rounded.
- `x * y`, `x` and `y` are 32 bit floating point numbers, correctly rounded.
- `x / y`, `x` and `y` are 32 bit floating point numbers, <= 2.5 ULP.

#### 64-bit Floating Point

We assume that IEEE 754 compliant floating point numbers are available on the
hardware, with the following ULP guarantees.

- `x + y`, `x` and `y` are 64 bit floating point numbers, correctly rounded.
- `x - y`, `x` and `y` are 64 bit floating point numbers, correctly rounded.
- `x * y`, `x` and `y` are 64 bit floating point numbers, correctly rounded.
- `x / y`, `x` and `y` are 64 bit floating point numbers, <= 2.5 ULP.

### Notes on Our Implementation

While we require support for 64-bit signed and unsigned integer types - we try
as much as possible to avoid using them. Since Abacus is primarily written with
customer GPUs in mind, we have only used 64-bit types where we would otherwise
have to emulate 64-bit operations using 32-bit hardware. In practice, the most
performance intensive section of Abacus (`abacus_math`) uses 64-bit types only
when calling `modf`, remainder and `remquo`.

## Implementation

Notes on algorithms used to implement the maths library, as well as ideas for
future optimizations.

### Maple Worksheets

The `maple` directory of abacus contains files with the `.mw` extension,
these are worksheets which can be loaded into the Maple software environment.
Worksheets provide mathematical expressions, computations, and plots visualizing
how the algorithm implementing each builtin function was derived.

The Maple software environment includes a free viewer called Maple Player which
can be used to view the worksheets. A copy of Maple Player can be downloaded
from [here](https://www.maplesoft.com/products/maple/MaplePlayer/mapleplay-download.aspx)

### Sollya

The [Sollya](http://sollya.gforge.inria.fr/) linux command line tool is used for
generating polynomial coefficients. Scripts for recreating how these values
were calculated can be found in the `sollya` folder.

When writing a new script we want to call the `fpminmax` sollya function, which
uses a practically efficient heuristic method to find a good polynomial
approximation of a function `f` on an interval range. Our function `f` will be
the original function we want to find a polynomial for, but the range should
correspond to narrowed ranged obtained using a range reduction algorithm.

We also need to pass `fpminmax` the order of the polynomial to generate, higher
orders are more precise but also more computationally expensive. To achieve a
good trade off we work out the error of our polynomial using `dirtyinfnorm`.
Then pick an order which results in an error just smaller than can be
represented by the floating point type we're interested in, one of half/float/double
precisions.

### Algorithms

Outline of common algorithms used throughout Abacus.

#### Newton Raphson Method

The NR method is a technique for finding approximations for the roots(zeros) of
a function. By starting with an initial guess for the root of the function, we
can perform repeated iterations of the algorithm to get increasingly precise
values.

In particular we use NR as part of the [fast inverse square root][rsqrt]
algorithm found in Quake, which is used as part of the implementation of many
of the builtins.

### Halley's Method

Like the NR method Halley's method is a root finding algorithm for functions,
however it improves the rate of convergence to the root for cubic functions so
we use it as part of our `cbrt` implementation.

#### Horner's Method

Horner's method is an algorithm for evaluating polynomials for a specific value
of x by transforming the polynomial into a computationally efficient form. It's
particularly useful for implementing fast multiplication and division of
floating point numbers on target architectures without bespoke hardware, since
the binary number can be represented as a polynomial of base 2.

#### Cody Waite

Cody-Waite is a range reduction algorithm, used to bound the interval of inputs to
an elementary function. Cody-Waite is used when you are doing range reduction by
subtracting a known constant value. For example, when calculating an exponential
function, for instance `exp(x)`, you will first reduce the range by subtracting
the constant value of `log(2)`. The problem occurs when you have a value of `x`
that is close to `log(2)`, you will get a big error in the subtraction when
creating the very small result. To combat this, the Cody-Waite breaks the single
`log(2)` constant into multiple sub-constants (normally 2 values, but sometimes
more!). Then we subtract these parts of the same constant multiple times from
the original value, starting with the largest part of the constant, then the
next smallest, and again and again. The result is we get a much more precise
resulting value after all these subtractions have occurred.

To calculate a Cody-Waite constant, you first get a very high precision value of
the constant you want (for instance using Wolfram Alpha). Then you round this
value into the precision of the value you are calculating, so for instance into
32-bit floating point. You want to round down here on the last digit of the
number. Then subtract the rounded value from the high precision value (again
using something like Wolfram Alpha), and the resulting value is your second
Cody-Waite value. Generally you want the values being subtracted to always be
positive, so if you get a negative result it means you didn't round the original
value down enough, and need to cut it lower.

#### Payne Hanek

Payne Hanek is a range reduction algorithm specifically for trigonometric
functions. Trigonometric functions are unique in that `f(x) = sin(x)` results
in a repeated pattern across the entire range `(sin(x) == sin(x + 2 * π))`.
Large floating point values need to be precisely modulo'd down into this
repeated range, but using any normal floating point tricks would result in
wildly wrong guesses. Payne Hanek solves this by using a large lookup table
representing many digits of π. Then, based on how large the floating point
value is, the relevant digits of π are used to modulo x down into the range
precisely.

### Development Techniques

There are a number of common design patterns used throughout Abacus to achieve
the required ULP precision targets.

#### Reading Materials

* "Handbook of Floating-Point Arithmetic" by Jean-Michel Muller et al.
* "Software Manual For The Elementary Functions" by William J Cody Jr and William Waite.

#### Coding Conventions

Pass OpenCL built-in scalar and vector types (`float`, `float2`, etc.) by value
rather than by reference to helper functions. This elminates unnecessary
`alloca` instructions which can hinder optimizations like the vectorizer.

#### Exact Operations

Abacus includes a number of floating point helper functions to enable more
precise addition and multiplication operations, without needing to cast to a
higher precision type. The idea is that the result of an operation `a OP b` is
decomposed into two numbers `hi` and `lo` which fit into the required
floating point data type. `hi` contains the larger magnitude number and `lo`
contains the smaller magnitude number.

`hi` and `lo` should then be operated on independently, then combined together
later. The idea is that enough precision should be retained in intermediate
operations, so the final recombined result is more accurate than if the
decomposition wasn't used.

For example, `(a + b) * c` could be implemented in the following way:

```c
abacus_half result_lo;
abacus_half result_hi = add_exact(a, b, &result_lo);
result_lo *= c;
result_hi *= c;
abacus_half final = result_hi + result_lo;
```

#### Chaining Exact Operations

It is possible to chain exact operations to further increase the final precision
depending on the magnitudes of the intermediate numbers. For example, computing
the same example as above:

```c
abacus_half int_lo;
abacus_half int_hi = add_exact(a, b, &int_lo);

abacus_half result_lo
abacus_half result_hi = multiply_exact(int_hi, c, &result_lo);

// We've multiplied int_hi and c, but we also need to multiply int_lo and c. add
// this to result_lo, as it will more than likely have a low enough magnitude to
// hold the result.
result_lo += int_lo * c;

// result_hi and result_lo represents the decomposition of the final value.
```

or instead, computing `a * b + c`:

```
abacus_half int_lo;
abacus_half int_hi = multiply_exact(a, b, &int_lo);

abacus_half result_lo
abacus_half result_hi = add_exact(int_hi, c, &result_lo);
result_lo += int_lo;

// result_hi and result_lo represents the decomposition of the final value.
```

#### Utilising Exact Operations In Horner Polynomials

Horner Polynomials are used to implement a number of functions. As this
calculation involves chaining many multiplications and additions, it can be made
more accurate by replacing parts of the Horner Polynomial calculation with calls
to `multiply_exact` and `add_exact`. An example of this can be found in the half
implementation of `acos`.

### Optimization Ideas

Notes from one of the significant contributors to Abacus about how to
optimize our implementation. These may or may not still be valid.

#### pown

Somehow abuse the fact that the 32 bit int gets cast perfectly to a double, less
accuracy is probably needed.

#### tan

Value of `pi/4` in 32 bit p-h may not be the best one, it seems to be off by a
(base 10) digit. (so maybe it'd be the same) Might not need extra rtz code if
that's the case.

Pad the start of `payload[]` with 0's, change it so that works, and you won't
need to do cody waithe at all. Instead of payloads entirely use an array of
floats/double and use exact multiplies and adds. Could be big, but would take
lots of time

Don't multiply by `fractPiOver4` at the end of p-h. Incorporate it into the
final polynomial estimate. Would take out all that big RTZ code :D (done in double).

#### double tan

Polynomials could be better. Need extended precision for proper testing so I'll
have to leave it for now.

#### sin cos

Polynomials can probably be made a bit better.

#### sinpi & cospi

You might be able to use a lower degree polynomial, and it just didn't pass on
my computer because of the +0.5 ULP. Might pass on a higher precision machine.
`cospi` uses the same polynomial so be sure to change both. Maple didn't like
the `cospi` one so I had to work around it

#### sqrt float

I seem to remember you don't need to do the checks for `xBig` if you change the
NR iteration to be:

```
result = 0.5f * result * (3.0f - result * (result * x));
```

Instead of:

```
result = 0.5f * result * (3.0f - (result * result) * x);
```

Or for some combination of them. Free opt if you can get it. Thought I'd put it
into BC's code but maybe not. There might have had to be an odd number of NR
iterations too or something to that effect.

#### atan2

This 19 degree polynomial was the best that can be found under the current
algorithm. I imagine you could maybe split the interval into 2 or 4 or `w/e` and
have different polynomials over it, but I'm not sure how much of a speedup you
could expect really.

> We should ignore above advice, split interval is bad for vector code.

#### exp double

Use a lesser degree polynomial. Don't think much else goes here.

#### tanh

Prime candidate for polynomial estimate. You need to do polynomials in the
range (0,18) to be done. They'll probably be long, but better than the divide
that's there at the moment.

#### erfc

Not really much. The problem here is what you're testing against, as there's
reference test. Intels is lacking in the [26.5,27.0] department, where the
answers are denormal, but we seem to match them everywhere but there.
More than likely all the polynomials can be shortened, but having to calculate
`exp(x^2)` accurately seems to have to happen.

#### erf

Could separate it from `erfc`, but again what do you test against. Used `erfc`
to make sure it's overly accurate

### tgamma

Keep a separate track of above and below fractions so you only need to do one
divide. Just try find a better algorithm in general! It's quite a hard function.

#### lgamma

Problem once again being an incomplete reference function. Our one is better
than the test one anyway. It's very branchy, but I think it needs to be. For
x > 10 stirlings approximation is the way to go unless you want to split the
whole double range up into polynomials. For x > 0 we have a max ulp < 16, and
for x > 15 a max ulp of something very small, maybe 1.

#### rsqrt

Take out the last more accurate Newton Rhapson iteration if you can. It's very
slow. Take out the `frexp` and `ldexp` also, that'd be a big saving. I'll do
this if there's time.

#### cbrt

The polynomial could probably be shorter. Maybe a better general algorithm, one
without a divide. It seems to be quite hard to find one. A more streamlined
`frexp`/`ldexp` is probably in order too.

#### exp10

Instead of the normal cody waithe, which leads to a long enough polynomial, we've
reduced `mod ln(10)/2ln(2)` instead of `ln(10)/ln(2)`, aka half the interval.
This reduces the polynomial but you have to do one additional small branch.
Probably worth it on CPU anyway. Probably don't need the 4th cody waithe
constant either, I suspect not.

Edit: it was only 1 (2?) extra polynomial term saved so scrapping this idea.
Maybe an early return for Nans would be better, though it's not needed.

#### expm1

I couldn't get it to pass, but I think it's because of the extra 0.5 ulp that
gets added onto on my machine. Is probably fine on cpu but could maybe be
streamlined a bit.

#### rootn

Is a little bit too accurate at the moment, reducing the second polynomial is
the best way to make it less accurate.

#### log1p

Is probably fine for a branchy algorithm, not sure how to best speed it
up/refactor it.

#### remquo & remainder

Find a way of doing bigger reductions to the input variables in the cases where
they're far apart. This is going to involve guessing an integer as close as
possible to `q = (x/m)`, the closer the better, (`not x/m` will overflow so we
need to be smarter, then calculating `xNew = x - q*m`  via exact multiplication
and addition. If `q` is close enough to `(x/m)` this will be exact, and we have
`remquo(x,m) = remquo(xNew, m)`. Should be possible without too much work, at
the moment in the worst case you can have a loop that goes on for
`2*1024 / 11 = 187` loops. Argh.

[rsqrt]: https://en.wikipedia.org/wiki/Fast_inverse_square_root#Newton's_method
