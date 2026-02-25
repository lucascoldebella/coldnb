import adminApi from "../adminApi";

export const adminAuth = {
  login: (username, password) =>
    adminApi.post("/api/admin/login", { username, password }),

  logout: () =>
    adminApi.post("/api/admin/logout"),

  me: () =>
    adminApi.get("/api/admin/me"),

  updateProfile: (data) =>
    adminApi.put("/api/admin/profile", data),

  changePassword: (currentPassword, newPassword) =>
    adminApi.put("/api/admin/password", { currentPassword, newPassword }),
};

export default adminAuth;
