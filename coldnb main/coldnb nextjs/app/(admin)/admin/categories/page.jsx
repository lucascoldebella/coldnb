"use client";
import { useState, useEffect, useRef, useMemo, useCallback } from "react";
import { useAdmin } from "@/context/AdminContext";
import { adminCategories } from "@/lib/api/adminProducts";
import toast from "react-hot-toast";
import DataTable from "@/components/admin/tables/DataTable";
import ImageCropper from "@/components/admin/common/ImageCropper";

const PlusIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="12" y1="5" x2="12" y2="19" />
    <line x1="5" y1="12" x2="19" y2="12" />
  </svg>
);

const EditIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" />
    <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" />
  </svg>
);

const TrashIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="3 6 5 6 21 6" />
    <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" />
  </svg>
);

const XIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="18" y1="6" x2="6" y2="18" />
    <line x1="6" y1="6" x2="18" y2="18" />
  </svg>
);

export default function CategoriesPage() {
  const { hasPermission } = useAdmin();
  const [loading, setLoading] = useState(true);
  const [categories, setCategories] = useState([]);
  const [showModal, setShowModal] = useState(false);
  const [editingCategory, setEditingCategory] = useState(null);
  const [formData, setFormData] = useState({ name: "", slug: "", parent_id: "", image_url: "", is_active: true });
  const [saving, setSaving] = useState(false);
  const [uploading, setUploading] = useState(false);
  const [cropSrc, setCropSrc] = useState(null);
  const fileInputRef = useRef(null);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const res = await adminCategories.list();
        const data = res.data?.data || res.data?.categories || res.data || [];
        setCategories(data);
      } catch (error) {
        toast.error("Failed to load categories");
      } finally {
        setLoading(false);
      }
    };

    fetchData();
  }, []);

  const openModal = useCallback((category = null) => {
    if (category) {
      setEditingCategory(category);
      setFormData({
        name: category.name,
        slug: category.slug,
        parent_id: category.parent_id || "",
        image_url: category.image_url || "",
        is_active: category.is_active,
      });
    } else {
      setEditingCategory(null);
      setFormData({ name: "", slug: "", parent_id: "", image_url: "", is_active: true });
    }
    setShowModal(true);
  }, []);

  const closeModal = () => {
    setShowModal(false);
    setEditingCategory(null);
    setFormData({ name: "", slug: "", parent_id: "", image_url: "", is_active: true });
  };

  const handleFileSelect = (e) => {
    const file = e.target.files?.[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = () => setCropSrc(reader.result);
    reader.readAsDataURL(file);
    if (fileInputRef.current) fileInputRef.current.value = "";
  };

  const handleCropComplete = async (blob) => {
    setCropSrc(null);
    setUploading(true);
    try {
      const fd = new FormData();
      fd.append("files", new File([blob], "category.jpg", { type: "image/jpeg" }));
      const res = await fetch("/api/admin/upload", { method: "POST", body: fd });
      const json = await res.json();
      if (json.success && json.data?.urls?.[0]) {
        setFormData(prev => ({ ...prev, image_url: json.data.urls[0] }));
      }
    } catch {
      toast.error("Image upload failed");
    } finally {
      setUploading(false);
    }
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    setSaving(true);

    try {
      const payload = {
        ...formData,
        parent_id: formData.parent_id ? parseInt(formData.parent_id, 10) : null,
      };

      if (editingCategory) {
        const res = await adminCategories.update(editingCategory.id, payload);
        const serverData = res.data?.data || res.data?.category || {};
        // Merge server response with old row to preserve fields not in RETURNING (e.g. product_count)
        const updated = { ...editingCategory, ...payload, ...serverData };
        setCategories(categories.map(c =>
          c.id === editingCategory.id ? updated : c
        ));
        toast.success("Category updated successfully");
      } else {
        const res = await adminCategories.create(payload);
        const created = res.data?.data || res.data?.category || res.data;
        setCategories([...categories, created]);
        toast.success("Category created successfully");
      }

      closeModal();
    } catch (error) {
      toast.error(editingCategory ? "Failed to update category" : "Failed to create category");
    } finally {
      setSaving(false);
    }
  };

  const handleDelete = useCallback(async (id) => {
    if (!confirm("Are you sure you want to delete this category?")) return;
    try {
      await adminCategories.delete(id);
      setCategories(prev => prev.filter(c => c.id !== id));
      toast.success("Category deleted successfully");
    } catch (error) {
      toast.error("Failed to delete category");
    }
  }, []);

  const handleNameChange = (value) => {
    // Only auto-generate slug when creating (not when editing an existing category)
    if (editingCategory) {
      setFormData(prev => ({ ...prev, name: value }));
    } else {
      const slug = value.toLowerCase().replace(/[^a-z0-9]+/g, "-").replace(/^-|-$/g, "");
      setFormData(prev => ({ ...prev, name: value, slug }));
    }
  };

  const getParentName = useCallback((parentId) => {
    const parent = categories.find(c => c.id === parentId);
    return parent ? parent.name : "-";
  }, [categories]);

  const rootCategories = useMemo(() => categories.filter(c => !c.parent_id), [categories]);

  const columns = useMemo(() => [
    {
      header: "Category",
      accessor: "name",
      render: (row) => (
        <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
          <div style={{
            width: 36, height: 36, borderRadius: "50%", overflow: "hidden",
            backgroundColor: "#f3f4f6", flexShrink: 0,
          }}>
            {row.image_url && (
              <img src={row.image_url} alt={row.name} style={{ width: "100%", height: "100%", objectFit: "cover" }} />
            )}
          </div>
          <div>
            <div style={{ fontWeight: 500 }}>{row.name}</div>
            <div style={{ fontSize: 12, color: "#6b7280" }}>/{row.slug}</div>
          </div>
        </div>
      ),
    },
    {
      header: "Parent",
      accessor: "parent_id",
      width: "140px",
      render: (row) => getParentName(row.parent_id),
    },
    {
      header: "Products",
      accessor: "product_count",
      width: "100px",
      render: (row) => (
        <span className="admin-badge badge-primary">{row.product_count}</span>
      ),
    },
    {
      header: "Status",
      accessor: "is_active",
      width: "100px",
      render: (row) => (
        <span className={`admin-badge ${row.is_active ? "badge-success" : "badge-danger"}`}>
          {row.is_active ? "Active" : "Inactive"}
        </span>
      ),
    },
    {
      header: "Actions",
      accessor: "actions",
      sortable: false,
      width: "100px",
      className: "cell-actions",
      render: (row) => (
        <div className="table-row-actions">
          {hasPermission("manage_categories") && (
            <>
              <button className="action-btn action-edit" onClick={(e) => { e.stopPropagation(); openModal(row); }}>
                <EditIcon />
              </button>
              <button className="action-btn action-delete" onClick={(e) => { e.stopPropagation(); handleDelete(row.id); }}>
                <TrashIcon />
              </button>
            </>
          )}
        </div>
      ),
    },
  ], [hasPermission, handleDelete, openModal, getParentName]);

  return (
    <div className="categories-page">
      <div className="admin-page-header">
        <h1 className="page-title">Categories</h1>
        <div className="page-actions">
          {hasPermission("manage_categories") && (
            <button className="admin-btn btn-primary" onClick={() => openModal()}>
              <PlusIcon />
              Add Category
            </button>
          )}
        </div>
      </div>

      <DataTable
        columns={columns}
        data={categories}
        loading={loading}
        searchPlaceholder="Search categories..."
        pagination={false}
      />

      {/* Modal */}
      {showModal && (
        <div className="admin-modal-overlay open">
          <div className="admin-modal md" onClick={e => e.stopPropagation()}>
            <div className="modal-header">
              <h3 className="modal-title">
                {editingCategory ? "Edit Category" : "New Category"}
              </h3>
              <button className="modal-close" onClick={closeModal}>
                <XIcon />
              </button>
            </div>
            <form onSubmit={handleSubmit}>
              <div className="modal-body">
                <div className="form-group">
                  <label>Category Name <span className="required">*</span></label>
                  <input
                    type="text"
                    className="admin-input"
                    value={formData.name}
                    onChange={(e) => handleNameChange(e.target.value)}
                    placeholder="Enter category name"
                    required
                  />
                </div>
                <div className="form-group">
                  <label>Slug</label>
                  <input
                    type="text"
                    className="admin-input"
                    value={formData.slug}
                    onChange={(e) => setFormData(prev => ({ ...prev, slug: e.target.value }))}
                    placeholder="category-slug"
                  />
                </div>
                <div className="form-group">
                  <label>Image</label>
                  {formData.image_url && (
                    <div style={{ marginBottom: 8 }}>
                      <img src={formData.image_url} alt="Preview" style={{ width: 64, height: 64, objectFit: "cover", borderRadius: "50%", border: "1px solid var(--admin-border-light)" }} />
                    </div>
                  )}
                  <div style={{ display: "flex", gap: 8 }}>
                    <input
                      className="admin-input"
                      value={formData.image_url}
                      onChange={(e) => setFormData(prev => ({ ...prev, image_url: e.target.value }))}
                      placeholder="/images/categories/..."
                      style={{ flex: 1 }}
                    />
                    <button
                      type="button"
                      className="admin-btn btn-secondary"
                      onClick={() => fileInputRef.current?.click()}
                      disabled={uploading}
                      style={{ whiteSpace: "nowrap" }}
                    >
                      {uploading ? "Uploading..." : "Browse"}
                    </button>
                    <input ref={fileInputRef} type="file" accept="image/jpeg,image/png,image/gif,image/webp" onChange={handleFileSelect} style={{ display: "none" }} />
                  </div>
                </div>
                <div className="form-group">
                  <label>Parent Category</label>
                  <select
                    className="admin-select"
                    value={formData.parent_id}
                    onChange={(e) => setFormData(prev => ({ ...prev, parent_id: e.target.value }))}
                  >
                    <option value="">None (Top Level)</option>
                    {rootCategories
                      .filter(c => c.id !== editingCategory?.id)
                      .map(c => (
                        <option key={c.id} value={c.id}>{c.name}</option>
                      ))
                    }
                  </select>
                </div>
                <div className="form-group">
                  <label className="admin-toggle">
                    <span
                      className={`toggle-switch ${formData.is_active ? "active" : ""}`}
                      onClick={() => setFormData(prev => ({ ...prev, is_active: !prev.is_active }))}
                    />
                    <span className="toggle-label">Active</span>
                  </label>
                </div>
              </div>
              <div className="modal-footer">
                <button type="button" className="admin-btn btn-secondary" onClick={closeModal}>
                  Cancel
                </button>
                <button type="submit" className="admin-btn btn-primary" disabled={saving}>
                  {saving ? "Saving..." : (editingCategory ? "Update" : "Create")}
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {cropSrc && (
        <ImageCropper
          imageSrc={cropSrc}
          onCrop={handleCropComplete}
          onCancel={() => setCropSrc(null)}
        />
      )}
    </div>
  );
}
