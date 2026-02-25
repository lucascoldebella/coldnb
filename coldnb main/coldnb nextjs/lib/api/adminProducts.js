import adminApi from "../adminApi";

export const adminProducts = {
  list: (params = {}) =>
    adminApi.get("/api/admin/products", { params }),

  get: (id) =>
    adminApi.get(`/api/admin/products/${id}`),

  create: (data) =>
    adminApi.post("/api/admin/products", data),

  update: (id, data) =>
    adminApi.put(`/api/admin/products/${id}`, data),

  delete: (id) =>
    adminApi.delete(`/api/admin/products/${id}`),

  addImage: (productId, imageData) =>
    adminApi.post(`/api/admin/products/${productId}/images`, imageData),

  deleteImage: (productId, imageId) =>
    adminApi.delete(`/api/admin/products/${productId}/images/${imageId}`),

  updateImage: (productId, imageId, data) =>
    adminApi.put(`/api/admin/products/${productId}/images/${imageId}`, data),

  updateStock: (id, quantity) =>
    adminApi.put(`/api/admin/products/${id}/stock`, { quantity }),

  bulkDelete: (ids) =>
    adminApi.post("/api/admin/products/bulk-delete", { ids }),

  exportCSV: (params = {}) =>
    adminApi.get("/api/admin/products/export", { params, responseType: "blob" }),
};

export const adminCategories = {
  list: () =>
    adminApi.get("/api/admin/categories"),

  get: (id) =>
    adminApi.get(`/api/admin/categories/${id}`),

  create: (data) =>
    adminApi.post("/api/admin/categories", data),

  update: (id, data) =>
    adminApi.put(`/api/admin/categories/${id}`, data),

  delete: (id) =>
    adminApi.delete(`/api/admin/categories/${id}`),
};

export default adminProducts;
