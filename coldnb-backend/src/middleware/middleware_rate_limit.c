#include "middleware/middleware_rate_limit.h"
#include "util/string_util.h"
#include "log/log.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Token bucket per IP address */
typedef struct IpBucket {
    char ip[64];                    /* Client IP string */
    double tokens;                  /* Current token count */
    time_t last_refill;             /* Last token refill time */
    time_t last_seen;               /* Last request time (for cleanup) */
    struct IpBucket *next;          /* Hash chain */
} IpBucket;

/* Hash table for IP buckets */
#define RATE_LIMIT_TABLE_SIZE 1024
#define AUTH_RATE_LIMIT_RPM   5     /* 5 login attempts per minute */

static IpBucket *g_table[RATE_LIMIT_TABLE_SIZE];
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static RateLimitConfig g_config;
static bool g_initialized = false;
static time_t g_last_cleanup = 0;

/* Simple hash for IP strings */
static unsigned int ip_hash(const char *ip) {
    unsigned int hash = 5381;
    while (*ip) {
        hash = ((hash << 5) + hash) + (unsigned char)*ip;
        ip++;
    }
    return hash % RATE_LIMIT_TABLE_SIZE;
}

/* Find or create bucket for an IP */
static IpBucket *get_bucket(const char *ip) {
    unsigned int idx = ip_hash(ip);
    IpBucket *bucket = g_table[idx];

    /* Search existing */
    while (bucket != NULL) {
        if (strcmp(bucket->ip, ip) == 0) {
            return bucket;
        }
        bucket = bucket->next;
    }

    /* Create new */
    bucket = calloc(1, sizeof(IpBucket));
    if (bucket == NULL) {
        return NULL;
    }

    snprintf(bucket->ip, sizeof(bucket->ip), "%s", ip);
    bucket->tokens = (double)(g_config.requests_per_minute + g_config.burst);
    bucket->last_refill = time(NULL);
    bucket->last_seen = time(NULL);

    /* Prepend to chain */
    bucket->next = g_table[idx];
    g_table[idx] = bucket;

    return bucket;
}

/* Refill tokens based on elapsed time */
static void refill_tokens(IpBucket *bucket, double max_tokens) {
    time_t now = time(NULL);
    double elapsed = difftime(now, bucket->last_refill);

    if (elapsed > 0) {
        double refill = (double)g_config.requests_per_minute * elapsed / 60.0;
        bucket->tokens += refill;
        if (bucket->tokens > max_tokens) {
            bucket->tokens = max_tokens;
        }
        bucket->last_refill = now;
    }
}

/* Remove stale entries (not seen in 5 minutes) */
static void cleanup_stale(void) {
    time_t now = time(NULL);
    if (difftime(now, g_last_cleanup) < g_config.cleanup_interval) {
        return;
    }
    g_last_cleanup = now;

    int removed = 0;
    for (int i = 0; i < RATE_LIMIT_TABLE_SIZE; i++) {
        IpBucket **pp = &g_table[i];
        while (*pp != NULL) {
            if (difftime(now, (*pp)->last_seen) > 300) {  /* 5 minutes */
                IpBucket *stale = *pp;
                *pp = stale->next;
                free(stale);
                removed++;
            } else {
                pp = &(*pp)->next;
            }
        }
    }

    if (removed > 0) {
        LOG_DEBUG("Rate limiter: cleaned up %d stale entries", removed);
    }
}

/* Core rate check: returns true if allowed */
static bool check_rate(const char *ip, double max_tokens) {
    if (!g_initialized || ip == NULL) {
        return true;  /* Not configured, allow */
    }

    pthread_mutex_lock(&g_lock);

    cleanup_stale();

    IpBucket *bucket = get_bucket(ip);
    if (bucket == NULL) {
        pthread_mutex_unlock(&g_lock);
        return true;  /* OOM, fail open */
    }

    refill_tokens(bucket, max_tokens);
    bucket->last_seen = time(NULL);

    bool allowed = bucket->tokens >= 1.0;
    if (allowed) {
        bucket->tokens -= 1.0;
    }

    pthread_mutex_unlock(&g_lock);

    return allowed;
}

int rate_limit_init(const RateLimitConfig *config) {
    if (config == NULL) {
        return -1;
    }

    g_config = *config;
    if (g_config.requests_per_minute <= 0) {
        g_config.requests_per_minute = 60;
    }
    if (g_config.burst <= 0) {
        g_config.burst = 20;
    }
    if (g_config.cleanup_interval <= 0) {
        g_config.cleanup_interval = 60;
    }

    memset(g_table, 0, sizeof(g_table));
    g_last_cleanup = time(NULL);
    g_initialized = true;

    LOG_INFO("Rate limiter initialized: %d req/min, burst %d",
             g_config.requests_per_minute, g_config.burst);

    return 0;
}

void rate_limit_shutdown(void) {
    if (!g_initialized) {
        return;
    }

    pthread_mutex_lock(&g_lock);

    for (int i = 0; i < RATE_LIMIT_TABLE_SIZE; i++) {
        IpBucket *bucket = g_table[i];
        while (bucket != NULL) {
            IpBucket *next = bucket->next;
            free(bucket);
            bucket = next;
        }
        g_table[i] = NULL;
    }

    g_initialized = false;
    pthread_mutex_unlock(&g_lock);

    LOG_INFO("Rate limiter shutdown");
}

bool rate_limit_middleware(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)user_data;

    double max_tokens = (double)(g_config.requests_per_minute + g_config.burst);

    if (!check_rate(req->client_ip, max_tokens)) {
        LOG_WARN("Rate limit exceeded for IP: %s on %s",
                 req->client_ip ? req->client_ip : "unknown", req->path);
        http_response_error(resp, HTTP_STATUS_TOO_MANY_REQUESTS,
                           "Too many requests. Please try again later.");
        http_response_add_header(resp, "Retry-After", "60");
        return false;
    }

    return true;
}

bool rate_limit_middleware_auth(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)user_data;

    /* Auth endpoints get a much stricter limit */
    double max_tokens = (double)(AUTH_RATE_LIMIT_RPM + 3);

    if (!check_rate(req->client_ip, max_tokens)) {
        LOG_WARN("Auth rate limit exceeded for IP: %s", req->client_ip ? req->client_ip : "unknown");
        http_response_error(resp, HTTP_STATUS_TOO_MANY_REQUESTS,
                           "Too many login attempts. Please try again later.");
        http_response_add_header(resp, "Retry-After", "300");
        return false;
    }

    return true;
}
