#pragma once

// Curve mapping constants (tweak to taste)

// Logarithmic curve: base used in log mapping (>1.0)
static constexpr float CURVE_LOG_BASE = 10.0f;

// Exponential curve: exponent multiplier used in exp(k*x)
static constexpr float CURVE_EXP_K = 3.0f;

// Sigmoidal curve: steepness parameter for logistic function
static constexpr float CURVE_SIGMOID_STEEPNESS = 12.0f;

// Tangent curve: scale factor controlling tangent steepness
static constexpr float CURVE_TAN_SCALE = 3.0f;

// General note: mappings take input in [0,1] and return [0,1]
