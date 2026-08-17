#ifndef EASY_LOG_H
#define EASY_LOG_H
#include "easy_log_internal.h"

#define EA_LOG_E(format, ...) \
    do { \
        easy_log_level_printf(EASY_LOG_LEVEL_ERROR, __func__, __LINE__, \
                              format, ##__VA_ARGS__); \
    } while (0)

#define EA_LOG_W(format, ...) \
    do { \
        easy_log_level_printf(EASY_LOG_LEVEL_WARNING, __func__, __LINE__, \
                              format, ##__VA_ARGS__); \
    } while (0)

#define EA_LOG_I(format, ...) \
    do { \
        easy_log_level_printf(EASY_LOG_LEVEL_INFO, __func__, __LINE__, \
                              format, ##__VA_ARGS__); \
    } while (0)

#define EA_LOG_D(format, ...) \
    do { \
        easy_log_level_printf(EASY_LOG_LEVEL_DEBUG, __func__, __LINE__, \
                              format, ##__VA_ARGS__); \
    } while (0)

int easy_log_init(easy_log_link_t link);



#endif
