import adminApi from "../adminApi";

export const adminUsers = {
  // List all customers
  list: (params = {}) =>
    adminApi.get("/api/admin/customers", { params }),

  // Get customer details
  get: (id) =>
    adminApi.get(`/api/admin/customers/${id}`),

  // Get customer orders
  getOrders: (id, params = {}) =>
    adminApi.get(`/api/admin/customers/${id}/orders`, { params }),

  // Update customer info
  update: (id, data) =>
    adminApi.put(`/api/admin/customers/${id}`, data),

  // Block/unblock customer
  setStatus: (id, isActive) =>
    adminApi.put(`/api/admin/customers/${id}/status`, { isActive }),

  // Export customers
  exportCSV: (params = {}) =>
    adminApi.get("/api/admin/customers/export", { params, responseType: "blob" }),
};

export default adminUsers;
