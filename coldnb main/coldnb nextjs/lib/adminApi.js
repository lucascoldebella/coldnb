import axios from "axios";
import { getApiBaseUrl } from "./apiBase";

// Create axios instance for admin API
const adminApi = axios.create({
  baseURL: getApiBaseUrl(),
  headers: {
    "Content-Type": "application/json",
  },
  timeout: 30000,
});

// Event emitter for auth events (used to notify AdminContext of 401s)
// This avoids creating multiple interceptors and potential race conditions
const authEventListeners = new Set();

export const onAuthError = (callback) => {
  authEventListeners.add(callback);
  return () => authEventListeners.delete(callback);
};

const emitAuthError = (type) => {
  authEventListeners.forEach((callback) => callback(type));
};

// Flag to prevent multiple simultaneous auth error emissions
let isEmittingAuthError = false;

// Request interceptor to add JWT token
adminApi.interceptors.request.use(
  (config) => {
    if (typeof window !== "undefined") {
      const token = localStorage.getItem("adminToken");
      if (token) {
        config.headers.Authorization = `Bearer ${token}`;
      }
    }
    return config;
  },
  (error) => {
    return Promise.reject(error);
  }
);

// Response interceptor for error handling
adminApi.interceptors.response.use(
  (response) => response,
  (error) => {
    // Handle common errors
    if (error.response) {
      switch (error.response.status) {
        case 401:
          // Emit auth error event (AdminContext will handle logout)
          // Only emit if not already emitting to prevent loops
          if (!isEmittingAuthError) {
            isEmittingAuthError = true;
            // Use setTimeout to break out of any potential synchronous loops
            setTimeout(() => {
              emitAuthError("session_expired");
              isEmittingAuthError = false;
            }, 0);
          }
          break;
        case 403:
        case 404:
        case 500:
          break;
      }
    }
    return Promise.reject(error);
  }
);

export default adminApi;
