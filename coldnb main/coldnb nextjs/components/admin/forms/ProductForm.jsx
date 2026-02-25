"use client";
import { useState, useCallback, useEffect } from "react";
import { validateImageFiles } from "@/lib/adminUtils";

const UploadIcon = () => (
  <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round">
    <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" />
    <polyline points="17 8 12 3 7 8" />
    <line x1="12" y1="3" x2="12" y2="15" />
  </svg>
);

const XIcon = () => (
  <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="18" y1="6" x2="6" y2="18" />
    <line x1="6" y1="6" x2="18" y2="18" />
  </svg>
);

const PlusIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="12" y1="5" x2="12" y2="19" />
    <line x1="5" y1="12" x2="19" y2="12" />
  </svg>
);

export default function ProductForm({
  initialData = {},
  categories = [],
  onSubmit,
  isSubmitting = false,
  submitLabel = "Save Product",
}) {
  const [formData, setFormData] = useState({
    name: initialData.name || "",
    slug: initialData.slug || "",
    sku: initialData.sku || "",
    description: initialData.description || "",
    price: initialData.price || "",
    compare_at_price: initialData.compare_at_price || "",
    cost_price: initialData.cost_price || "",
    category_id: initialData.category_id || "",
    stock_quantity: initialData.stock_quantity ?? 0,
    low_stock_threshold: initialData.low_stock_threshold ?? 10,
    weight: initialData.weight || "",
    is_active: initialData.is_active !== undefined ? Boolean(initialData.is_active) : true,
    is_featured: Boolean(initialData.is_featured),
    meta_title: initialData.meta_title || "",
    meta_description: initialData.meta_description || "",
  });

  const [images, setImages] = useState(initialData.images || []);
  const [deletedImageIds, setDeletedImageIds] = useState([]);
  const [colors, setColors] = useState(initialData.colors || []);
  const [sizes, setSizes] = useState(initialData.sizes || []);
  const [errors, setErrors] = useState({});
  const [dragging, setDragging] = useState(false);
  const [uploading, setUploading] = useState(false);
  const [uploadError, setUploadError] = useState(null);

  useEffect(() => {
    if (initialData && initialData.id) {
      setFormData({
        name: initialData.name || "",
        slug: initialData.slug || "",
        sku: initialData.sku || "",
        description: initialData.description || "",
        price: initialData.price || "",
        compare_at_price: initialData.compare_at_price || "",
        cost_price: initialData.cost_price || "",
        category_id: initialData.category_id || "",
        stock_quantity: initialData.stock_quantity ?? 0,
        low_stock_threshold: initialData.low_stock_threshold ?? 10,
        weight: initialData.weight || "",
        is_active: initialData.is_active !== undefined ? Boolean(initialData.is_active) : true,
        is_featured: Boolean(initialData.is_featured),
        meta_title: initialData.meta_title || "",
        meta_description: initialData.meta_description || "",
      });
      setImages(initialData.images || []);
      setDeletedImageIds([]);
      setColors(initialData.colors || []);
      setSizes(initialData.sizes || []);
    }
  }, [initialData]);

  const handleChange = (e) => {
    const { name, value, type, checked } = e.target;
    setFormData(prev => ({
      ...prev,
      [name]: type === "checkbox" ? checked : value,
    }));

    if (name === "name" && !initialData.id) {
      const slug = value
        .toLowerCase()
        .replace(/[^a-z0-9]+/g, "-")
        .replace(/^-|-$/g, "");
      setFormData(prev => ({ ...prev, slug }));
    }
  };

  const handleImageUpload = useCallback(async (files) => {
    const fileArray = Array.from(files);
    if (fileArray.length === 0) return;

    // Validate files before uploading
    const { validFiles, errors: validationErrors } = validateImageFiles(fileArray);
    if (validationErrors.length > 0) {
      setUploadError(validationErrors.join(" "));
      if (validFiles.length === 0) return;
    }

    setUploading(true);
    setUploadError(null);

    try {
      const uploadFormData = new FormData();
      validFiles.forEach(file => {
        uploadFormData.append("files", file);
      });

      const response = await fetch("/api/admin/upload", {
        method: "POST",
        body: uploadFormData,
      });

      const result = await response.json();

      if (!result.success) {
        throw new Error(result.error || "Upload failed");
      }

      const newImages = result.data.urls.map((url, index) => ({
        id: Date.now() + index,
        url: url,
        preview: url,
        is_primary: images.length === 0 && index === 0,
      }));

      setImages(prev => [...prev, ...newImages]);
    } catch (error) {
      setUploadError(error.message || "Failed to upload images");
    } finally {
      setUploading(false);
    }
  }, [images.length]);

  const handleDrop = useCallback((e) => {
    e.preventDefault();
    setDragging(false);
    const files = e.dataTransfer.files;
    if (files.length) {
      handleImageUpload(files);
    }
  }, [handleImageUpload]);

  const handleDragOver = (e) => {
    e.preventDefault();
    setDragging(true);
  };

  const handleDragLeave = () => {
    setDragging(false);
  };

  const removeImage = (id) => {
    setImages(prev => {
      const imageToRemove = prev.find(img => img.id === id);
      if (imageToRemove && imageToRemove.product_id) {
        setDeletedImageIds(prevDeleted => [...prevDeleted, id]);
      }
      const filtered = prev.filter(img => img.id !== id);
      if (filtered.length > 0 && !filtered.some(img => img.is_primary)) {
        filtered[0].is_primary = true;
      }
      return filtered;
    });
  };

  const setPrimaryImage = (id) => {
    setImages(prev => prev.map(img => ({
      ...img,
      is_primary: img.id === id,
    })));
  };

  const addColor = () => {
    setColors(prev => [...prev, { id: Date.now(), name: "", hex_code: "#000000" }]);
  };

  const updateColor = (id, field, value) => {
    setColors(prev => prev.map(c => c.id === id ? { ...c, [field]: value } : c));
  };

  const removeColor = (id) => {
    setColors(prev => prev.filter(c => c.id !== id));
  };

  const addSize = () => {
    setSizes(prev => [...prev, { id: Date.now(), name: "" }]);
  };

  const updateSize = (id, value) => {
    setSizes(prev => prev.map(s => s.id === id ? { ...s, name: value } : s));
  };

  const removeSize = (id) => {
    setSizes(prev => prev.filter(s => s.id !== id));
  };

  const validate = () => {
    const newErrors = {};
    if (!formData.name.trim()) newErrors.name = "Name is required";
    if (!formData.sku.trim()) newErrors.sku = "SKU is required";
    if (!formData.price) newErrors.price = "Price is required";
    if (!formData.category_id) newErrors.category_id = "Category is required";
    setErrors(newErrors);
    return Object.keys(newErrors).length === 0;
  };

  const handleSubmit = (e) => {
    e.preventDefault();
    if (!validate()) return;

    const submitData = {
      name: formData.name,
      slug: formData.slug,
      sku: formData.sku,
      description: formData.description || null,
      price: parseFloat(formData.price) || 0,
      stock_quantity: parseInt(formData.stock_quantity, 10) || 0,
      low_stock_threshold: parseInt(formData.low_stock_threshold, 10) || 10,
      is_active: Boolean(formData.is_active),
      is_featured: Boolean(formData.is_featured),
      meta_title: formData.meta_title || null,
      meta_description: formData.meta_description || null,
      images,
      deletedImageIds,
      colors: colors.filter(c => c.name.trim()),
      sizes: sizes.filter(s => s.name.trim()),
    };

    if (formData.compare_at_price) {
      submitData.compare_at_price = parseFloat(formData.compare_at_price);
    }
    if (formData.cost_price) {
      submitData.cost_price = parseFloat(formData.cost_price);
    }
    if (formData.weight) {
      submitData.weight = parseFloat(formData.weight);
    }
    const categoryId = parseInt(formData.category_id, 10);
    if (categoryId > 0) {
      submitData.category_id = categoryId;
    }

    onSubmit(submitData);
  };

  return (
    <form className="admin-form" onSubmit={handleSubmit}>
      {/* Basic Information */}
      <div className="form-section">
        <div className="section-header">
          <h3 className="section-title">Basic Information</h3>
        </div>
        <div className="section-body">
          <div className="form-row cols-2">
            <div className="form-group">
              <label>Product Name <span className="required">*</span></label>
              <input
                type="text"
                name="name"
                className={`admin-input ${errors.name ? "error" : ""}`}
                value={formData.name}
                onChange={handleChange}
                placeholder="Enter product name"
              />
              {errors.name && <div className="form-error">{errors.name}</div>}
            </div>
            <div className="form-group">
              <label>URL Slug</label>
              <input
                type="text"
                name="slug"
                className="admin-input"
                value={formData.slug}
                onChange={handleChange}
                placeholder="product-url-slug"
              />
            </div>
          </div>

          <div className="form-row cols-2">
            <div className="form-group">
              <label>SKU <span className="required">*</span></label>
              <input
                type="text"
                name="sku"
                className={`admin-input ${errors.sku ? "error" : ""}`}
                value={formData.sku}
                onChange={handleChange}
                placeholder="PRD-001"
              />
              {errors.sku && <div className="form-error">{errors.sku}</div>}
            </div>
            <div className="form-group">
              <label>Category <span className="required">*</span></label>
              <select
                name="category_id"
                className={`admin-select ${errors.category_id ? "error" : ""}`}
                value={formData.category_id}
                onChange={handleChange}
              >
                <option value="">Select category</option>
                {categories.map(cat => (
                  <option key={cat.id} value={cat.id}>{cat.name}</option>
                ))}
              </select>
              {errors.category_id && <div className="form-error">{errors.category_id}</div>}
            </div>
          </div>

          <div className="form-group">
            <label>Description</label>
            <textarea
              name="description"
              className="admin-textarea"
              value={formData.description}
              onChange={handleChange}
              placeholder="Enter product description..."
              rows={4}
            />
          </div>
        </div>
      </div>

      {/* Images */}
      <div className="form-section">
        <div className="section-header">
          <h3 className="section-title">Images</h3>
          {uploading && <span style={{ fontSize: 12, color: "#6b7280" }}>Uploading...</span>}
        </div>
        <div className="section-body">
          {uploadError && (
            <div style={{ color: "#dc2626", fontSize: 14, marginBottom: 12 }}>
              {uploadError}
            </div>
          )}
          <div className="image-uploader">
            <div
              className={`upload-area ${dragging ? "dragging" : ""} ${uploading ? "uploading" : ""}`}
              onDrop={handleDrop}
              onDragOver={handleDragOver}
              onDragLeave={handleDragLeave}
              onClick={() => !uploading && document.getElementById("image-input").click()}
              style={{ opacity: uploading ? 0.6 : 1, pointerEvents: uploading ? "none" : "auto" }}
            >
              <span className="upload-icon"><UploadIcon /></span>
              <p className="upload-text">
                {uploading ? "Uploading images..." : <><span>Click to upload</span> or drag and drop</>}
              </p>
              <p className="upload-hint">JPEG, PNG, GIF, WebP up to 5MB</p>
              <input
                id="image-input"
                type="file"
                multiple
                accept="image/jpeg,image/png,image/gif,image/webp"
                style={{ display: "none" }}
                onChange={(e) => { handleImageUpload(e.target.files); e.target.value = ""; }}
                disabled={uploading}
              />
            </div>

            {images.length > 0 && (
              <div className="upload-preview">
                {images.map(img => (
                  <div
                    key={img.id}
                    className={`preview-item ${img.is_primary ? "primary" : ""}`}
                    onClick={() => setPrimaryImage(img.id)}
                    style={{ cursor: "pointer" }}
                  >
                    <img src={img.preview || img.url} alt="Product" />
                    <button
                      type="button"
                      className="preview-remove"
                      onClick={(e) => { e.stopPropagation(); removeImage(img.id); }}
                    >
                      <XIcon />
                    </button>
                  </div>
                ))}
              </div>
            )}
          </div>
          <p className="form-hint">Click an image to set it as primary</p>
        </div>
      </div>

      {/* Pricing */}
      <div className="form-section">
        <div className="section-header">
          <h3 className="section-title">Pricing</h3>
        </div>
        <div className="section-body">
          <div className="form-row cols-3">
            <div className="form-group">
              <label>Price <span className="required">*</span></label>
              <div className="input-group">
                <span className="input-addon">R$</span>
                <input
                  type="number"
                  name="price"
                  className={`admin-input ${errors.price ? "error" : ""}`}
                  value={formData.price}
                  onChange={handleChange}
                  placeholder="0.00"
                  step="0.01"
                  min="0"
                />
              </div>
              {errors.price && <div className="form-error">{errors.price}</div>}
            </div>
            <div className="form-group">
              <label>Compare at Price</label>
              <div className="input-group">
                <span className="input-addon">R$</span>
                <input
                  type="number"
                  name="compare_at_price"
                  className="admin-input"
                  value={formData.compare_at_price}
                  onChange={handleChange}
                  placeholder="0.00"
                  step="0.01"
                  min="0"
                />
              </div>
              <p className="form-hint">Original price for sale items</p>
            </div>
            <div className="form-group">
              <label>Cost Price</label>
              <div className="input-group">
                <span className="input-addon">R$</span>
                <input
                  type="number"
                  name="cost_price"
                  className="admin-input"
                  value={formData.cost_price}
                  onChange={handleChange}
                  placeholder="0.00"
                  step="0.01"
                  min="0"
                />
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Inventory */}
      <div className="form-section">
        <div className="section-header">
          <h3 className="section-title">Inventory</h3>
        </div>
        <div className="section-body">
          <div className="form-row cols-3">
            <div className="form-group">
              <label>Stock Quantity</label>
              <input
                type="number"
                name="stock_quantity"
                className="admin-input"
                value={formData.stock_quantity}
                onChange={handleChange}
                min="0"
              />
            </div>
            <div className="form-group">
              <label>Low Stock Threshold</label>
              <input
                type="number"
                name="low_stock_threshold"
                className="admin-input"
                value={formData.low_stock_threshold}
                onChange={handleChange}
                min="0"
              />
            </div>
            <div className="form-group">
              <label>Weight (grams)</label>
              <input
                type="number"
                name="weight"
                className="admin-input"
                value={formData.weight}
                onChange={handleChange}
                placeholder="0"
                min="0"
              />
            </div>
          </div>
        </div>
      </div>

      {/* Variants */}
      <div className="form-section">
        <div className="section-header">
          <h3 className="section-title">Variants</h3>
        </div>
        <div className="section-body">
          {/* Colors */}
          <div className="form-group">
            <label>Colors</label>
            {colors.map(color => (
              <div key={color.id} style={{ display: "flex", gap: 8, marginBottom: 8 }}>
                <input
                  type="color"
                  value={color.hex_code}
                  onChange={(e) => updateColor(color.id, "hex_code", e.target.value)}
                  style={{ width: 42, height: 42, padding: 0, border: "1px solid var(--admin-border)", borderRadius: "var(--admin-radius)" }}
                />
                <input
                  type="text"
                  className="admin-input"
                  value={color.name}
                  onChange={(e) => updateColor(color.id, "name", e.target.value)}
                  placeholder="Color name"
                  style={{ flex: 1 }}
                />
                <button
                  type="button"
                  className="admin-btn btn-ghost btn-icon"
                  onClick={() => removeColor(color.id)}
                >
                  <XIcon />
                </button>
              </div>
            ))}
            <button type="button" className="admin-btn btn-secondary btn-sm" onClick={addColor}>
              <PlusIcon /> Add Color
            </button>
          </div>

          {/* Sizes */}
          <div className="form-group" style={{ marginTop: 20 }}>
            <label>Sizes</label>
            {sizes.map(size => (
              <div key={size.id} style={{ display: "flex", gap: 8, marginBottom: 8 }}>
                <input
                  type="text"
                  className="admin-input"
                  value={size.name}
                  onChange={(e) => updateSize(size.id, e.target.value)}
                  placeholder="Size (e.g., S, M, L, 7, 8)"
                  style={{ flex: 1 }}
                />
                <button
                  type="button"
                  className="admin-btn btn-ghost btn-icon"
                  onClick={() => removeSize(size.id)}
                >
                  <XIcon />
                </button>
              </div>
            ))}
            <button type="button" className="admin-btn btn-secondary btn-sm" onClick={addSize}>
              <PlusIcon /> Add Size
            </button>
          </div>
        </div>
      </div>

      {/* Status */}
      <div className="form-section">
        <div className="section-header">
          <h3 className="section-title">Status</h3>
        </div>
        <div className="section-body">
          <div style={{ display: "flex", gap: 24 }}>
            <label className="admin-toggle">
              <span
                className={`toggle-switch ${formData.is_active ? "active" : ""}`}
                onClick={() => setFormData(prev => ({ ...prev, is_active: !prev.is_active }))}
              />
              <span className="toggle-label">Active</span>
            </label>
            <label className="admin-toggle">
              <span
                className={`toggle-switch ${formData.is_featured ? "active" : ""}`}
                onClick={() => setFormData(prev => ({ ...prev, is_featured: !prev.is_featured }))}
              />
              <span className="toggle-label">Featured</span>
            </label>
          </div>
        </div>
      </div>

      {/* SEO */}
      <div className="form-section">
        <div className="section-header">
          <h3 className="section-title">SEO</h3>
          <p className="section-subtitle">Search engine optimization settings</p>
        </div>
        <div className="section-body">
          <div className="form-group">
            <label>Meta Title</label>
            <input
              type="text"
              name="meta_title"
              className="admin-input"
              value={formData.meta_title}
              onChange={handleChange}
              placeholder="Product title for search engines"
            />
          </div>
          <div className="form-group">
            <label>Meta Description</label>
            <textarea
              name="meta_description"
              className="admin-textarea"
              value={formData.meta_description}
              onChange={handleChange}
              placeholder="Brief description for search results..."
              rows={3}
            />
          </div>
        </div>
      </div>

      {/* Actions */}
      <div className="form-actions">
        <button type="button" className="admin-btn btn-secondary" onClick={() => window.history.back()}>
          Cancel
        </button>
        <button type="submit" className="admin-btn btn-primary" disabled={isSubmitting}>
          {isSubmitting ? "Saving..." : submitLabel}
        </button>
      </div>
    </form>
  );
}
