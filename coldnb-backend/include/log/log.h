#ifndef COLDNB_LOG_H
#define COLDNB_LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

/* Log levels */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4
} LogLevel;

/* Log configuration */
typedef struct {
    LogLevel level;
    const char *file_path;
    size_t max_file_size;
    int rotate_count;
    bool log_to_stdout;
    bool include_timestamp;
    bool include_level;
    bool include_location;
} LogConfig;

/* Initialize logging system
 * Returns 0 on success, -1 on error */
int log_init(const LogConfig *config);

/* Shutdown logging system */
void log_shutdown(void);

/* Set minimum log level */
void log_set_level(LogLevel level);

/* Get current log level */
LogLevel log_get_level(void);

/* Parse log level from string (e.g., "info", "debug")
 * Returns LOG_LEVEL_INFO on invalid input */
LogLevel log_level_from_string(const char *str);

/* Get log level name as string */
const char *log_level_to_string(LogLevel level);

/* Core logging function - prefer macros below */
void log_write(LogLevel level, const char *file, int line,
               const char *func, const char *fmt, ...);

/* Logging macros with file/line info */
#define LOG_DEBUG(fmt, ...) \
    log_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    log_write(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    log_write(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    log_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_FATAL(fmt, ...) \
    log_write(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

/* Conditional debug logging - compiles out in release builds */
#ifdef DEBUG
#define LOG_TRACE(fmt, ...) LOG_DEBUG(fmt, ##__VA_ARGS__)
#else
#define LOG_TRACE(fmt, ...) ((void)0)
#endif

#endif /* COLDNB_LOG_H */
