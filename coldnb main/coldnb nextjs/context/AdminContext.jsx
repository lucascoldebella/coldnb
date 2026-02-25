"use client";
import React, { createContext, useContext, useState, useEffect, useCallback, useRef } from "react";
import { useRouter, usePathname } from "next/navigation";
import adminApi, { onAuthError } from "@/lib/adminApi";

const AdminContext = createContext();

export const useAdmin = () => {
  const context = useContext(AdminContext);
  if (!context) {
    throw new Error("useAdmin must be used within AdminProvider");
  }
  return context;
};

// Default dashboard card configuration
const defaultDashboardCards = [
  { id: "revenue", visible: true, order: 0 },
  { id: "orders", visible: true, order: 1 },
  { id: "customers", visible: true, order: 2 },
  { id: "lowStock", visible: true, order: 3 },
  { id: "revenueChart", visible: true, order: 4 },
  { id: "orderStatusChart", visible: true, order: 5 },
  { id: "recentOrders", visible: true, order: 6 },
  { id: "lowStockProducts", visible: true, order: 7 },
];

// Permission categories
export const PERMISSIONS = {
  DASHBOARD: ["view_dashboard", "customize_dashboard"],
  FINANCIAL: ["view_financial", "view_revenue", "export_financial"],
  PRODUCTS: ["view_products", "create_products", "edit_products", "delete_products", "manage_categories", "manage_inventory"],
  ORDERS: ["view_orders", "view_order_details", "update_order_status", "cancel_orders"],
  CUSTOMERS: ["view_customers", "view_customer_details", "edit_customers"],
  MARKETING: ["view_marketing", "view_analytics", "manage_discounts"],
  TEAM: ["manage_team", "create_employees", "edit_employees", "assign_permissions"],
};

export function AdminProvider({ children }) {
  const [admin, setAdmin] = useState(null);
  const [token, setToken] = useState(null);
  const [permissions, setPermissions] = useState({});
  const [isLoading, setIsLoading] = useState(true);
  const [sidebarCollapsed, setSidebarCollapsed] = useState(false);
  const [dashboardCards, setDashboardCards] = useState(defaultDashboardCards);

  // Ref to prevent multiple simultaneous logout calls
  const isLoggingOut = useRef(false);
  // Ref for debouncing localStorage writes
  const dashboardCardsDebounceRef = useRef(null);

  const router = useRouter();
  const pathname = usePathname();

  // Check if user has a specific permission
  const hasPermission = useCallback((permission) => {
    if (!admin) return false;
    // super_admin bypasses all permission checks
    if (admin.role === "super_admin") return true;
    return permissions[permission] === true;
  }, [admin, permissions]);

  // Check if user can access a section
  const canAccess = useCallback((section) => {
    if (!admin) return false;
    if (admin.role === "super_admin") return true;

    const sectionPermissions = {
      dashboard: ["view_dashboard"],
      financial: ["view_financial"],
      products: ["view_products"],
      orders: ["view_orders"],
      customers: ["view_customers"],
      marketing: ["view_marketing"],
      team: ["manage_team"],
      homepage: ["manage_homepage"],
      shipping: ["view_products"],
    };

    const required = sectionPermissions[section] || [];
    return required.some(p => permissions[p] === true);
  }, [admin, permissions]);

  // Login function
  const login = async (username, password) => {
    try {
      const response = await adminApi.post("/api/admin/login", { username, password });
      const responseData = response.data.data || response.data;
      const { token: newToken, user: adminData } = responseData;

      localStorage.setItem("adminToken", newToken);
      localStorage.setItem("adminUser", JSON.stringify(adminData));
      setToken(newToken);
      setAdmin(adminData);
      setPermissions(adminData.permissions || {});

      return { success: true };
    } catch (error) {
      console.error("Login error:", error);
      return {
        success: false,
        error: error.response?.data?.error || "Login failed"
      };
    }
  };

  // Logout function - protected against multiple simultaneous calls
  const logout = useCallback(async () => {
    // Prevent multiple logout calls
    if (isLoggingOut.current) {
      return;
    }
    isLoggingOut.current = true;

    try {
      // Only call logout API if we have a token
      const storedToken = localStorage.getItem("adminToken");
      if (storedToken) {
        await adminApi.post("/api/admin/logout").catch(() => {
          // Ignore errors from logout API - we're logging out anyway
        });
      }
    } finally {
      localStorage.removeItem("adminToken");
      localStorage.removeItem("adminUser");
      setToken(null);
      setAdmin(null);
      setPermissions({});

      // Only redirect if not already on login page
      if (pathname !== "/admin/login") {
        router.push("/admin/login");
      }

      // Reset the flag after a short delay to allow for cleanup
      setTimeout(() => {
        isLoggingOut.current = false;
      }, 1000);
    }
  }, [router, pathname]);

  // Validate token and get admin info
  const validateSession = useCallback(async () => {
    const storedToken = localStorage.getItem("adminToken");

    if (!storedToken) {
      setIsLoading(false);
      return false;
    }

    // Immediately restore admin data from localStorage (prevents login flash on refresh)
    const storedUser = localStorage.getItem("adminUser");
    if (storedUser) {
      try {
        const cachedAdmin = JSON.parse(storedUser);
        setToken(storedToken);
        setAdmin(cachedAdmin);
        setPermissions(cachedAdmin.permissions || {});
      } catch (_e) {
        // Invalid stored data, will validate via API
      }
    }

    try {
      setToken(storedToken);
      const response = await adminApi.get("/api/admin/me");
      const adminData = response.data.data || response.data.admin || response.data;

      // Merge permissions from cached user data (backend /me endpoint may not return them)
      const cachedPerms = storedUser ? (JSON.parse(storedUser).permissions || {}) : {};
      const mergedAdmin = { ...adminData, permissions: adminData.permissions || cachedPerms };

      setAdmin(mergedAdmin);
      setPermissions(mergedAdmin.permissions || {});
      localStorage.setItem("adminUser", JSON.stringify(mergedAdmin));
      setIsLoading(false);
      return true;
    } catch (error) {
      // If API fails but we have cached data and token, keep the session alive
      if (storedUser && error.response?.status !== 401) {
        setIsLoading(false);
        return true;
      }
      console.error("Session validation error:", error);
      localStorage.removeItem("adminToken");
      localStorage.removeItem("adminUser");
      setToken(null);
      setAdmin(null);
      setPermissions({});
      setIsLoading(false);
      return false;
    }
  }, []);

  // Load dashboard cards from localStorage
  useEffect(() => {
    const savedCards = localStorage.getItem("adminDashboardCards");
    if (savedCards) {
      try {
        setDashboardCards(JSON.parse(savedCards));
      } catch (e) {
        console.error("Error loading dashboard cards:", e);
      }
    }
  }, []);

  // Save dashboard cards to localStorage (debounced to prevent excessive writes)
  const updateDashboardCards = useCallback((cards) => {
    setDashboardCards(cards);

    // Clear any pending localStorage write
    if (dashboardCardsDebounceRef.current) {
      clearTimeout(dashboardCardsDebounceRef.current);
    }

    // Debounce localStorage write by 300ms
    dashboardCardsDebounceRef.current = setTimeout(() => {
      localStorage.setItem("adminDashboardCards", JSON.stringify(cards));
    }, 300);
  }, []);

  // Toggle sidebar
  const toggleSidebar = useCallback(() => {
    setSidebarCollapsed(prev => !prev);
  }, []);

  // Validate session on mount only (empty dependency array)
  useEffect(() => {
    validateSession();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // Listen for auth errors from adminApi (401 responses)
  // This uses the event-based system to avoid recreating interceptors
  useEffect(() => {
    const unsubscribe = onAuthError((type) => {
      if (type === "session_expired" && pathname !== "/admin/login") {
        logout();
      }
    });

    return unsubscribe;
  }, [logout, pathname]);

  // Cleanup debounce timeout on unmount
  useEffect(() => {
    return () => {
      if (dashboardCardsDebounceRef.current) {
        clearTimeout(dashboardCardsDebounceRef.current);
      }
    };
  }, []);

  const value = {
    admin,
    token,
    permissions,
    isLoading,
    sidebarCollapsed,
    dashboardCards,
    login,
    logout,
    hasPermission,
    canAccess,
    toggleSidebar,
    updateDashboardCards,
    validateSession,
  };

  return (
    <AdminContext.Provider value={value}>
      {children}
    </AdminContext.Provider>
  );
}

export default AdminContext;
