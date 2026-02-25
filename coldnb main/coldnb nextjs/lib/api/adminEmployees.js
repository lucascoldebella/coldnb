import adminApi from "../adminApi";

export const adminEmployees = {
  // List all employees
  list: (params = {}) =>
    adminApi.get("/api/admin/employees", { params }),

  // Get employee details
  get: (id) =>
    adminApi.get(`/api/admin/employees/${id}`),

  // Create new employee
  create: (data) =>
    adminApi.post("/api/admin/employees", data),

  // Update employee
  update: (id, data) =>
    adminApi.put(`/api/admin/employees/${id}`, data),

  // Delete employee
  delete: (id) =>
    adminApi.delete(`/api/admin/employees/${id}`),

  // Update employee permissions
  updatePermissions: (id, permissions) =>
    adminApi.put(`/api/admin/employees/${id}/permissions`, { permissions }),

  // Upload employee photo
  uploadPhoto: (id, formData) =>
    adminApi.post(`/api/admin/employees/${id}/photo`, formData, {
      headers: { "Content-Type": "multipart/form-data" },
    }),

  // Set employee active status
  setStatus: (id, isActive) =>
    adminApi.put(`/api/admin/employees/${id}/status`, { isActive }),
};

export default adminEmployees;
