"use client";
import { useState } from "react";
import toast from "react-hot-toast";
import { adminNavMenus, adminNavGroups, adminNavItems } from "@/lib/api/adminNavigation";
import NavItemFormModal from "./NavItemFormModal";
import NavGroupFormModal from "./NavGroupFormModal";

const fieldGroupStyle = {
  background: "var(--admin-bg)",
  borderRadius: "var(--admin-radius)",
  padding: "16px",
  marginBottom: 12,
  border: "1px solid var(--admin-border-light)",
};

const PencilIcon = () => (
  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" />
    <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" />
  </svg>
);

const TrashIcon = () => (
  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="3 6 5 6 21 6" /><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" />
  </svg>
);

const PlusIcon = () => (
  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="12" y1="5" x2="12" y2="19" /><line x1="5" y1="12" x2="19" y2="12" />
  </svg>
);

export default function MenuEditor({ menu, groups, hasPermission, onRefresh }) {
  const [editingItem, setEditingItem] = useState(null);
  const [creatingItemGroupId, setCreatingItemGroupId] = useState(null);
  const [editingGroup, setEditingGroup] = useState(null);
  const [creatingGroup, setCreatingGroup] = useState(false);
  const [editingMenu, setEditingMenu] = useState(false);
  const [menuForm, setMenuForm] = useState({});

  const isGrid = menu.menu_type === "mega_grid";

  /* ---- Menu-level settings ---- */
  const handleMenuSave = async () => {
    try {
      await adminNavMenus.update(menu.id, menuForm);
      toast.success("Menu updated");
      setEditingMenu(false);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update menu");
    }
  };

  /* ---- Group CRUD ---- */
  const handleGroupCreate = async (data) => {
    try {
      await adminNavGroups.create(menu.id, data);
      toast.success("Group created");
      setCreatingGroup(false);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to create group");
    }
  };

  const handleGroupUpdate = async (data) => {
    try {
      await adminNavGroups.update(editingGroup.id, data);
      toast.success("Group updated");
      setEditingGroup(null);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update group");
    }
  };

  const handleGroupDelete = async (id) => {
    if (!confirm("Delete this group and all its items?")) return;
    try {
      await adminNavGroups.delete(id);
      toast.success("Group deleted");
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to delete group");
    }
  };

  /* ---- Item CRUD ---- */
  const handleItemCreate = async (data) => {
    try {
      await adminNavItems.create(creatingItemGroupId, data);
      toast.success("Item created");
      setCreatingItemGroupId(null);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to create item");
    }
  };

  const handleItemUpdate = async (data) => {
    try {
      await adminNavItems.update(editingItem.id, data);
      toast.success("Item updated");
      setEditingItem(null);
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to update item");
    }
  };

  const handleItemDelete = async (id) => {
    if (!confirm("Delete this item?")) return;
    try {
      await adminNavItems.delete(id);
      toast.success("Item deleted");
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Failed to delete item");
    }
  };

  const handleToggleActive = async (item) => {
    try {
      await adminNavItems.update(item.id, { is_active: !item.is_active });
      onRefresh();
    } catch (err) {
      toast.error(err.response?.data?.error || "Toggle failed");
    }
  };

  return (
    <div>
      {/* Menu settings row */}
      {!editingMenu ? (
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 12 }}>
          <div style={{ fontSize: 12, color: "var(--admin-text-secondary)" }}>
            Type: <strong>{menu.menu_type}</strong>
            {menu.show_products && <> &middot; Products carousel: {menu.products_count}</>}
            {menu.banner_image_url && <> &middot; Has banner</>}
          </div>
          <div style={{ display: "flex", gap: 6 }}>
            {hasPermission && (
              <>
                <button className="admin-btn btn-secondary btn-sm" onClick={() => { setEditingMenu(true); setMenuForm({ show_products: menu.show_products, products_count: menu.products_count || 4, banner_image_url: menu.banner_image_url || "", banner_link: menu.banner_link || "", banner_title: menu.banner_title || "" }); }}>
                  Settings
                </button>
                <button className="admin-btn btn-primary btn-sm" onClick={() => setCreatingGroup(true)}>
                  <PlusIcon /> Group
                </button>
              </>
            )}
          </div>
        </div>
      ) : (
        <div style={fieldGroupStyle}>
          <div style={{ fontSize: 11, fontWeight: 600, textTransform: "uppercase", color: "var(--admin-primary)", marginBottom: 8 }}>Menu Settings</div>
          {menu.menu_type === "mega_columns" && (
            <>
              <div className="form-row cols-2">
                <div className="form-group">
                  <label>Show Products</label>
                  <div className="admin-toggle" onClick={() => setMenuForm((p) => ({ ...p, show_products: !p.show_products }))}>
                    <span className={`toggle-switch ${menuForm.show_products ? "active" : ""}`} />
                    <span className="toggle-label">{menuForm.show_products ? "Yes" : "No"}</span>
                  </div>
                </div>
                {menuForm.show_products && (
                  <div className="form-group">
                    <label>Products Count</label>
                    <input className="admin-input" type="number" value={menuForm.products_count} onChange={(e) => setMenuForm((p) => ({ ...p, products_count: parseInt(e.target.value) || 4 }))} />
                  </div>
                )}
              </div>
              <div className="form-group">
                <label>Banner Image URL</label>
                <input className="admin-input" value={menuForm.banner_image_url} onChange={(e) => setMenuForm((p) => ({ ...p, banner_image_url: e.target.value }))} />
              </div>
              <div className="form-row cols-2">
                <div className="form-group">
                  <label>Banner Link</label>
                  <input className="admin-input" value={menuForm.banner_link} onChange={(e) => setMenuForm((p) => ({ ...p, banner_link: e.target.value }))} />
                </div>
                <div className="form-group">
                  <label>Banner Title Key</label>
                  <input className="admin-input" value={menuForm.banner_title} onChange={(e) => setMenuForm((p) => ({ ...p, banner_title: e.target.value }))} />
                </div>
              </div>
            </>
          )}
          <div style={{ display: "flex", gap: 8, justifyContent: "flex-end" }}>
            <button className="admin-btn btn-secondary btn-sm" onClick={() => setEditingMenu(false)}>Cancel</button>
            <button className="admin-btn btn-primary btn-sm" onClick={handleMenuSave}>Save Settings</button>
          </div>
        </div>
      )}

      {/* Groups & items */}
      {groups.map((group) => (
        <div key={group.id} style={fieldGroupStyle}>
          <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 8 }}>
            <div style={{ fontWeight: 600, fontSize: 13 }}>
              {group.title || "(Unnamed group)"}
              {group.translation_key && (
                <span style={{ fontWeight: 400, fontSize: 11, color: "var(--admin-text-secondary)", marginLeft: 6 }}>
                  [{group.translation_key}]
                </span>
              )}
              <span style={{ fontSize: 11, color: "var(--admin-text-secondary)", marginLeft: 6 }}>
                ({(group.items || []).length} items)
              </span>
            </div>
            {hasPermission && (
              <div style={{ display: "flex", gap: 4 }}>
                <button className="admin-btn btn-secondary btn-sm" style={{ padding: "4px 8px" }} onClick={() => setEditingGroup(group)}>
                  <PencilIcon />
                </button>
                <button className="admin-btn btn-danger btn-sm" style={{ padding: "4px 8px" }} onClick={() => handleGroupDelete(group.id)}>
                  <TrashIcon />
                </button>
                <button className="admin-btn btn-primary btn-sm" style={{ padding: "4px 8px" }} onClick={() => setCreatingItemGroupId(group.id)}>
                  <PlusIcon />
                </button>
              </div>
            )}
          </div>

          {/* Items list */}
          <div style={{ display: "flex", flexDirection: "column", gap: 4 }}>
            {(group.items || []).map((item) => (
              <div key={item.id} style={{
                display: "flex", alignItems: "center", gap: 8, padding: "6px 8px",
                borderRadius: 6, background: item.is_active ? "var(--admin-card-bg)" : "var(--admin-bg)",
                border: "1px solid var(--admin-border-light)",
                opacity: item.is_active ? 1 : 0.5,
              }}>
                {isGrid && item.image_url && (
                  <div style={{ width: 40, height: 28, borderRadius: 4, overflow: "hidden", flexShrink: 0, background: "#f3f4f6" }}>
                    <img src={item.image_url} alt={item.image_alt || ""} style={{ width: "100%", height: "100%", objectFit: "cover" }} />
                  </div>
                )}
                <div style={{ flex: 1, minWidth: 0 }}>
                  <div style={{ fontSize: 13, fontWeight: 500 }}>{item.label}</div>
                  <div style={{ fontSize: 11, color: "var(--admin-text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
                    {item.href}
                    {item.badge && <span className="admin-badge badge-info" style={{ fontSize: 10, marginLeft: 6, padding: "1px 5px" }}>{item.badge}</span>}
                  </div>
                </div>
                {hasPermission && (
                  <div style={{ display: "flex", gap: 2, flexShrink: 0 }}>
                    <button className="admin-btn btn-secondary btn-sm" style={{ padding: "3px 6px" }} onClick={() => handleToggleActive(item)} title={item.is_active ? "Deactivate" : "Activate"}>
                      {item.is_active ? "On" : "Off"}
                    </button>
                    <button className="admin-btn btn-secondary btn-sm" style={{ padding: "3px 6px" }} onClick={() => setEditingItem(item)}>
                      <PencilIcon />
                    </button>
                    <button className="admin-btn btn-danger btn-sm" style={{ padding: "3px 6px" }} onClick={() => handleItemDelete(item.id)}>
                      <TrashIcon />
                    </button>
                  </div>
                )}
              </div>
            ))}
            {(group.items || []).length === 0 && (
              <div style={{ fontSize: 12, color: "var(--admin-text-secondary)", padding: "8px 0", textAlign: "center" }}>
                No items yet
              </div>
            )}
          </div>
        </div>
      ))}

      {groups.length === 0 && (
        <div style={{ textAlign: "center", padding: 24, color: "var(--admin-text-secondary)", fontSize: 13 }}>
          No groups yet. Add a group to start adding menu items.
        </div>
      )}

      {/* Modals */}
      {creatingItemGroupId && (
        <NavItemFormModal showImageFields={isGrid} onSave={handleItemCreate} onClose={() => setCreatingItemGroupId(null)} />
      )}
      {editingItem && (
        <NavItemFormModal item={editingItem} showImageFields={isGrid} onSave={handleItemUpdate} onClose={() => setEditingItem(null)} />
      )}
      {creatingGroup && (
        <NavGroupFormModal onSave={handleGroupCreate} onClose={() => setCreatingGroup(false)} />
      )}
      {editingGroup && (
        <NavGroupFormModal group={editingGroup} onSave={handleGroupUpdate} onClose={() => setEditingGroup(null)} />
      )}
    </div>
  );
}
