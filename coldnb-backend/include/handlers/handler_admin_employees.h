#ifndef COLDNB_HANDLER_ADMIN_EMPLOYEES_H
#define COLDNB_HANDLER_ADMIN_EMPLOYEES_H

#include "http/http_request.h"
#include "http/http_response.h"
#include "http/http_router.h"
#include "db/db_connection.h"

/* Register employee management routes */
void handler_admin_employees_register(HttpRouter *router, DbPool *pool);

/* GET /api/admin/employees - List all employees */
void handler_admin_employees_list(HttpRequest *req, HttpResponse *resp, void *user_data);

/* GET /api/admin/employees/:id - Get employee details */
void handler_admin_employees_get(HttpRequest *req, HttpResponse *resp, void *user_data);

/* POST /api/admin/employees - Create new employee */
void handler_admin_employees_create(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/employees/:id - Update employee */
void handler_admin_employees_update(HttpRequest *req, HttpResponse *resp, void *user_data);

/* DELETE /api/admin/employees/:id - Delete employee */
void handler_admin_employees_delete(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/employees/:id/permissions - Update employee permissions */
void handler_admin_employees_update_permissions(HttpRequest *req, HttpResponse *resp, void *user_data);

/* PUT /api/admin/employees/:id/status - Update employee active status */
void handler_admin_employees_update_status(HttpRequest *req, HttpResponse *resp, void *user_data);

#endif /* COLDNB_HANDLER_ADMIN_EMPLOYEES_H */
