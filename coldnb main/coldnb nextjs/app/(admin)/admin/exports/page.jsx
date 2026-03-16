"use client";
import { useState } from "react";
import adminApi from "@/lib/adminApi";
import toast from "react-hot-toast";

export default function AdminExportsPage() {
  const [loading, setLoading] = useState({});
  const [dateFrom, setDateFrom] = useState("");
  const [dateTo, setDateTo] = useState("");

  const downloadCSV = async (type) => {
    setLoading((prev) => ({ ...prev, [type]: true }));
    try {
      let url = `/api/admin/exports/${type}`;
      if (type === "orders" && dateFrom && dateTo) {
        url += `?date_from=${dateFrom}&date_to=${dateTo}`;
      }

      const response = await adminApi.get(url, { responseType: "blob" });
      const blob = new Blob([response.data], { type: "text/csv" });
      const link = document.createElement("a");
      link.href = URL.createObjectURL(blob);
      link.download = `${type}-${new Date().toISOString().split("T")[0]}.csv`;
      link.click();
      URL.revokeObjectURL(link.href);
      toast.success(`${type.charAt(0).toUpperCase() + type.slice(1)} exported successfully`);
    } catch (error) {
      console.error("Export failed:", error);
      toast.error("Export failed. Please try again.");
    } finally {
      setLoading((prev) => ({ ...prev, [type]: false }));
    }
  };

  return (
    <div className="admin-page-content">
      <div className="d-flex justify-content-between align-items-center mb-4">
        <h4 className="mb-0">Data Exports</h4>
      </div>

      <div className="row g-4">
        {/* Orders Export */}
        <div className="col-md-4">
          <div className="admin-card">
            <div className="card-header">
              <h5 className="card-title mb-0">Orders</h5>
            </div>
            <div className="card-body">
              <p className="text-muted mb-3">
                Export all orders with customer info, payment status, shipping details, and totals.
              </p>
              <div className="mb-2">
                <label className="form-label small">Date Range (optional)</label>
                <div className="d-flex gap-2">
                  <input
                    type="date"
                    className="form-control form-control-sm"
                    value={dateFrom}
                    onChange={(e) => setDateFrom(e.target.value)}
                    placeholder="From"
                  />
                  <input
                    type="date"
                    className="form-control form-control-sm"
                    value={dateTo}
                    onChange={(e) => setDateTo(e.target.value)}
                    placeholder="To"
                  />
                </div>
              </div>
            </div>
            <div className="card-footer">
              <button
                className="btn btn-primary w-100"
                onClick={() => downloadCSV("orders")}
                disabled={loading.orders}
              >
                {loading.orders ? (
                  <span className="spinner-border spinner-border-sm me-2" />
                ) : null}
                Export Orders CSV
              </button>
            </div>
          </div>
        </div>

        {/* Products Export */}
        <div className="col-md-4">
          <div className="admin-card">
            <div className="card-header">
              <h5 className="card-title mb-0">Products</h5>
            </div>
            <div className="card-body">
              <p className="text-muted mb-3">
                Export all products with SKU, price, stock quantity, category, and status flags.
              </p>
            </div>
            <div className="card-footer">
              <button
                className="btn btn-primary w-100"
                onClick={() => downloadCSV("products")}
                disabled={loading.products}
              >
                {loading.products ? (
                  <span className="spinner-border spinner-border-sm me-2" />
                ) : null}
                Export Products CSV
              </button>
            </div>
          </div>
        </div>

        {/* Customers Export */}
        <div className="col-md-4">
          <div className="admin-card">
            <div className="card-header">
              <h5 className="card-title mb-0">Customers</h5>
            </div>
            <div className="card-body">
              <p className="text-muted mb-3">
                Export all customers with email, name, order count, and total spent.
              </p>
            </div>
            <div className="card-footer">
              <button
                className="btn btn-primary w-100"
                onClick={() => downloadCSV("customers")}
                disabled={loading.customers}
              >
                {loading.customers ? (
                  <span className="spinner-border spinner-border-sm me-2" />
                ) : null}
                Export Customers CSV
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
