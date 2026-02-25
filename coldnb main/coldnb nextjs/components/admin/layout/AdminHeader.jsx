"use client";
import { useState, useEffect, useRef } from "react";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { useAdmin } from "@/context/AdminContext";

const MenuIcon = () => (
  <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="3" y1="12" x2="21" y2="12" />
    <line x1="3" y1="6" x2="21" y2="6" />
    <line x1="3" y1="18" x2="21" y2="18" />
  </svg>
);

const SearchIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <circle cx="11" cy="11" r="8" />
    <line x1="21" y1="21" x2="16.65" y2="16.65" />
  </svg>
);

const BellIcon = () => (
  <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M18 8A6 6 0 0 0 6 8c0 7-3 9-3 9h18s-3-2-3-9" />
    <path d="M13.73 21a2 2 0 0 1-3.46 0" />
  </svg>
);

const ChevronIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="9 18 15 12 9 6" />
  </svg>
);

const UserIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2" />
    <circle cx="12" cy="7" r="4" />
  </svg>
);

const SettingsIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <circle cx="12" cy="12" r="3" />
    <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z" />
  </svg>
);

const LogoutIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4" />
    <polyline points="16 17 21 12 16 7" />
    <line x1="21" y1="12" x2="9" y2="12" />
  </svg>
);

// Breadcrumb mapping
const breadcrumbMap = {
  "/admin": "Dashboard",
  "/admin/dashboard": "Dashboard",
  "/admin/financial": "Financial",
  "/admin/products": "Products",
  "/admin/products/new": "New Product",
  "/admin/categories": "Categories",
  "/admin/orders": "Orders",
  "/admin/customers": "Customers",
  "/admin/marketing": "Marketing",
  "/admin/team": "Team",
  "/admin/team/new": "New Employee",
};

export default function AdminHeader() {
  const pathname = usePathname();
  const { admin, toggleSidebar, logout } = useAdmin();
  const [showDropdown, setShowDropdown] = useState(false);
  const [searchQuery, setSearchQuery] = useState("");
  const dropdownRef = useRef(null);

  // Generate breadcrumbs
  const getBreadcrumbs = () => {
    const parts = pathname.split("/").filter(Boolean);
    const breadcrumbs = [];
    let currentPath = "";

    for (let i = 0; i < parts.length; i++) {
      currentPath += `/${parts[i]}`;
      const label = breadcrumbMap[currentPath];

      // Handle dynamic routes (like [id])
      if (!label && parts[i].match(/^[0-9a-fA-F-]+$/)) {
        // This is likely an ID, skip it for breadcrumb
        continue;
      }

      if (label) {
        breadcrumbs.push({
          path: currentPath,
          label,
          isLast: i === parts.length - 1,
        });
      }
    }

    return breadcrumbs;
  };

  const breadcrumbs = getBreadcrumbs();

  // Close dropdown when clicking outside
  useEffect(() => {
    const handleClickOutside = (event) => {
      if (dropdownRef.current && !dropdownRef.current.contains(event.target)) {
        setShowDropdown(false);
      }
    };

    document.addEventListener("mousedown", handleClickOutside);
    return () => document.removeEventListener("mousedown", handleClickOutside);
  }, []);

  const getInitials = (name) => {
    if (!name) return "A";
    return name
      .split(" ")
      .map((n) => n[0])
      .join("")
      .toUpperCase()
      .slice(0, 2);
  };

  const handleSearch = (e) => {
    e.preventDefault();
  };

  return (
    <header className="admin-header">
      <div className="header-left">
        <button className="header-toggle" onClick={toggleSidebar}>
          <MenuIcon />
        </button>

        <nav className="header-breadcrumb">
          {breadcrumbs.map((crumb, index) => (
            <span key={crumb.path}>
              {index > 0 && <span className="breadcrumb-separator"><ChevronIcon /></span>}
              {crumb.isLast ? (
                <span className="breadcrumb-current">{crumb.label}</span>
              ) : (
                <Link href={crumb.path}>{crumb.label}</Link>
              )}
            </span>
          ))}
        </nav>
      </div>

      <div className="header-right">
        <form className="header-search" onSubmit={handleSearch}>
          <span className="search-icon"><SearchIcon /></span>
          <input
            type="text"
            placeholder="Search..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
          />
        </form>

        <button className="header-icon-btn">
          <BellIcon />
          <span className="notification-dot" />
        </button>

        <div className="dropdown-btn-wrapper" ref={dropdownRef}>
          <button
            className="header-user"
            onClick={() => setShowDropdown(!showDropdown)}
          >
            <div className="user-avatar">
              {admin?.photo_url ? (
                <img src={admin.photo_url} alt={admin?.username} />
              ) : (
                getInitials(admin?.username)
              )}
            </div>
            <span className="user-name">{admin?.username || "Admin"}</span>
          </button>

          <div className={`dropdown-menu ${showDropdown ? "open" : ""}`}>
            <Link href="/admin/profile" className="dropdown-item" onClick={() => setShowDropdown(false)}>
              <UserIcon />
              Profile
            </Link>
            <Link href="/admin/settings" className="dropdown-item" onClick={() => setShowDropdown(false)}>
              <SettingsIcon />
              Settings
            </Link>
            <div className="dropdown-divider" />
            <button className="dropdown-item danger" onClick={logout}>
              <LogoutIcon />
              Logout
            </button>
          </div>
        </div>
      </div>
    </header>
  );
}
