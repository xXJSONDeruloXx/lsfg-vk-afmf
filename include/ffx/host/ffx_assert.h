// Simplified FidelityFX assert for lsfg-vk-afmf
#pragma once

#include <assert.h>

#define FFX_ASSERT(condition) assert(condition)
#define FFX_FORWARD_DECLARE(x) typedef struct x x;
