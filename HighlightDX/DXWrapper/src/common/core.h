#pragma once
// head files
#include <cstdint>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include <utility>
#include <type_traits>
#include <algorithm>
#include <functional>
#include <memory>
#include <csignal>
#include <vector>
#include <map>
#include <list>
#include <chrono>
#include <d3d11.h>
#include <dcomp.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include <mutex>


using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using float32 = float;
using float64 = double;

#ifdef DOUBLE_PRECISION
using real = float64;
#else
using real = float32;
#endif


// Constant
constexpr real PI = (real)3.14159265358979;
constexpr real INV_PI = (real)0.31830988618379067154;
constexpr real INV_2PI = (real)0.15915494309189533577;
constexpr real INV_4PI = (real)0.07957747154594766788;
constexpr real PI_Over2 = (real)1.57079632679489661923;
constexpr real PI_Over4 = (real)0.78539816339744830961;
