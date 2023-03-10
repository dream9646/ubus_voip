/******************************************************************
 *
 * Copyright (C) 2013 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT memdbg protocol stack
 * Module  : memdbg
 * File    : memdbg_log.h
 *
 ******************************************************************/
#ifndef MEMDBG_LOG_H
#define MEMDBG_LOG_H

#ifdef MEMDBG_LOG_LIB_ENABLE

#include "log.h"

#ifndef MODULE
#define MODULE memdbg
#endif

#ifndef CONFIG_MEMDBG_LOGLEVEL
#define CONFIG_MEMDBG_LOGLEVEL LOG_LEVEL_INFO
#endif

LOG_DECLARE_MODULE();

#else

#define LOG_DEFINE_MODULE(x)
#define LOG_REGISTER_MODULE()

# define LOGV(fmt, ...)  printf("Verbose: " fmt"\n" , ## __VA_ARGS__)
# define LOGD(fmt, ...)  printf("Debug: " fmt"\n" , ## __VA_ARGS__)
# define LOGI(fmt, ...)  printf("Info: " fmt "\n" , ## __VA_ARGS__)
# define LOGN(fmt, ...)  printf("Notice: " fmt "\n" , ## __VA_ARGS__)
# define LOGW(fmt, ...)  printf("Warning: " fmt "\n" , ## __VA_ARGS__)
# define LOGE(fmt, ...)  fprintf(stderr, "Error: " fmt "\n" , ## __VA_ARGS__)
# define LOGF(fmt, ...)  fprintf(stderr, "Fatal: " fmt "\n" , ## __VA_ARGS__)

# define LOGAV(fmt, ...)  LOGV(fmt, ## __VA_ARGS__)
# define LOGAD(fmt, ...)  LOGD(fmt, ## __VA_ARGS__)
# define LOGAI(fmt, ...)  LOGI(fmt, ## __VA_ARGS__)
# define LOGAN(fmt, ...)  LOGN(fmt, ## __VA_ARGS__)
# define LOGAW(fmt, ...)  LOGW(fmt, ## __VA_ARGS__)
# define LOGAE(fmt, ...)  LOGE(fmt, ## __VA_ARGS__)
# define LOGAF(fmt, ...)  LOGF(fmt, ## __VA_ARGS__)

# define SYSLOGW(id, fmt, ...)  fprintf(stderr, "SyslogW[%d]: " fmt "\n", id, ## __VA_ARGS__)


#endif	/* MEMDBG_LOG_LIB_ENABLE */
#endif /* MEMDBG_LOG_H */
