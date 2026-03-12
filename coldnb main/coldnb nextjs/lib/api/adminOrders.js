import adminApi from "../adminApi";

export const adminOrders = {
  list: (params = {}) =>
    adminApi.get("/api/admin/orders", { params }),

  get: (id) =>
    adminApi.get(`/api/admin/orders/${id}`),

  updateStatus: (id, status) =>
    adminApi.put(`/api/admin/orders/${id}/status`, { status }),

  updateTracking: (id, data) =>
    adminApi.put(`/api/admin/orders/${id}/tracking`, data),

  updatePaymentStatus: (id, paymentStatus) =>
    adminApi.put(`/api/admin/orders/${id}/payment-status`, { paymentStatus }),

  addNote: (id, note) =>
    adminApi.post(`/api/admin/orders/${id}/notes`, { note }),

  cancel: (id, reason) =>
    adminApi.post(`/api/admin/orders/${id}/cancel`, { reason }),

  refund: (id, amount, reason) =>
    adminApi.post(`/api/admin/orders/${id}/refund`, { amount, reason }),

  exportCSV: (params = {}) =>
    adminApi.get("/api/admin/orders/export", { params, responseType: "blob" }),
};

export default adminOrders;
