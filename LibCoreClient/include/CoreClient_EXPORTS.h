#pragma once

#ifdef _WIN32
#define CORECLIENT_EXPORTS __declspec(dllexport)
#else
#define CORECLIENT_EXPORTS
#endif
