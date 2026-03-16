#ifndef COLDNB_HANDLER_LOYALTY_H
#define COLDNB_HANDLER_LOYALTY_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register loyalty routes (customer + admin) */
void handler_loyalty_register(HttpRouter *router, DbPool *pool);

/* GET /api/loyalty/balance - Get user's loyalty points balance */
void handler_loyalty_balance(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/loyalty/history - Get user's points transaction history */
void handler_loyalty_history(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/loyalty/rewards - List available rewards */
void handler_loyalty_rewards_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/loyalty/redeem - Redeem points for a reward */
void handler_loyalty_redeem(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/loyalty/rewards - Admin list rewards */
void handler_admin_loyalty_rewards_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/admin/loyalty/rewards - Admin create reward */
void handler_admin_loyalty_rewards_create(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/loyalty/rewards/:id - Admin update reward */
void handler_admin_loyalty_rewards_update(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/admin/loyalty/rewards/:id - Admin delete reward */
void handler_admin_loyalty_rewards_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/admin/loyalty/grant - Admin manually grant points to a user */
void handler_admin_loyalty_grant(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_LOYALTY_H */
