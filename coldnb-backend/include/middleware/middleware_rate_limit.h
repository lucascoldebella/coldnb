#ifndef COLDNB_MIDDLEWARE_RATE_LIMIT_H
#define COLDNB_MIDDLEWARE_RATE_LIMIT_H

#include "http/http_request.h"
#include "http/http_response.h"

#include <stdbool.h>

/* Rate limiter configuration */
typedef struct {
    int requests_per_minute;    /* Max requests per minute per IP */
    int burst;                  /* Burst allowance above rate */
    int cleanup_interval;       /* Seconds between table cleanup */
} RateLimitConfig;

/* Initialize rate limiter. Returns 0 on success. */
int rate_limit_init(const RateLimitConfig *config);

/* Shutdown and free all rate limiter resources */
void rate_limit_shutdown(void);

/* Rate limiting middleware — returns false (blocks chain) if rate exceeded */
bool rate_limit_middleware(HttpRequest *req, HttpResponse *resp, void *user_data);

/* Stricter rate limit for auth endpoints */
bool rate_limit_middleware_auth(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_MIDDLEWARE_RATE_LIMIT_H */
