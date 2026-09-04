// Copyright (c) 2026 nux. All rights reserved.
//
// Shared export macros and ABI version for the closed SSMEL SDK.

#ifndef SSMEL_API_H
#define SSMEL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
  #if defined(SSMEL_BUILD_SHARED) || defined(SSMEL_ENGINE_BUILD_SHARED)
    #define SSMEL_API __declspec(dllexport)
  #elif defined(SSMEL_USE_SHARED) || defined(SSMEL_ENGINE_USE_SHARED)
    #define SSMEL_API __declspec(dllimport)
  #else
    #define SSMEL_API
  #endif
#else
  #if defined(SSMEL_BUILD_SHARED) || defined(SSMEL_ENGINE_BUILD_SHARED)
    #define SSMEL_API __attribute__((visibility("default")))
  #else
    #define SSMEL_API
  #endif
#endif

#define SSMEL_ENGINE_API SSMEL_API

/** Package version (zip name / CHANGELOG). Keep in sync with sdk/CHANGELOG.md. */
#define SSMEL_VERSION_MAJOR 1
#define SSMEL_VERSION_MINOR 0
#define SSMEL_VERSION_PATCH 0
#define SSMEL_VERSION_STRING "1.0.0"

/** Public ABI for ssmel_map / ssmel_engine. Bump when structs or exported symbols change. */
#define SSMEL_ABI_VERSION 1u

#ifdef __cplusplus
}
#endif

#endif  // SSMEL_API_H
