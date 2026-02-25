import axios from "axios";
import supabase from "./supabase";

const userApi = axios.create({
  baseURL: process.env.NEXT_PUBLIC_API_URL || "http://localhost:8080",
  headers: {
    "Content-Type": "application/json",
  },
  timeout: 30000,
});

// Request interceptor to attach Supabase JWT
userApi.interceptors.request.use(
  async (config) => {
    if (supabase) {
      const { data } = await supabase.auth.getSession();
      const token = data?.session?.access_token;
      if (token) {
        config.headers.Authorization = `Bearer ${token}`;
      }
    }
    return config;
  },
  (error) => Promise.reject(error)
);

// Response interceptor for error handling
userApi.interceptors.response.use(
  (response) => response,
  (error) => {
    if (error.response?.status === 401 && supabase) {
      supabase.auth.signOut();
    }
    return Promise.reject(error);
  }
);

// Profile API
export const profileApi = {
  get: () => userApi.get("/api/user/profile"),
  update: (data) => userApi.put("/api/user/profile", data),
};

// Addresses API
export const addressesApi = {
  list: () => userApi.get("/api/addresses"),
  create: (data) => userApi.post("/api/addresses", data),
  update: (id, data) => userApi.put(`/api/addresses/${id}`, data),
  delete: (id) => userApi.delete(`/api/addresses/${id}`),
  setDefault: (id) => userApi.put(`/api/addresses/${id}/default`),
};

// Orders API
export const ordersApi = {
  list: (params = {}) => userApi.get("/api/orders", { params }),
  get: (id) => userApi.get(`/api/orders/${id}`),
};

export default userApi;
