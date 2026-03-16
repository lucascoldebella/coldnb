import userApi from "@/lib/userApi";
import adminApi from "@/lib/adminApi";

export const loyaltyApi = {
  getBalance: () => userApi.get("/api/loyalty/balance"),
  getHistory: () => userApi.get("/api/loyalty/history"),
  getRewards: () => userApi.get("/api/loyalty/rewards"),
  redeem: (reward_id) => userApi.post("/api/loyalty/redeem", { reward_id }),
};

export const adminLoyaltyApi = {
  listRewards: () => adminApi.get("/api/admin/loyalty/rewards"),
  createReward: (data) => adminApi.post("/api/admin/loyalty/rewards", data),
  updateReward: (id, data) => adminApi.put(`/api/admin/loyalty/rewards/${id}`, data),
  deleteReward: (id) => adminApi.delete(`/api/admin/loyalty/rewards/${id}`),
  grantPoints: (data) => adminApi.post("/api/admin/loyalty/grant", data),
  listAbandonedCarts: () => adminApi.get("/api/admin/abandoned-carts"),
  sendAbandonedCartEmails: () => adminApi.post("/api/admin/abandoned-carts/send"),
};
