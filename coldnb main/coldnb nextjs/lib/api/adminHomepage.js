import adminApi from "../adminApi";

export const adminHeroSlides = {
  list: () =>
    adminApi.get("/api/admin/homepage/hero-slides"),

  create: (data) =>
    adminApi.post("/api/admin/homepage/hero-slides", data),

  update: (id, data) =>
    adminApi.put(`/api/admin/homepage/hero-slides/${id}`, data),

  delete: (id) =>
    adminApi.delete(`/api/admin/homepage/hero-slides/${id}`),

  reorder: (order) =>
    adminApi.put("/api/admin/homepage/hero-slides/reorder", { order }),
};

export const adminBanners = {
  list: () =>
    adminApi.get("/api/admin/homepage/banners"),

  create: (data) =>
    adminApi.post("/api/admin/homepage/banners", data),

  update: (id, data) =>
    adminApi.put(`/api/admin/homepage/banners/${id}`, data),

  delete: (id) =>
    adminApi.delete(`/api/admin/homepage/banners/${id}`),
};

export const adminSections = {
  list: () =>
    adminApi.get("/api/admin/homepage/sections"),

  update: (id, data) =>
    adminApi.put(`/api/admin/homepage/sections/${id}`, data),

  updateProducts: (id, products) =>
    adminApi.post(`/api/admin/homepage/sections/${id}/products`, { products }),
};

export const adminCampaigns = {
  list: () =>
    adminApi.get("/api/admin/homepage/campaigns"),

  create: (data) =>
    adminApi.post("/api/admin/homepage/campaigns", data),

  update: (id, data) =>
    adminApi.put(`/api/admin/homepage/campaigns/${id}`, data),

  delete: (id) =>
    adminApi.delete(`/api/admin/homepage/campaigns/${id}`),
};

export default { heroSlides: adminHeroSlides, banners: adminBanners, sections: adminSections, campaigns: adminCampaigns };
