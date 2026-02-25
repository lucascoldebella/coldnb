"use client";
import { useState, useEffect, useMemo, useCallback } from "react";
import Link from "next/link";
import { useRouter } from "next/navigation";
import { useAdmin } from "@/context/AdminContext";
import DataTable from "@/components/admin/tables/DataTable";
import { adminEmployees } from "@/lib/api/adminEmployees";
import toast from "react-hot-toast";
import { formatDate } from "@/lib/adminUtils";

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

export default function TeamPage() {
  const router = useRouter();
  const { admin, hasPermission } = useAdmin();
  const [loading, setLoading] = useState(true);
  const [employees, setEmployees] = useState([]);

  const isSuperAdmin = admin?.role === "super_admin";

  useEffect(() => {
    const fetchData = async () => {
      try {
        const res = await adminEmployees.list();
        const data = res.data?.data || res.data?.employees || res.data || [];
        setEmployees(data);
      } catch (error) {
        toast.error("Failed to load employees");
      } finally {
        setLoading(false);
      }
    };

    fetchData();
  }, []);

  const handleDelete = useCallback(async (id) => {
    if (!confirm("Are you sure you want to deactivate this employee?")) return;
    try {
      await adminEmployees.delete(id);
      toast.success("Employee deactivated successfully");
      setEmployees(prev => prev.map(e =>
        e.id === id ? { ...e, is_active: false } : e
      ));
    } catch (error) {
      toast.error("Failed to deactivate employee");
    }
  }, []);

  const getInitials = useCallback((name) => {
    if (!name) return "?";
    return name
      .split(" ")
      .map(n => n[0])
      .join("")
      .toUpperCase()
      .slice(0, 2);
  }, []);

  // Memoize columns to prevent unnecessary DataTable re-renders
  const columns = useMemo(() => [
    {
      header: "Employee",
      accessor: "full_name",
      render: (row) => (
        <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
          <div
            style={{
              width: 40,
              height: 40,
              borderRadius: "50%",
              backgroundColor: row.is_active ? "var(--admin-primary)" : "var(--admin-text-muted)",
              display: "flex",
              alignItems: "center",
              justifyContent: "center",
              color: "white",
              fontWeight: 600,
              fontSize: 14,
              flexShrink: 0,
              overflow: "hidden",
            }}
          >
            {row.photo_url ? (
              <img
                src={row.photo_url}
                alt={row.full_name}
                style={{ width: "100%", height: "100%", objectFit: "cover" }}
              />
            ) : (
              getInitials(row.full_name || row.username)
            )}
          </div>
          <div>
            <div style={{ fontWeight: 500 }}>{row.full_name || row.username}</div>
            <div style={{ fontSize: 12, color: "#6b7280" }}>{row.email}</div>
          </div>
        </div>
      ),
    },
    {
      header: "Username",
      accessor: "username",
      width: "140px",
      render: (row) => (
        <span style={{ fontFamily: "monospace", fontSize: 13 }}>@{row.username}</span>
      ),
    },
    {
      header: "Role",
      accessor: "role",
      width: "120px",
      render: (row) => (
        <span className={`role-badge role-${row.role}`}>
          {row.role === "super_admin" ? "Super Admin" : "Admin"}
        </span>
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
      header: "Last Login",
      accessor: "last_login",
      width: "160px",
      render: (row) => (
        <span style={{ fontSize: 13, color: "#6b7280" }}>
          {formatDate(row.last_login)}
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
          {(isSuperAdmin || hasPermission("edit_employees")) && (
            <Link href={`/admin/team/${row.id}`} className="action-btn action-edit">
              <EditIcon />
            </Link>
          )}
          {isSuperAdmin && row.id !== admin?.id && (
            <button
              className="action-btn action-delete"
              onClick={(e) => { e.stopPropagation(); handleDelete(row.id); }}
            >
              <TrashIcon />
            </button>
          )}
        </div>
      ),
    },
  ], [isSuperAdmin, hasPermission, admin?.id, handleDelete, getInitials]);

  return (
    <div className="team-page">
      <div className="admin-page-header">
        <h1 className="page-title">Team Management</h1>
        <div className="page-actions">
          {isSuperAdmin && (
            <Link href="/admin/team/new" className="admin-btn btn-primary">
              <PlusIcon />
              Add Employee
            </Link>
          )}
        </div>
      </div>

      {!isSuperAdmin && (
        <div className="alert-card alert-info" style={{ marginBottom: 24 }}>
          <div className="alert-content">
            <div className="alert-message">
              You can view team members but only Super Admins can create or modify employee accounts.
            </div>
          </div>
        </div>
      )}

      <DataTable
        columns={columns}
        data={employees}
        loading={loading}
        searchPlaceholder="Search employees..."
        onRowClick={(row) => router.push(`/admin/team/${row.id}`)}
      />
    </div>
  );
}
