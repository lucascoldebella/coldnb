"use client";
import { useState, useEffect } from "react";
import { useRouter, useParams } from "next/navigation";
import toast from "react-hot-toast";
import { useAdmin } from "@/context/AdminContext";
import { adminEmployees } from "@/lib/api/adminEmployees";
import EmployeeForm from "@/components/admin/forms/EmployeeForm";

export default function EditEmployeePage() {
  const router = useRouter();
  const params = useParams();
  const { admin, hasPermission } = useAdmin();
  const [employee, setEmployee] = useState(null);
  const [loading, setLoading] = useState(true);
  const [isSubmitting, setIsSubmitting] = useState(false);

  const isSuperAdmin = admin?.role === "super_admin";
  const canEdit = isSuperAdmin || hasPermission("edit_employees");

  useEffect(() => {
    const fetchData = async () => {
      try {
        const res = await adminEmployees.get(params.id);
        const data = res.data?.data || res.data?.employee || res.data;
        setEmployee(data);
      } catch (_error) {
        setEmployee(null);
      } finally {
        setLoading(false);
      }
    };

    if (params.id) {
      fetchData();
    }
  }, [params.id]);

  const handleSubmit = async (data) => {
    setIsSubmitting(true);
    try {
      await adminEmployees.update(params.id, data);
      toast.success("Employee updated successfully");
      router.push("/admin/team");
    } catch (error) {
      const errData = error.response?.data?.error;
      const errMsg = typeof errData === 'object' ? errData.message : errData;
      toast.error(errMsg || "Error updating employee. Please try again.");
    } finally {
      setIsSubmitting(false);
    }
  };

  if (loading) {
    return (
      <div className="edit-employee-page">
        <div className="admin-page-header">
          <div className="skeleton skeleton-title" style={{ width: 200 }} />
        </div>
        <div className="admin-card">
          <div className="card-body">
            <div className="skeleton skeleton-text" style={{ marginBottom: 16 }} />
            <div className="skeleton skeleton-text" style={{ marginBottom: 16 }} />
            <div className="skeleton skeleton-text" />
          </div>
        </div>
      </div>
    );
  }

  if (!employee) {
    return (
      <div className="edit-employee-page">
        <div className="admin-page-header">
          <h1 className="page-title">Employee Not Found</h1>
        </div>
        <div className="admin-card">
          <div className="card-body" style={{ textAlign: "center", padding: 40 }}>
            <p>The employee you&apos;re looking for doesn&apos;t exist.</p>
            <button className="admin-btn btn-primary" onClick={() => router.push("/admin/team")}>
              Back to Team
            </button>
          </div>
        </div>
      </div>
    );
  }

  // View-only mode for non-super admins
  if (!canEdit) {
    return (
      <div className="edit-employee-page">
        <div className="admin-page-header">
          <h1 className="page-title">Employee Details</h1>
        </div>

        <div className="admin-card">
          <div className="card-header">
            <h3 className="card-title">Profile Information</h3>
          </div>
          <div className="card-body">
            <div style={{ display: "flex", gap: 24, alignItems: "center" }}>
              <div
                style={{
                  width: 80,
                  height: 80,
                  borderRadius: "50%",
                  backgroundColor: "var(--admin-primary)",
                  display: "flex",
                  alignItems: "center",
                  justifyContent: "center",
                  color: "white",
                  fontWeight: 700,
                  fontSize: 28,
                }}
              >
                {employee.full_name?.split(" ").map(n => n[0]).join("").slice(0, 2) || "?"}
              </div>
              <div>
                <h2 style={{ margin: 0, fontSize: 24 }}>{employee.full_name}</h2>
                <p style={{ margin: "4px 0 0", color: "#6b7280" }}>@{employee.username}</p>
                <div style={{ marginTop: 8 }}>
                  <span className={`role-badge role-${employee.role}`}>
                    {employee.role === "super_admin" ? "Super Admin" : "Admin"}
                  </span>
                  <span
                    className={`admin-badge ${employee.is_active ? "badge-success" : "badge-danger"}`}
                    style={{ marginLeft: 8 }}
                  >
                    {employee.is_active ? "Active" : "Inactive"}
                  </span>
                </div>
              </div>
            </div>

            <div className="admin-divider" />

            <div style={{ display: "grid", gridTemplateColumns: "repeat(2, 1fr)", gap: 16 }}>
              <div>
                <div style={{ fontSize: 12, color: "#6b7280", marginBottom: 4 }}>Email</div>
                <div>{employee.email || "-"}</div>
              </div>
              <div>
                <div style={{ fontSize: 12, color: "#6b7280", marginBottom: 4 }}>Phone</div>
                <div>{employee.phone || "-"}</div>
              </div>
              <div>
                <div style={{ fontSize: 12, color: "#6b7280", marginBottom: 4 }}>Employee ID</div>
                <div>{employee.employee_id || "-"}</div>
              </div>
              <div>
                <div style={{ fontSize: 12, color: "#6b7280", marginBottom: 4 }}>CPF</div>
                <div>{employee.cpf || "-"}</div>
              </div>
            </div>
          </div>
        </div>

        <div style={{ marginTop: 24 }}>
          <button className="admin-btn btn-secondary" onClick={() => router.push("/admin/team")}>
            Back to Team
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="edit-employee-page">
      <div className="admin-page-header">
        <h1 className="page-title">Edit Employee</h1>
      </div>

      <EmployeeForm
        initialData={employee}
        onSubmit={handleSubmit}
        isSubmitting={isSubmitting}
        submitLabel="Update Employee"
        showPasswordFields={isSuperAdmin}
      />
    </div>
  );
}
