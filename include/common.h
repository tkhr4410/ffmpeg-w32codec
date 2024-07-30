// Copyright (c) 2024 Takahiro Ishida
// Licensed under the MIT License.

#pragma once

extern void debug_printf(const char *fmt, ...);

#if _DEBUG
#define LOGE(fmt, ...)	debug_printf("E/" LOG_TAG ": " fmt "\n", ##__VA_ARGS__)
#define LOGW(fmt, ...)	debug_printf("W/" LOG_TAG ": " fmt "\n", ##__VA_ARGS__)
#define LOGI(fmt, ...)	debug_printf("I/" LOG_TAG ": " fmt "\n", ##__VA_ARGS__)
#define LOGD(fmt, ...)	debug_printf("D/" LOG_TAG ": " fmt "\n", ##__VA_ARGS__)
#else
#define LOGE(fmt, ...)  do {} while (0)
#define LOGW(fmt, ...)  do {} while (0)
#define LOGI(fmt, ...)  do {} while (0)
#define LOGD(fmt, ...)  do {} while (0)
#endif
