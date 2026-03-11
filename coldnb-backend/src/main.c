/*
 * Coldnb E-commerce Backend Server
 * Main entry point
 */

#include "config/config_loader.h"
#include "db/db_connection.h"
#include "http/http_server.h"
#include "http/http_router.h"
#include "auth/auth_supabase.h"
#include "auth/auth_middleware.h"
#include "services/svc_admin_auth.h"
#include "services/svc_email.h"
#include "clients/client_stripe.h"
#include "clients/client_brevo.h"
#include "middleware/middleware_analytics.h"
#include "middleware/middleware_rate_limit.h"
#include "handlers/handler_products.h"
#include "handlers/handler_cart.h"
#include "handlers/handler_admin.h"
#include "handlers/handler_user.h"
#include "handlers/handler_addresses.h"
#include "handlers/handler_wishlist.h"
#include "handlers/handler_orders.h"
#include "handlers/handler_payments.h"
#include "handlers/handler_newsletter.h"
#include "handlers/handler_contact.h"
#include "handlers/handler_admin_orders.h"
#include "handlers/handler_admin_products.h"
#include "handlers/handler_admin_analytics.h"
#include "handlers/handler_admin_employees.h"
#include "handlers/handler_admin_homepage.h"
#include "handlers/handler_admin_navigation.h"
#include "handlers/handler_shipping.h"
#include "util/hash_util.h"
#include "log/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

/* Global server instance for signal handler */
static HttpServer *g_server = NULL;

/* Print usage information */
static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -c, --config PATH    Configuration file path\n");
    fprintf(stderr, "  -p, --port PORT      Override server port\n");
    fprintf(stderr, "  -h, --help           Show this help message\n");
    fprintf(stderr, "  -v, --version        Show version information\n");
}

/* Print version information */
static void print_version(void) {
    printf("coldnb-server version 1.0.0\n");
    printf("Coldnb E-commerce Backend\n");
}

/* Signal handler for graceful shutdown */
static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        LOG_INFO("Received signal %d, shutting down...", signum);
        if (g_server != NULL) {
            http_server_stop(g_server);
        }
    }
}

/* Setup signal handlers */
static int setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        LOG_ERROR("Failed to setup SIGINT handler");
        return -1;
    }

    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        LOG_ERROR("Failed to setup SIGTERM handler");
        return -1;
    }

    /* Ignore SIGPIPE to handle broken connections gracefully */
    signal(SIGPIPE, SIG_IGN);

    return 0;
}

/* Initialize logging from config */
static int init_logging(const Config *config) {
    LogConfig log_config = {
        .level = log_level_from_string(config_get_string(config, "log.level", "info")),
        .file_path = config_get_string(config, "log.file", NULL),
        .max_file_size = config_get_size(config, "log.max_size", 100 * 1024 * 1024),
        .rotate_count = config_get_int(config, "log.rotate_count", 5),
        .log_to_stdout = true,
        .include_timestamp = true,
        .include_level = true,
        .include_location = true
    };

    return log_init(&log_config);
}

/* Initialize database pool from config */
static DbPool *init_database(const Config *config) {
    char *db_password = config_load_secret(config, "database.password_file");

    DbPoolConfig db_config = {
        .host = config_get_string(config, "database.host", "localhost"),
        .port = config_get_int(config, "database.port", 5432),
        .dbname = config_get_string(config, "database.name", "coldnb"),
        .user = config_get_string(config, "database.user", "coldnb"),
        .password = db_password,
        .pool_size = config_get_int(config, "database.pool_size", 10),
        .connect_timeout = config_get_int(config, "database.timeout", 10)
    };

    DbPool *pool = db_pool_create(&db_config);

    /* Clear password from memory */
    if (db_password != NULL) {
        memset(db_password, 0, strlen(db_password));
        free(db_password);
    }

    return pool;
}

/* Initialize Supabase auth from config */
static int init_supabase(const Config *config) {
    char *jwt_secret = config_load_secret(config, "supabase.jwt_secret_file");
    char *anon_key = config_load_secret(config, "supabase.anon_key_file");
    char *service_role_key = config_load_secret(config, "supabase.service_role_key_file");

    if (jwt_secret == NULL) {
        LOG_ERROR("Failed to load Supabase JWT secret");
        free(anon_key);
        free(service_role_key);
        return -1;
    }

    SupabaseConfig supabase_config = {
        .project_url = config_get_string(config, "supabase.project_url", NULL),
        .jwt_secret = jwt_secret,
        .anon_key = anon_key,
        .service_role_key = service_role_key
    };

    int result = supabase_init(&supabase_config);

    /* Clear secrets from memory */
    if (jwt_secret != NULL) {
        memset(jwt_secret, 0, strlen(jwt_secret));
        free(jwt_secret);
    }
    if (anon_key != NULL) {
        memset(anon_key, 0, strlen(anon_key));
        free(anon_key);
    }
    if (service_role_key != NULL) {
        memset(service_role_key, 0, strlen(service_role_key));
        free(service_role_key);
    }

    return result;
}

/* Initialize admin auth from config */
static int init_admin_auth(const Config *config) {
    char *admin_jwt_secret = config_load_secret(config, "admin.jwt_secret_file");

    if (admin_jwt_secret == NULL) {
        LOG_WARN("Admin JWT secret not configured, admin auth disabled");
        return 0;
    }

    AdminAuthConfig admin_config = {
        .jwt_secret = admin_jwt_secret,
        .session_duration = config_get_int(config, "admin.session_duration", 86400)
    };

    int result = admin_auth_init(&admin_config);

    /* Clear secret from memory */
    memset(admin_jwt_secret, 0, strlen(admin_jwt_secret));
    free(admin_jwt_secret);

    return result;
}

/* Initialize Stripe payment integration from config */
static int init_stripe(const Config *config) {
    char *secret_key = config_load_secret(config, "stripe.secret_key_file");
    char *webhook_secret = config_load_secret(config, "stripe.webhook_secret_file");

    if (secret_key == NULL) {
        LOG_WARN("Stripe secret key not configured, payments disabled");
        free(webhook_secret);
        return 0;
    }

    StripeConfig stripe_config = {
        .secret_key = secret_key,
        .webhook_secret = webhook_secret,
        .pix_enabled = config_get_bool(config, "stripe.pix_enabled", false)
    };

    int result = stripe_init(&stripe_config);

    /* Clear secrets from memory */
    memset(secret_key, 0, strlen(secret_key));
    free(secret_key);
    if (webhook_secret != NULL) {
        memset(webhook_secret, 0, strlen(webhook_secret));
        free(webhook_secret);
    }

    return result;
}

/* Initialize Brevo email marketing from config */
static int init_brevo(const Config *config) {
    char *api_key = config_load_secret(config, "brevo.api_key_file");

    if (api_key == NULL) {
        LOG_WARN("Brevo API key not configured, email marketing disabled");
        return 0;
    }

    BrevoConfig brevo_config = {
        .api_key = api_key,
        .list_id = config_get_int(config, "brevo.list_id", 1),
        .sandbox_mode = config_get_bool(config, "brevo.sandbox_mode", false)
    };

    int result = brevo_init(&brevo_config);

    /* Clear secret from memory */
    memset(api_key, 0, strlen(api_key));
    free(api_key);

    return result;
}

static int init_email_service(const Config *config) {
    EmailServiceConfig email_config = {
        .store_name = config_get_string(config, "email.store_name", "Coldnb"),
        .site_url = config_get_string(config, "email.site_url", "https://coldnb.com"),
        .sender_email = config_get_string(config, "email.sender_email", NULL),
        .sender_name = config_get_string(config, "email.sender_name", NULL),
        .reply_to_email = config_get_string(config, "email.reply_to_email", NULL),
        .reply_to_name = config_get_string(config, "email.reply_to_name", NULL),
        .notification_email = config_get_string(config, "email.notification_email", NULL),
        .notification_name = config_get_string(config, "email.notification_name", NULL)
    };

    return email_service_init(&email_config);
}

/* Create and configure HTTP server from config */
static HttpServer *create_server(const Config *config, int port_override) {
    HttpServerConfig server_config;
    http_server_config_defaults(&server_config);

    server_config.port = port_override > 0 ? port_override :
                         config_get_int(config, "server.port", HTTP_SERVER_DEFAULT_PORT);
    server_config.thread_pool_size = config_get_int(config, "server.workers",
                                                    HTTP_SERVER_DEFAULT_THREADS);
    server_config.max_connections = config_get_size(config, "server.max_connections",
                                                    HTTP_SERVER_DEFAULT_MAX_CONNECTIONS);
    server_config.request_timeout = config_get_size(config, "server.request_timeout",
                                                    HTTP_SERVER_DEFAULT_TIMEOUT);
    server_config.bind_address = config_get_string(config, "server.bind_address", "127.0.0.1");
    server_config.trust_proxy = config_get_bool(config, "server.trust_proxy", true);

    server_config.cors_origins = config_get_string(config, "cors.origins", "*");
    server_config.cors_methods = config_get_string(config, "cors.methods",
                                                   "GET,POST,PUT,DELETE,OPTIONS");
    server_config.cors_headers = config_get_string(config, "cors.headers",
                                                   "Content-Type,Authorization,X-Session-ID");

    return http_server_create(&server_config);
}

/* Register all routes */
static void register_routes(HttpRouter *router, DbPool *pool) {
    /* Apply rate limiting FIRST — blocks abusive IPs before any processing */
    http_router_use(router, rate_limit_middleware, NULL);

    /* Stricter rate limit on auth endpoints */
    http_router_use_path(router, "/api/admin/login", rate_limit_middleware_auth, NULL);

    /* Apply analytics middleware globally (tracks page views) */
    http_router_use(router, analytics_middleware_page_view, NULL);

    /* Public routes (no auth required) */
    handler_products_register(router, pool);
    handler_newsletter_register(router, pool);
    handler_contact_register(router, pool);

    /* Apply auth middleware to protected paths */
    http_router_use_path(router, "/api/cart", auth_middleware_required, NULL);
    http_router_use_path(router, "/api/user", auth_middleware_required, NULL);
    http_router_use_path(router, "/api/addresses", auth_middleware_required, NULL);
    http_router_use_path(router, "/api/wishlist", auth_middleware_required, NULL);
    http_router_use_path(router, "/api/orders", auth_middleware_required, NULL);
    http_router_use_path(router, "/api/payments", auth_middleware_required, NULL);

    /* Apply admin middleware to admin paths (except login) */
    http_router_use_path(router, "/api/admin/me", auth_middleware_admin, NULL);
    http_router_use_path(router, "/api/admin/logout", auth_middleware_admin, NULL);
    http_router_use_path(router, "/api/admin/orders", auth_middleware_admin, NULL);
    http_router_use_path(router, "/api/admin/users", auth_middleware_admin, NULL);
    http_router_use_path(router, "/api/admin/products", auth_middleware_admin, NULL);
    http_router_use_path(router, "/api/admin/analytics", auth_middleware_admin, NULL);
    http_router_use_path(router, "/api/admin/homepage", auth_middleware_admin, NULL);
    http_router_use_path(router, "/api/admin/navigation", auth_middleware_admin, NULL);
    http_router_use_path(router, "/api/admin/shipping", auth_middleware_admin, NULL);
    http_router_use_path(router, "/api/admin/categories", auth_middleware_admin, NULL);

    /* Protected routes (auth required) */
    handler_cart_register(router, pool);
    handler_user_register(router, pool);
    handler_addresses_register(router, pool);
    handler_wishlist_register(router, pool);
    handler_orders_register(router, pool);
    handler_payments_register(router, pool);

    /* Admin routes */
    handler_admin_register(router, pool);
    handler_admin_orders_register(router, pool);
    handler_admin_products_register(router, pool);
    handler_admin_analytics_register(router, pool);
    handler_admin_employees_register(router, pool);
    handler_admin_homepage_register(router, pool);
    handler_admin_navigation_register(router, pool);
    handler_shipping_register(router, pool);

    LOG_INFO("Routes registered");
}

/* Health check handler */
static void handler_health(HttpRequest *req, HttpResponse *resp, void *user_data) {
    (void)req;
    (void)user_data;
    http_response_json(resp, HTTP_STATUS_OK, "{\"status\":\"ok\"}");
}

int main(int argc, char *argv[]) {
    const char *config_path = NULL;
    int port_override = 0;

    /* Parse command line arguments */
    static struct option long_options[] = {
        {"config",  required_argument, 0, 'c'},
        {"port",    required_argument, 0, 'p'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "c:p:hv", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                config_path = optarg;
                break;
            case 'p':
                port_override = atoi(optarg);
                if (port_override <= 0 || port_override > 65535) {
                    fprintf(stderr, "Invalid port number: %s\n", optarg);
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                print_version();
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* Default config path */
    if (config_path == NULL) {
        config_path = "config/server.conf";
    }

    /* Initialize hashing library first */
    if (hash_init() != 0) {
        fprintf(stderr, "Failed to initialize crypto library\n");
        return 1;
    }

    /* Load configuration */
    Config *config = config_load(config_path);
    if (config == NULL) {
        fprintf(stderr, "Failed to load configuration from: %s\n", config_path);
        return 1;
    }

    /* Initialize logging */
    if (init_logging(config) != 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        config_free(config);
        return 1;
    }

    LOG_INFO("Starting coldnb-server...");
    LOG_INFO("Configuration loaded from: %s", config_path);

    /* Initialize database pool */
    DbPool *db_pool = init_database(config);
    if (db_pool == NULL) {
        LOG_FATAL("Failed to create database pool");
        config_free(config);
        log_shutdown();
        return 1;
    }

    /* Initialize Supabase auth */
    if (init_supabase(config) != 0) {
        LOG_FATAL("Failed to initialize Supabase auth");
        db_pool_free(db_pool);
        config_free(config);
        log_shutdown();
        return 1;
    }

    /* Initialize admin auth */
    if (init_admin_auth(config) != 0) {
        LOG_FATAL("Failed to initialize admin auth");
        supabase_shutdown();
        db_pool_free(db_pool);
        config_free(config);
        log_shutdown();
        return 1;
    }

    /* Initialize Stripe payments (optional) */
    if (init_stripe(config) != 0) {
        LOG_WARN("Stripe initialization failed, payments will be unavailable");
    }

    /* Initialize Brevo email marketing (optional) */
    if (init_brevo(config) != 0) {
        LOG_WARN("Brevo initialization failed, email marketing will be unavailable");
    }

    /* Initialize transactional email service (optional) */
    if (init_email_service(config) != 0) {
        LOG_WARN("Email service initialization failed, transactional emails will be unavailable");
    }

    /* Set database pool for admin middleware */
    auth_middleware_set_db_pool(db_pool);

    /* Initialize analytics middleware */
    analytics_middleware_init(db_pool);

    /* Initialize rate limiter */
    if (config_get_bool(config, "rate_limit.enabled", true)) {
        RateLimitConfig rl_config = {
            .requests_per_minute = config_get_int(config, "rate_limit.requests_per_minute", 60),
            .burst = config_get_int(config, "rate_limit.burst", 20),
            .cleanup_interval = 60
        };
        if (rate_limit_init(&rl_config) != 0) {
            LOG_WARN("Failed to initialize rate limiter");
        }
    }

    /* Create HTTP server */
    HttpServer *server = create_server(config, port_override);
    if (server == NULL) {
        LOG_FATAL("Failed to create HTTP server");
        email_service_shutdown();
        brevo_shutdown();
        stripe_shutdown();
        admin_auth_shutdown();
        supabase_shutdown();
        db_pool_free(db_pool);
        config_free(config);
        log_shutdown();
        return 1;
    }

    /* Store for signal handler */
    g_server = server;

    /* Setup signal handlers */
    if (setup_signals() != 0) {
        LOG_FATAL("Failed to setup signal handlers");
        http_server_free(server);
        email_service_shutdown();
        brevo_shutdown();
        stripe_shutdown();
        admin_auth_shutdown();
        supabase_shutdown();
        db_pool_free(db_pool);
        config_free(config);
        log_shutdown();
        return 1;
    }

    /* Get router and register routes */
    HttpRouter *router = http_server_get_router(server);

    /* Register health check (public, no analytics) */
    ROUTE_GET(router, "/health", handler_health, NULL);
    ROUTE_GET(router, "/api/health", handler_health, NULL);

    /* Register all application routes */
    register_routes(router, db_pool);

    /* Print registered routes in debug mode */
    #ifdef DEBUG
    http_router_print_routes(router);
    #endif

    LOG_INFO("Server listening on port %d", http_server_get_port(server));

    /* Start server (blocks until shutdown) */
    int result = http_server_start(server);

    /* Cleanup */
    LOG_INFO("Shutting down...");

    g_server = NULL;
    http_server_free(server);
    rate_limit_shutdown();
    email_service_shutdown();
    brevo_shutdown();
    stripe_shutdown();
    admin_auth_shutdown();
    supabase_shutdown();
    db_pool_free(db_pool);
    config_free(config);

    LOG_INFO("Server stopped");
    log_shutdown();

    return result == 0 ? 0 : 1;
}
