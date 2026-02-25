import adminApi from "../adminApi";

export const adminShipping = {
  listZones: () =>
    adminApi.get("/api/admin/shipping/zones"),

  createZone: (data) =>
    adminApi.post("/api/admin/shipping/zones", data),

  updateZone: (id, data) =>
    adminApi.put(`/api/admin/shipping/zones/${id}`, data),

  deleteZone: (id) =>
    adminApi.delete(`/api/admin/shipping/zones/${id}`),
};
