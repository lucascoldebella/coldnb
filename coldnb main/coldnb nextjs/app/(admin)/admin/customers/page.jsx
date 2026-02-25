"use client";
import { useState, useEffect, useCallback } from "react";
import { useRouter } from "next/navigation";
import toast from "react-hot-toast";
import adminApi from "@/lib/adminApi";
import DataTable from "@/components/admin/tables/DataTable";
import { formatCurrency, formatDate, exportToCSV } from "@/lib/adminUtils";

const EyeIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z" />
    <circle cx="12" cy="12" r="3" />
  </svg>
);

const DownloadIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" />
    <polyline points="7 10 12 15 17 10" />
    <line x1="12" y1="15" x2="12" y2="3" />
  </svg>
);

export default function CustomersPage() {
  const router = useRouter();
  const [loading, setLoading] = useState(true);
  const [customers, setCustomers] = useState([]);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const res = await adminApi.get("/api/admin/customers");
        const data = res.data?.data?.customers || res.data?.data || res.data?.customers || [];
        setCustomers(Array.isArray(data) ? data : []);
      } catch (_error) {
        setCustomers([]);
      } finally {
        setLoading(false);
      }
    };

    fetchData();
  }, []);

  const handleExport = useCallback(() => {
    const csvColumns = [
      { header: "Name", accessor: "name" },
      { header: "Email", accessor: "email" },
      { header: "Phone", accessor: "phone" },
      { header: "Orders", accessor: "orders_count" },
      { header: "Total Spent", accessor: "total_spent" },
      { header: "Joined", accessor: "created_at", exportValue: (r) => formatDate(r.created_at) },
    ];
    exportToCSV(customers, csvColumns, "customers");
    toast.success("Customers exported");
  }, [customers]);

  const columns = [
    {
      header: "Customer",
      accessor: "name",
      render: (row) => (
        <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
          <div
            style={{
              width: 40,
              height: 40,
              borderRadius: "50%",
              backgroundColor: "var(--admin-primary)",
              display: "flex",
              alignItems: "center",
              justifyContent: "center",
              color: "white",
              fontWeight: 600,
              fontSize: 14,
              flexShrink: 0,
            }}
          >
            {(row.name || row.full_name || "?").split(" ").map(n => n[0]).join("").slice(0, 2)}
          </div>
          <div>
            <div style={{ fontWeight: 500 }}>{row.name || row.full_name}</div>
            <div style={{ fontSize: 12, color: "#6b7280" }}>{row.email}</div>
          </div>
        </div>
      ),
    },
    {
      header: "Phone",
      accessor: "phone",
      width: "140px",
    },
    {
      header: "Orders",
      accessor: "orders_count",
      width: "100px",
      render: (row) => (
        <span className="admin-badge badge-primary">{row.orders_count || 0}</span>
      ),
    },
    {
      header: "Total Spent",
      accessor: "total_spent",
      width: "140px",
      render: (row) => (
        <span style={{ fontWeight: 600 }}>{formatCurrency(row.total_spent)}</span>
      ),
    },
    {
      header: "Joined",
      accessor: "created_at",
      width: "120px",
      render: (row) => formatDate(row.created_at),
    },
    {
      header: "Actions",
      accessor: "actions",
      sortable: false,
      width: "80px",
      className: "cell-actions",
      render: (row) => (
        <div className="table-row-actions">
          <button
            className="action-btn action-view"
            onClick={(e) => {
              e.stopPropagation();
              router.push(`/admin/customers/${row.id}`);
            }}
          >
            <EyeIcon />
          </button>
        </div>
      ),
    },
  ];

  return (
    <div className="customers-page">
      <div className="admin-page-header">
        <h1 className="page-title">Customers</h1>
        <div className="page-actions">
          <button className="admin-btn btn-secondary" onClick={handleExport}>
            <DownloadIcon />
            Export
          </button>
        </div>
      </div>

      <DataTable
        columns={columns}
        data={customers}
        loading={loading}
        searchPlaceholder="Search customers..."
        onRowClick={(row) => router.push(`/admin/customers/${row.id}`)}
      />
    </div>
  );
}
