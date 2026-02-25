"use client";
import { useEffect, useState } from "react";
import { useRouter, usePathname } from "next/navigation";
import { useAdmin } from "@/context/AdminContext";
import AdminSidebar from "./AdminSidebar";
import AdminHeader from "./AdminHeader";
import AdminErrorBoundary from "../common/AdminErrorBoundary";

// Loading spinner component
const LoadingScreen = () => (
  <div
    style={{
      display: "flex",
      alignItems: "center",
      justifyContent: "center",
      minHeight: "100vh",
      backgroundColor: "var(--admin-bg)",
    }}
  >
    <div
      style={{
        width: 40,
        height: 40,
        border: "3px solid var(--admin-border)",
        borderTopColor: "var(--admin-primary)",
        borderRadius: "50%",
        animation: "spin 0.8s linear infinite",
      }}
    />
    <style jsx>{`
      @keyframes spin {
        to {
          transform: rotate(360deg);
        }
      }
    `}</style>
  </div>
);

export default function AdminLayout({ children }) {
  const router = useRouter();
  const pathname = usePathname();
  const { admin, isLoading, sidebarCollapsed } = useAdmin();
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);

  // Redirect to login if not authenticated
  useEffect(() => {
    if (!isLoading && !admin && pathname !== "/admin/login") {
      router.push("/admin/login");
    }
  }, [admin, isLoading, pathname, router]);

  // Close mobile menu on route change
  useEffect(() => {
    setMobileMenuOpen(false);
  }, [pathname]);

  // Show loading screen while checking auth
  if (isLoading) {
    return <LoadingScreen />;
  }

  // For login page, don't show the admin layout
  if (pathname === "/admin/login") {
    return children;
  }

  // If not authenticated and not on login page, show loading (will redirect)
  if (!admin) {
    return <LoadingScreen />;
  }

  return (
    <div className="admin-wrapper">
      {/* Mobile overlay */}
      <div
        className={`admin-overlay ${mobileMenuOpen ? "visible" : ""}`}
        onClick={() => setMobileMenuOpen(false)}
      />

      {/* Sidebar */}
      <AdminSidebar
        mobileOpen={mobileMenuOpen}
        onMobileClose={() => setMobileMenuOpen(false)}
      />

      {/* Main content */}
      <main className={`admin-main ${sidebarCollapsed ? "sidebar-collapsed" : ""}`}>
        <AdminHeader onMobileMenuToggle={() => setMobileMenuOpen(!mobileMenuOpen)} />
        <div className="admin-content">
          {/* key={pathname} resets error boundary on navigation */}
          <AdminErrorBoundary key={pathname}>
            {children}
          </AdminErrorBoundary>
        </div>
      </main>
    </div>
  );
}
