import adminApi from "@/lib/adminApi";

export const adminDiscounts = {
  list: () => adminApi.get("/api/admin/discounts"),
  create: (data) => adminApi.post("/api/admin/discounts", data),
  update: (id, data) => adminApi.put(`/api/admin/discounts/${id}`, data),
  delete: (id) => adminApi.delete(`/api/admin/discounts/${id}`),
};
