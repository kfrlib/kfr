/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
 */
 
 #include "cometa/string.hpp"

#include "version.hpp"
#include "base/abs.hpp"
#include "base/asin_acos.hpp"
#include "base/atan.hpp"
#include "base/complex.hpp"
#include "base/constants.hpp"
#include "base/digitreverse.hpp"
#include "base/dispatch.hpp"
#include "base/function.hpp"
#include "base/gamma.hpp"
#include "base/log_exp.hpp"
#include "base/logical.hpp"
#include "base/memory.hpp"
#include "base/min_max.hpp"
#include "base/operators.hpp"
#include "base/read_write.hpp"
#include "base/round.hpp"
#include "base/saturation.hpp"
#include "base/select.hpp"
#include "base/shuffle.hpp"
#include "base/sin_cos.hpp"
#include "base/sinh_cosh.hpp"
#include "base/sqrt.hpp"
#include "base/tan.hpp"
#include "base/types.hpp"
#include "base/univector.hpp"
#include "base/vec.hpp"
#include "expressions/basic.hpp"
#include "expressions/conversion.hpp"
#include "expressions/generators.hpp"
#include "expressions/operators.hpp"
#include "expressions/pointer.hpp"
#include "expressions/reduce.hpp"
#include "dispatch/cpuid.hpp"
#include "dispatch/runtimedispatch.hpp"

#include "misc/compiletime.hpp"
#include "misc/random.hpp"
#include "misc/small_buffer.hpp"
#include "misc/sort.hpp"

#include "data/bitrev.hpp"
#include "data/sincos.hpp"
#include "dsp/biquad.hpp"
#include "dsp/fir.hpp"
#include "dsp/goertzel.hpp"
#include "dsp/interpolation.hpp"
#include "dsp/oscillators.hpp"
#include "dsp/resample.hpp"
#include "dsp/speaker.hpp"
#include "dsp/units.hpp"
#include "dsp/weighting.hpp"
#include "dsp/window.hpp"
#include "io/audiofile.hpp"
#include "io/file.hpp"
#include "io/python_plot.hpp"
#include "io/tostring.hpp"
#include "math.hpp"
#include "vec.hpp"

#include "dft/bitrev.hpp"
#include "dft/fft.hpp"
#include "dft/ft.hpp"
#include "dft/reference_dft.hpp"
