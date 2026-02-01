#include "log/log.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* Static state */
static struct {
    LogConfig config;
    FILE *file;
    pthread_mutex_t mutex;
    bool initialized;
} log_state = {
    .config = {
        .level = LOG_LEVEL_INFO,
        .file_path = NULL,
        .max_file_size = 100 * 1024 * 1024,  /* 100 MB default */
        .rotate_count = 5,
        .log_to_stdout = true,
        .include_timestamp = true,
        .include_level = true,
        .include_location = true
    },
    .file = NULL,
    .initialized = false
};

/* Level names */
static const char *level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

/* Level colors for terminal output */
static const char *level_colors[] = {
    "\x1b[36m",  /* DEBUG: cyan */
    "\x1b[32m",  /* INFO: green */
    "\x1b[33m",  /* WARN: yellow */
    "\x1b[31m",  /* ERROR: red */
    "\x1b[35m"   /* FATAL: magenta */
};

static const char *color_reset = "\x1b[0m";

/* Check if stdout is a terminal */
static bool is_terminal(void) {
    return isatty(fileno(stdout));
}

/* Get current file size */
static size_t get_file_size(FILE *f) {
    if (f == NULL) {
        return 0;
    }
    long pos = ftell(f);
    if (fseek(f, 0, SEEK_END) != 0) {
        return 0;
    }
    long size = ftell(f);
    fseek(f, pos, SEEK_SET);
    return (size_t)(size > 0 ? size : 0);
}

/* Rotate log files */
static void rotate_logs(void) {
    if (log_state.config.file_path == NULL || log_state.config.rotate_count <= 0) {
        return;
    }

    /* Close current file */
    if (log_state.file != NULL) {
        fclose(log_state.file);
        log_state.file = NULL;
    }

    /* Rotate existing files */
    char old_path[4096];
    char new_path[4096];

    /* Remove oldest file */
    snprintf(old_path, sizeof(old_path), "%s.%d",
             log_state.config.file_path, log_state.config.rotate_count);
    unlink(old_path);

    /* Rotate remaining files */
    for (int i = log_state.config.rotate_count - 1; i >= 1; i--) {
        snprintf(old_path, sizeof(old_path), "%s.%d", log_state.config.file_path, i);
        snprintf(new_path, sizeof(new_path), "%s.%d", log_state.config.file_path, i + 1);
        rename(old_path, new_path);
    }

    /* Rotate current file */
    snprintf(new_path, sizeof(new_path), "%s.1", log_state.config.file_path);
    rename(log_state.config.file_path, new_path);

    /* Open new file */
    log_state.file = fopen(log_state.config.file_path, "a");
}

int log_init(const LogConfig *config) {
    if (log_state.initialized) {
        return 0;  /* Already initialized */
    }

    if (pthread_mutex_init(&log_state.mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize log mutex\n");
        return -1;
    }

    if (config != NULL) {
        log_state.config = *config;
    }

    /* Open log file if specified */
    if (log_state.config.file_path != NULL && log_state.config.file_path[0] != '\0') {
        log_state.file = fopen(log_state.config.file_path, "a");
        if (log_state.file == NULL) {
            fprintf(stderr, "Failed to open log file '%s': %s\n",
                    log_state.config.file_path, strerror(errno));
            /* Continue without file logging */
        }
    }

    log_state.initialized = true;
    return 0;
}

void log_shutdown(void) {
    if (!log_state.initialized) {
        return;
    }

    pthread_mutex_lock(&log_state.mutex);

    if (log_state.file != NULL) {
        fclose(log_state.file);
        log_state.file = NULL;
    }

    log_state.initialized = false;
    pthread_mutex_unlock(&log_state.mutex);
    pthread_mutex_destroy(&log_state.mutex);
}

void log_set_level(LogLevel level) {
    if (level > LOG_LEVEL_FATAL) {
        level = LOG_LEVEL_FATAL;
    }
    log_state.config.level = level;
}

LogLevel log_get_level(void) {
    return log_state.config.level;
}

LogLevel log_level_from_string(const char *str) {
    if (str == NULL) {
        return LOG_LEVEL_INFO;
    }

    if (strcasecmp(str, "debug") == 0) {
        return LOG_LEVEL_DEBUG;
    }
    if (strcasecmp(str, "info") == 0) {
        return LOG_LEVEL_INFO;
    }
    if (strcasecmp(str, "warn") == 0 || strcasecmp(str, "warning") == 0) {
        return LOG_LEVEL_WARN;
    }
    if (strcasecmp(str, "error") == 0) {
        return LOG_LEVEL_ERROR;
    }
    if (strcasecmp(str, "fatal") == 0) {
        return LOG_LEVEL_FATAL;
    }

    return LOG_LEVEL_INFO;
}

const char *log_level_to_string(LogLevel level) {
    if (level > LOG_LEVEL_FATAL) {
        return "UNKNOWN";
    }
    return level_names[level];
}

void log_write(LogLevel level, const char *file, int line,
               const char *func, const char *fmt, ...) {
    /* Check log level */
    if (level < log_state.config.level) {
        return;
    }

    /* Initialize if not already done (allows logging before explicit init) */
    if (!log_state.initialized) {
        if (log_init(NULL) != 0) {
            return;
        }
    }

    /* Build timestamp */
    char timestamp[32] = "";
    if (log_state.config.include_timestamp) {
        time_t now = time(NULL);
        struct tm tm_info;
        localtime_r(&now, &tm_info);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_info);
    }

    /* Build location string */
    char location[256] = "";
    if (log_state.config.include_location) {
        /* Extract filename from path */
        const char *filename = file;
        const char *slash = strrchr(file, '/');
        if (slash != NULL) {
            filename = slash + 1;
        }
        snprintf(location, sizeof(location), " [%s:%d %s()]", filename, line, func);
    }

    /* Format message */
    char message[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    pthread_mutex_lock(&log_state.mutex);

    /* Check for log rotation */
    if (log_state.file != NULL && log_state.config.max_file_size > 0) {
        size_t current_size = get_file_size(log_state.file);
        if (current_size >= log_state.config.max_file_size) {
            rotate_logs();
        }
    }

    /* Write to file */
    if (log_state.file != NULL) {
        if (log_state.config.include_timestamp) {
            fprintf(log_state.file, "%s ", timestamp);
        }
        if (log_state.config.include_level) {
            fprintf(log_state.file, "[%s]", level_names[level]);
        }
        fprintf(log_state.file, "%s %s\n", location, message);
        fflush(log_state.file);
    }

    /* Write to stdout */
    if (log_state.config.log_to_stdout) {
        bool use_color = is_terminal();

        if (log_state.config.include_timestamp) {
            fprintf(stdout, "%s ", timestamp);
        }
        if (log_state.config.include_level) {
            if (use_color) {
                fprintf(stdout, "%s[%s]%s", level_colors[level], level_names[level], color_reset);
            } else {
                fprintf(stdout, "[%s]", level_names[level]);
            }
        }
        fprintf(stdout, "%s %s\n", location, message);
        fflush(stdout);
    }

    pthread_mutex_unlock(&log_state.mutex);

    /* Exit on fatal */
    if (level == LOG_LEVEL_FATAL) {
        log_shutdown();
        exit(EXIT_FAILURE);
    }
}
