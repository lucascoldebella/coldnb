import adminApi from "../adminApi";

export const adminNavMenus = {
  list: () =>
    adminApi.get("/api/admin/navigation/menus"),

  create: (data) =>
    adminApi.post("/api/admin/navigation/menus", data),

  update: (id, data) =>
    adminApi.put(`/api/admin/navigation/menus/${id}`, data),

  delete: (id) =>
    adminApi.delete(`/api/admin/navigation/menus/${id}`),
};

export const adminNavGroups = {
  list: (menuId) =>
    adminApi.get(`/api/admin/navigation/menus/${menuId}/groups`),

  create: (menuId, data) =>
    adminApi.post(`/api/admin/navigation/menus/${menuId}/groups`, data),

  update: (id, data) =>
    adminApi.put(`/api/admin/navigation/groups/${id}`, data),

  delete: (id) =>
    adminApi.delete(`/api/admin/navigation/groups/${id}`),
};

export const adminNavItems = {
  create: (groupId, data) =>
    adminApi.post(`/api/admin/navigation/groups/${groupId}/items`, data),

  update: (id, data) =>
    adminApi.put(`/api/admin/navigation/items/${id}`, data),

  delete: (id) =>
    adminApi.delete(`/api/admin/navigation/items/${id}`),

  reorder: (order) =>
    adminApi.put("/api/admin/navigation/items/reorder", { order }),
};

export default { menus: adminNavMenus, groups: adminNavGroups, items: adminNavItems };
