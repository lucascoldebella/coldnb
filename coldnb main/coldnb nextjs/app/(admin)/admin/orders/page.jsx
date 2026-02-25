"use client";
import { useState, useEffect, useMemo } from "react";
import Link from "next/link";
import { useRouter } from "next/navigation";
import DataTable from "@/components/admin/tables/DataTable";
import StatusBadge from "@/components/admin/orders/StatusBadge";
import { adminOrders } from "@/lib/api/adminOrders";
import toast from "react-hot-toast";
import { formatCurrency, formatDateTime, exportToCSV } from "@/lib/adminUtils";

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

export default function OrdersPage() {
  const router = useRouter();
  const [loading, setLoading] = useState(true);
  const [orders, setOrders] = useState([]);
  const [filters, setFilters] = useState({
    status: "all",
    payment_status: "all",
  });

  const fetchOrders = async () => {
    try {
      setLoading(true);
      const res = await adminOrders.list();
      const data =
        res.data?.data?.orders || res.data?.data || res.data?.orders || [];
      setOrders(Array.isArray(data) ? data : []);
    } catch (error) {
      toast.error("Failed to load orders");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchOrders();
  }, []);

  // Filter orders (memoized)
  const filteredOrders = useMemo(() => {
    return orders.filter((order) => {
      if (filters.status !== "all" && order.status !== filters.status) return false;
      if (filters.payment_status !== "all" && order.payment_status !== filters.payment_status) return false;
      return true;
    });
  }, [orders, filters]);

  const handleFilterChange = (key, value) => {
    setFilters(prev => ({ ...prev, [key]: value }));
  };

  const handleStatusUpdate = async (orderId, newStatus) => {
    try {
      await adminOrders.updateStatus(orderId, newStatus);
      setOrders(orders.map(o =>
        o.id === orderId ? { ...o, status: newStatus } : o
      ));
      toast.success("Order status updated");
    } catch (error) {
      toast.error("Failed to update order status");
    }
  };

  const handleExportCSV = () => {
    const csvColumns = [
      { header: "Order Number", accessor: "order_number" },
      { header: "Customer", accessor: "customer_name" },
      { header: "Email", accessor: "customer_email" },
      { header: "Total", accessor: "total", exportValue: (row) => row.total },
      { header: "Status", accessor: "status" },
      { header: "Payment", accessor: "payment_status" },
      { header: "Items", accessor: "items_count" },
      { header: "Date", accessor: "created_at", exportValue: (row) => formatDateTime(row.created_at) },
    ];
    exportToCSV(filteredOrders, csvColumns, "orders");
  };

  // Memoize columns to prevent unnecessary DataTable re-renders
  const columns = useMemo(() => [
    {
      header: "Order",
      accessor: "order_number",
      render: (row) => (
        <div>
          <div style={{ fontWeight: 600 }}>{row.order_number}</div>
          <div style={{ fontSize: 12, color: "#6b7280" }}>
            {row.items_count} item{row.items_count !== 1 ? "s" : ""}
          </div>
        </div>
      ),
    },
    {
      header: "Customer",
      accessor: "customer_name",
      render: (row) => (
        <div>
          <div style={{ fontWeight: 500 }}>{row.customer_name}</div>
          <div style={{ fontSize: 12, color: "#6b7280" }}>{row.customer_email}</div>
        </div>
      ),
    },
    {
      header: "Date",
      accessor: "created_at",
      width: "160px",
      render: (row) => formatDateTime(row.created_at),
    },
    {
      header: "Total",
      accessor: "total",
      width: "120px",
      render: (row) => (
        <span style={{ fontWeight: 600 }}>{formatCurrency(row.total)}</span>
      ),
    },
    {
      header: "Payment",
      accessor: "payment_status",
      width: "120px",
      render: (row) => <StatusBadge status={row.payment_status} type="payment" />,
    },
    {
      header: "Status",
      accessor: "status",
      width: "140px",
      render: (row) => (
        <div className="dropdown-btn-wrapper">
          <StatusBadge status={row.status} type="order" />
        </div>
      ),
    },
    {
      header: "Actions",
      accessor: "actions",
      sortable: false,
      width: "80px",
      className: "cell-actions",
      render: (row) => (
        <div className="table-row-actions">
          <Link href={`/admin/orders/${row.id}`} className="action-btn action-view">
            <EyeIcon />
          </Link>
        </div>
      ),
    },
  ], []);

  return (
    <div className="orders-page">
      <div className="admin-page-header">
        <h1 className="page-title">Orders</h1>
        <div className="page-actions">
          <button className="admin-btn btn-secondary" onClick={handleExportCSV}>
            <DownloadIcon />
            Export
          </button>
        </div>
      </div>

      <DataTable
        columns={columns}
        data={filteredOrders}
        loading={loading}
        searchPlaceholder="Search orders..."
        onRowClick={(row) => router.push(`/admin/orders/${row.id}`)}
        filters={
          <>
            <div className="filter-select">
              <select
                value={filters.status}
                onChange={(e) => handleFilterChange("status", e.target.value)}
              >
                <option value="all">All Status</option>
                <option value="pending">Pending</option>
                <option value="processing">Processing</option>
                <option value="shipped">Shipped</option>
                <option value="delivered">Delivered</option>
                <option value="cancelled">Cancelled</option>
              </select>
            </div>
            <div className="filter-select">
              <select
                value={filters.payment_status}
                onChange={(e) => handleFilterChange("payment_status", e.target.value)}
              >
                <option value="all">All Payments</option>
                <option value="paid">Paid</option>
                <option value="pending">Pending</option>
                <option value="failed">Failed</option>
                <option value="refunded">Refunded</option>
              </select>
            </div>
          </>
        }
      />
    </div>
  );
}
