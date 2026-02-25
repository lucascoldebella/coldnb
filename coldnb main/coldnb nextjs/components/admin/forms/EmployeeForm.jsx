"use client";
import { useState, useCallback } from "react";
import PermissionCheckboxes from "./PermissionCheckboxes";

const UploadIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" />
    <polyline points="17 8 12 3 7 8" />
    <line x1="12" y1="3" x2="12" y2="15" />
  </svg>
);

const XIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="18" y1="6" x2="6" y2="18" />
    <line x1="6" y1="6" x2="18" y2="18" />
  </svg>
);

export default function EmployeeForm({
  initialData = {},
  onSubmit,
  isSubmitting = false,
  submitLabel = "Save Employee",
  showPasswordFields = true,
}) {
  const [formData, setFormData] = useState({
    username: initialData.username || "",
    email: initialData.email || "",
    password: "",
    confirmPassword: "",
    full_name: initialData.full_name || "",
    employee_id: initialData.employee_id || "",
    cpf: initialData.cpf || "",
    phone: initialData.phone || "",
    role: initialData.role || "admin",
    is_active: initialData.is_active !== undefined ? initialData.is_active : true,
    ...initialData,
  });

  const [permissions, setPermissions] = useState(initialData.permissions || {});
  const [photo, setPhoto] = useState(initialData.photo_url || null);
  const [photoFile, setPhotoFile] = useState(null);
  const [errors, setErrors] = useState({});

  const handleChange = (e) => {
    const { name, value, type, checked } = e.target;
    setFormData(prev => ({
      ...prev,
      [name]: type === "checkbox" ? checked : value,
    }));

    // Clear error when field is edited
    if (errors[name]) {
      setErrors(prev => ({ ...prev, [name]: null }));
    }
  };

  const handlePhotoUpload = (e) => {
    const file = e.target.files[0];
    if (file) {
      setPhotoFile(file);
      setPhoto(URL.createObjectURL(file));
    }
  };

  const removePhoto = () => {
    setPhoto(null);
    setPhotoFile(null);
  };

  const formatCPF = (value) => {
    // Remove non-digits
    const digits = value.replace(/\D/g, "");
    // Format as XXX.XXX.XXX-XX
    if (digits.length <= 3) return digits;
    if (digits.length <= 6) return `${digits.slice(0, 3)}.${digits.slice(3)}`;
    if (digits.length <= 9) return `${digits.slice(0, 3)}.${digits.slice(3, 6)}.${digits.slice(6)}`;
    return `${digits.slice(0, 3)}.${digits.slice(3, 6)}.${digits.slice(6, 9)}-${digits.slice(9, 11)}`;
  };

  const handleCPFChange = (e) => {
    const formatted = formatCPF(e.target.value);
    setFormData(prev => ({ ...prev, cpf: formatted }));
  };

  const formatPhone = (value) => {
    const digits = value.replace(/\D/g, "");
    if (digits.length <= 2) return `(${digits}`;
    if (digits.length <= 7) return `(${digits.slice(0, 2)}) ${digits.slice(2)}`;
    return `(${digits.slice(0, 2)}) ${digits.slice(2, 7)}-${digits.slice(7, 11)}`;
  };

  const handlePhoneChange = (e) => {
    const formatted = formatPhone(e.target.value);
    setFormData(prev => ({ ...prev, phone: formatted }));
  };

  const validate = () => {
    const newErrors = {};

    if (!formData.username.trim()) {
      newErrors.username = "Username is required";
    }

    if (showPasswordFields && !initialData.id) {
      if (!formData.password) {
        newErrors.password = "Password is required";
      } else if (formData.password.length < 8) {
        newErrors.password = "Password must be at least 8 characters";
      }

      if (formData.password !== formData.confirmPassword) {
        newErrors.confirmPassword = "Passwords do not match";
      }
    }

    if (formData.email && !/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(formData.email)) {
      newErrors.email = "Invalid email address";
    }

    setErrors(newErrors);
    return Object.keys(newErrors).length === 0;
  };

  const handleSubmit = (e) => {
    e.preventDefault();
    if (!validate()) return;

    const submitData = {
      ...formData,
      permissions,
      photoFile,
    };

    // Remove confirmPassword from submission
    delete submitData.confirmPassword;

    // Remove password if empty (for updates)
    if (!submitData.password) {
      delete submitData.password;
    }

    onSubmit(submitData);
  };

  return (
    <form className="admin-form" onSubmit={handleSubmit}>
      {/* Photo & Basic Info */}
      <div className="form-section">
        <div className="section-header">
          <h3 className="section-title">Profile Information</h3>
        </div>
        <div className="section-body">
          <div style={{ display: "flex", gap: 24, marginBottom: 20 }}>
            {/* Photo Upload */}
            <div style={{ flexShrink: 0 }}>
              <div
                style={{
                  width: 120,
                  height: 120,
                  borderRadius: "50%",
                  backgroundColor: "var(--admin-bg)",
                  border: "2px dashed var(--admin-border)",
                  display: "flex",
                  alignItems: "center",
                  justifyContent: "center",
                  overflow: "hidden",
                  cursor: "pointer",
                  position: "relative",
                }}
                onClick={() => document.getElementById("photo-input").click()}
              >
                {photo ? (
                  <>
                    <img
                      src={photo}
                      alt="Profile"
                      style={{ width: "100%", height: "100%", objectFit: "cover" }}
                    />
                    <button
                      type="button"
                      onClick={(e) => { e.stopPropagation(); removePhoto(); }}
                      style={{
                        position: "absolute",
                        top: 0,
                        right: 0,
                        width: 28,
                        height: 28,
                        borderRadius: "50%",
                        backgroundColor: "var(--admin-danger)",
                        color: "white",
                        border: "none",
                        cursor: "pointer",
                        display: "flex",
                        alignItems: "center",
                        justifyContent: "center",
                      }}
                    >
                      <XIcon />
                    </button>
                  </>
                ) : (
                  <div style={{ textAlign: "center", color: "var(--admin-text-muted)" }}>
                    <UploadIcon />
                    <div style={{ fontSize: 12, marginTop: 4 }}>Upload</div>
                  </div>
                )}
                <input
                  id="photo-input"
                  type="file"
                  accept="image/*"
                  style={{ display: "none" }}
                  onChange={handlePhotoUpload}
                />
              </div>
            </div>

            {/* Name Fields */}
            <div style={{ flex: 1 }}>
              <div className="form-group">
                <label>Full Name</label>
                <input
                  type="text"
                  name="full_name"
                  className="admin-input"
                  value={formData.full_name}
                  onChange={handleChange}
                  placeholder="Enter full name"
                />
              </div>
              <div className="form-row cols-2" style={{ marginTop: 16 }}>
                <div className="form-group" style={{ marginBottom: 0 }}>
                  <label>Employee ID</label>
                  <input
                    type="text"
                    name="employee_id"
                    className="admin-input"
                    value={formData.employee_id}
                    onChange={handleChange}
                    placeholder="EMP-001"
                  />
                </div>
                <div className="form-group" style={{ marginBottom: 0 }}>
                  <label>CPF</label>
                  <input
                    type="text"
                    name="cpf"
                    className="admin-input"
                    value={formData.cpf}
                    onChange={handleCPFChange}
                    placeholder="000.000.000-00"
                    maxLength={14}
                  />
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Account Information */}
      <div className="form-section">
        <div className="section-header">
          <h3 className="section-title">Account Information</h3>
        </div>
        <div className="section-body">
          <div className="form-row cols-2">
            <div className="form-group">
              <label>Username <span className="required">*</span></label>
              <input
                type="text"
                name="username"
                className={`admin-input ${errors.username ? "error" : ""}`}
                value={formData.username}
                onChange={handleChange}
                placeholder="Enter username"
              />
              {errors.username && <div className="form-error">{errors.username}</div>}
            </div>
            <div className="form-group">
              <label>Email</label>
              <input
                type="email"
                name="email"
                className={`admin-input ${errors.email ? "error" : ""}`}
                value={formData.email}
                onChange={handleChange}
                placeholder="email@example.com"
              />
              {errors.email && <div className="form-error">{errors.email}</div>}
            </div>
          </div>

          <div className="form-group">
            <label>Phone</label>
            <input
              type="text"
              name="phone"
              className="admin-input"
              value={formData.phone}
              onChange={handlePhoneChange}
              placeholder="(00) 00000-0000"
              maxLength={15}
            />
          </div>

          {showPasswordFields && (
            <div className="form-row cols-2">
              <div className="form-group">
                <label>
                  Password {!initialData.id && <span className="required">*</span>}
                </label>
                <input
                  type="password"
                  name="password"
                  className={`admin-input ${errors.password ? "error" : ""}`}
                  value={formData.password}
                  onChange={handleChange}
                  placeholder={initialData.id ? "Leave blank to keep current" : "Enter password"}
                />
                {errors.password && <div className="form-error">{errors.password}</div>}
              </div>
              <div className="form-group">
                <label>
                  Confirm Password {!initialData.id && <span className="required">*</span>}
                </label>
                <input
                  type="password"
                  name="confirmPassword"
                  className={`admin-input ${errors.confirmPassword ? "error" : ""}`}
                  value={formData.confirmPassword}
                  onChange={handleChange}
                  placeholder="Confirm password"
                />
                {errors.confirmPassword && <div className="form-error">{errors.confirmPassword}</div>}
              </div>
            </div>
          )}
        </div>
      </div>

      {/* Role & Status */}
      <div className="form-section">
        <div className="section-header">
          <h3 className="section-title">Role & Status</h3>
        </div>
        <div className="section-body">
          <div className="form-row cols-2">
            <div className="form-group">
              <label>Role</label>
              <select
                name="role"
                className="admin-select"
                value={formData.role}
                onChange={handleChange}
              >
                <option value="admin">Admin</option>
                <option value="super_admin">Super Admin</option>
              </select>
              <p className="form-hint">
                Super Admin has full access to all features
              </p>
            </div>
            <div className="form-group">
              <label>Status</label>
              <div style={{ paddingTop: 8 }}>
                <label className="admin-toggle">
                  <span
                    className={`toggle-switch ${formData.is_active ? "active" : ""}`}
                    onClick={() => setFormData(prev => ({ ...prev, is_active: !prev.is_active }))}
                  />
                  <span className="toggle-label">
                    {formData.is_active ? "Active" : "Inactive"}
                  </span>
                </label>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Permissions */}
      {formData.role !== "super_admin" && (
        <div className="form-section">
          <div className="section-header">
            <h3 className="section-title">Permissions</h3>
            <p className="section-subtitle">
              Select which features this employee can access
            </p>
          </div>
          <div className="section-body">
            <PermissionCheckboxes
              permissions={permissions}
              onChange={setPermissions}
            />
          </div>
        </div>
      )}

      {/* Actions */}
      <div className="form-actions">
        <button
          type="button"
          className="admin-btn btn-secondary"
          onClick={() => window.history.back()}
        >
          Cancel
        </button>
        <button
          type="submit"
          className="admin-btn btn-primary"
          disabled={isSubmitting}
        >
          {isSubmitting ? "Saving..." : submitLabel}
        </button>
      </div>
    </form>
  );
}
