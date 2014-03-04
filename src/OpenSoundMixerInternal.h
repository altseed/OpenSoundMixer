
#pragma once

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>
#include <assert.h>

#if _WIN32
#define	STRICMP _stricmp
#else
#define	STRICMP strcasecmp
#endif

