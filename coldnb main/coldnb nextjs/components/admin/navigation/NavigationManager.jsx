"use client";
import { useState, useEffect, useCallback } from "react";
import toast from "react-hot-toast";
import { adminNavMenus, adminNavGroups } from "@/lib/api/adminNavigation";
import MenuEditor from "./MenuEditor";

const extractArray = (res, key) => {
  const wrapper = res?.data?.data ?? res?.data;
  if (wrapper && typeof wrapper === "object" && !Array.isArray(wrapper)) {
    const arr = wrapper[key];
    if (Array.isArray(arr)) return arr;
  }
  return Array.isArray(wrapper) ? wrapper : [];
};

const ChevronIcon = ({ open }) => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"
    style={{ transform: open ? "rotate(180deg)" : "rotate(0deg)", transition: "transform 0.2s" }}>
    <polyline points="6 9 12 15 18 9" />
  </svg>
);

const menuTypeLabels = {
  mega_grid: "Mega Grid",
  mega_columns: "Mega Columns",
  simple: "Simple Dropdown",
};

export default function NavigationManager({ hasPermission }) {
  const [menus, setMenus] = useState([]);
  const [menuGroups, setMenuGroups] = useState({});
  const [loading, setLoading] = useState(true);
  const [expanded, setExpanded] = useState({});
  const [toggling, setToggling] = useState({});

  const fetchMenus = useCallback(async () => {
    try {
      const res = await adminNavMenus.list();
      const list = extractArray(res, "menus");
      setMenus(list);
      // Auto-expand first menu
      if (list.length > 0 && Object.keys(expanded).length === 0) {
        setExpanded({ [list[0].id]: true });
      }
    } catch {
      toast.error("Failed to load navigation menus");
    } finally {
      setLoading(false);
    }
  }, []); // eslint-disable-line react-hooks/exhaustive-deps

  const fetchGroups = useCallback(async (menuId) => {
    try {
      const res = await adminNavGroups.list(menuId);
      const list = extractArray(res, "groups");
      setMenuGroups((prev) => ({ ...prev, [menuId]: list }));
    } catch {
      // silent
    }
  }, []);

  useEffect(() => {
    fetchMenus();
  }, [fetchMenus]);

  // Fetch groups when a menu is expanded
  useEffect(() => {
    Object.keys(expanded).forEach((id) => {
      if (expanded[id] && !menuGroups[id]) {
        fetchGroups(parseInt(id));
      }
    });
  }, [expanded, menuGroups, fetchGroups]);

  const toggleMenu = (menuId) => {
    setExpanded((prev) => ({ ...prev, [menuId]: !prev[menuId] }));
  };

  const handleToggleMenuActive = async (e, menu) => {
    e.stopPropagation();
    setToggling((prev) => ({ ...prev, [menu.id]: true }));
    try {
      await adminNavMenus.update(menu.id, { is_active: !menu.is_active });
      toast.success(`${menu.name} ${menu.is_active ? "hidden from" : "shown in"} navigation`);
      await fetchMenus();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to toggle menu");
    } finally {
      setToggling((prev) => ({ ...prev, [menu.id]: false }));
    }
  };

  const handleRefresh = async (menuId) => {
    await fetchGroups(menuId);
    await fetchMenus();
  };

  if (loading) {
    return (
      <div style={{ textAlign: "center", padding: 24, color: "var(--admin-text-secondary)" }}>
        Loading navigation...
      </div>
    );
  }

  return (
    <div>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 16 }}>
        <h3 style={{ margin: 0, fontSize: 16 }}>Navigation Menus ({menus.length})</h3>
      </div>

      <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
        {menus.map((menu) => {
          const isOpen = expanded[menu.id];
          return (
            <div key={menu.id} className="admin-card" style={{ opacity: menu.is_active ? 1 : 0.55, transition: "opacity 0.2s" }}>
              <div
                style={{
                  display: "flex", alignItems: "center", gap: 12,
                  padding: "12px 16px", cursor: "pointer",
                  borderBottom: isOpen ? "1px solid var(--admin-border)" : "none",
                }}
                onClick={() => toggleMenu(menu.id)}
              >
                <div style={{ flex: 1 }}>
                  <div style={{ fontWeight: 600, fontSize: 14 }}>
                    {menu.name}
                    <span style={{ fontWeight: 400, fontSize: 11, color: "var(--admin-text-secondary)", marginLeft: 8 }}>
                      /{menu.slug}
                    </span>
                    {!menu.is_active && (
                      <span className="admin-badge badge-danger" style={{ fontSize: 10, marginLeft: 8, verticalAlign: "middle" }}>
                        Hidden
                      </span>
                    )}
                  </div>
                  <div style={{ fontSize: 12, color: "var(--admin-text-secondary)", marginTop: 2 }}>
                    {menuTypeLabels[menu.menu_type] || menu.menu_type}
                    {" "}&middot; {menu.group_count || 0} groups &middot; {menu.item_count || 0} items
                  </div>
                </div>
                {hasPermission && (
                  <div
                    className="admin-toggle"
                    style={{ margin: 0 }}
                    onClick={(e) => handleToggleMenuActive(e, menu)}
                    title={menu.is_active ? `Hide "${menu.name}" from navigation` : `Show "${menu.name}" in navigation`}
                  >
                    <span className={`toggle-switch ${menu.is_active ? "active" : ""} ${toggling[menu.id] ? "disabled" : ""}`} />
                  </div>
                )}
                {!hasPermission && (
                  <span className={`admin-badge ${menu.is_active ? "badge-success" : "badge-danger"}`} style={{ fontSize: 11 }}>
                    {menu.is_active ? "Visible" : "Hidden"}
                  </span>
                )}
                <ChevronIcon open={isOpen} />
              </div>
              {isOpen && (
                <div style={{ padding: 16 }}>
                  <MenuEditor
                    menu={menu}
                    groups={menuGroups[menu.id] || []}
                    hasPermission={hasPermission}
                    onRefresh={() => handleRefresh(menu.id)}
                  />
                </div>
              )}
            </div>
          );
        })}
      </div>
    </div>
  );
}
