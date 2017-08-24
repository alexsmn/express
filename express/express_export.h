#pragma once

#ifdef COMPONENT_BUILD

#ifdef EXPRESS_IMPLEMENTATION
#define EXPRESS_EXPORT __declspec(dllexport)
#else
#define EXPRESS_EXPORT __declspec(dllimport)
#endif

#else // COMPONENT_BUILD

#define EXPRESS_EXPORT

#endif // COMPONENT_BUILD
