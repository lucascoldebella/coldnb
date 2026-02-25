import adminApi from "../adminApi";

export const adminAnalytics = {
  // Main dashboard metrics
  dashboard: () =>
    adminApi.get("/api/admin/analytics/dashboard"),

  // Sales analytics
  sales: (params = {}) =>
    adminApi.get("/api/admin/analytics/sales", { params }),

  // Revenue over time
  revenue: (params = {}) =>
    adminApi.get("/api/admin/analytics/revenue", { params }),

  // Revenue by category
  revenueByCategory: (params = {}) =>
    adminApi.get("/api/admin/analytics/revenue-by-category", { params }),

  // Product performance
  products: (params = {}) =>
    adminApi.get("/api/admin/analytics/products", { params }),

  // Top selling products
  topProducts: (params = {}) =>
    adminApi.get("/api/admin/analytics/top-products", { params }),

  // Traffic/views analytics
  traffic: (params = {}) =>
    adminApi.get("/api/admin/analytics/traffic", { params }),

  // Order status distribution
  orderStatus: () =>
    adminApi.get("/api/admin/analytics/order-status"),

  // Payment methods distribution
  paymentMethods: (params = {}) =>
    adminApi.get("/api/admin/analytics/payment-methods", { params }),

  // Low stock products
  lowStock: (params = {}) =>
    adminApi.get("/api/admin/analytics/low-stock", { params }),

  // Recent orders
  recentOrders: (limit = 5) =>
    adminApi.get("/api/admin/analytics/recent-orders", { params: { limit } }),

  // Customer acquisition
  customerAcquisition: (params = {}) =>
    adminApi.get("/api/admin/analytics/customer-acquisition", { params }),

  // Conversion rates
  conversionRates: (params = {}) =>
    adminApi.get("/api/admin/analytics/conversion-rates", { params }),
};

export default adminAnalytics;
