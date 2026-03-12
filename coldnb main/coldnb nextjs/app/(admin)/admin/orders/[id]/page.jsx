"use client";
import { useState, useEffect } from "react";
import { useRouter, useParams } from "next/navigation";
import Link from "next/link";
import StatusBadge from "@/components/admin/orders/StatusBadge";
import OrderTimeline from "@/components/admin/orders/OrderTimeline";
import { adminOrders } from "@/lib/api/adminOrders";
import toast from "react-hot-toast";
import { formatCurrency, formatDate } from "@/lib/adminUtils";

const ArrowLeftIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="19" y1="12" x2="5" y2="12" />
    <polyline points="12 19 5 12 12 5" />
  </svg>
);

const PrintIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="6 9 6 2 18 2 18 9" />
    <path d="M6 18H4a2 2 0 0 1-2-2v-5a2 2 0 0 1 2-2h16a2 2 0 0 1 2 2v5a2 2 0 0 1-2 2h-2" />
    <rect x="6" y="14" width="12" height="8" />
  </svg>
);

const orderStatuses = [
  { value: "pending", label: "Pending" },
  { value: "processing", label: "Processing" },
  { value: "shipped", label: "Shipped" },
  { value: "delivered", label: "Delivered" },
  { value: "cancelled", label: "Cancelled" },
];

const carriers = [
  { value: "correios", label: "Correios" },
  { value: "sedex", label: "Sedex" },
  { value: "pac", label: "PAC" },
  { value: "dhl", label: "DHL" },
  { value: "fedex", label: "FedEx" },
  { value: "other", label: "Other" },
];

const TruckIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <rect x="1" y="3" width="15" height="13" />
    <polygon points="16 8 20 8 23 11 23 16 16 16 16 8" />
    <circle cx="5.5" cy="18.5" r="2.5" />
    <circle cx="18.5" cy="18.5" r="2.5" />
  </svg>
);

export default function OrderDetailPage() {
  const router = useRouter();
  const params = useParams();
  const [loading, setLoading] = useState(true);
  const [order, setOrder] = useState(null);
  const [updatingStatus, setUpdatingStatus] = useState(false);
  const [showTrackingForm, setShowTrackingForm] = useState(false);
  const [trackingData, setTrackingData] = useState({ tracking_number: "", carrier: "correios", estimated_delivery: "" });
  const [savingTracking, setSavingTracking] = useState(false);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const res = await adminOrders.get(params.id);
        const orderData = res.data?.data || res.data?.order || res.data;
        setOrder(orderData);
      } catch (error) {
        toast.error("Failed to load order details");
      } finally {
        setLoading(false);
      }
    };

    fetchData();
  }, [params.id]);

  const handleStatusChange = async (newStatus) => {
    setUpdatingStatus(true);
    try {
      await adminOrders.updateStatus(params.id, newStatus);
      setOrder(prev => ({ ...prev, status: newStatus }));
      toast.success("Order status updated successfully");
    } catch (error) {
      toast.error("Failed to update order status");
    } finally {
      setUpdatingStatus(false);
    }
  };

  const handleTrackingSubmit = async (e) => {
    e.preventDefault();
    if (!trackingData.tracking_number.trim()) {
      toast.error("Tracking number is required");
      return;
    }
    setSavingTracking(true);
    try {
      await adminOrders.updateTracking(params.id, trackingData);
      setOrder(prev => ({
        ...prev,
        tracking_number: trackingData.tracking_number,
        carrier: trackingData.carrier,
        estimated_delivery_date: trackingData.estimated_delivery,
        status: "shipped",
      }));
      setShowTrackingForm(false);
      toast.success("Tracking info saved and customer notified");
    } catch (error) {
      toast.error("Failed to update tracking info");
    } finally {
      setSavingTracking(false);
    }
  };

  if (loading) {
    return (
      <div className="order-detail-page">
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

  if (!order) {
    return (
      <div className="order-detail-page">
        <div className="admin-page-header">
          <h1 className="page-title">Order Not Found</h1>
        </div>
      </div>
    );
  }

  return (
    <div className="order-detail-page">
      <div className="admin-page-header">
        <div style={{ display: "flex", alignItems: "center", gap: 16 }}>
          <button
            className="admin-btn btn-ghost btn-icon"
            onClick={() => router.back()}
          >
            <ArrowLeftIcon />
          </button>
          <div>
            <h1 className="page-title" style={{ marginBottom: 4 }}>
              {order.order_number}
            </h1>
            <div style={{ display: "flex", gap: 8 }}>
              <StatusBadge status={order.status} type="order" />
              <StatusBadge status={order.payment_status} type="payment" />
            </div>
          </div>
        </div>
        <div className="page-actions">
          <button className="admin-btn btn-secondary" onClick={() => window.print()}>
            <PrintIcon />
            Print
          </button>
        </div>
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "2fr 1fr", gap: 24 }}>
        {/* Left Column */}
        <div>
          {/* Order Items */}
          <div className="admin-card" style={{ marginBottom: 24 }}>
            <div className="card-header">
              <h3 className="card-title">Order Items</h3>
            </div>
            <div className="card-body" style={{ padding: 0 }}>
              <table className="admin-table">
                <thead>
                  <tr>
                    <th>Product</th>
                    <th style={{ width: 80 }}>Qty</th>
                    <th style={{ width: 120, textAlign: "right" }}>Price</th>
                    <th style={{ width: 120, textAlign: "right" }}>Total</th>
                  </tr>
                </thead>
                <tbody>
                  {order.items.map(item => (
                    <tr key={item.id}>
                      <td>
                        <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
                          <div style={{ width: 48, height: 48, borderRadius: 8, backgroundColor: "#f3f4f6", overflow: "hidden" }}>
                            {item.image && <img src={item.image} alt={item.name} style={{ width: "100%", height: "100%", objectFit: "cover" }} />}
                          </div>
                          <div>
                            <div style={{ fontWeight: 500 }}>{item.name}</div>
                            <div style={{ fontSize: 12, color: "#6b7280" }}>{item.sku}</div>
                          </div>
                        </div>
                      </td>
                      <td>{item.quantity}</td>
                      <td style={{ textAlign: "right" }}>{formatCurrency(item.unit_price)}</td>
                      <td style={{ textAlign: "right", fontWeight: 600 }}>
                        {formatCurrency(item.unit_price * item.quantity)}
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
            <div className="card-footer">
              <div style={{ display: "flex", flexDirection: "column", alignItems: "flex-end", gap: 8 }}>
                <div style={{ display: "flex", justifyContent: "space-between", width: 200 }}>
                  <span>Subtotal:</span>
                  <span>{formatCurrency(order.subtotal)}</span>
                </div>
                <div style={{ display: "flex", justifyContent: "space-between", width: 200 }}>
                  <span>Shipping:</span>
                  <span>{formatCurrency(order.shipping)}</span>
                </div>
                {order.discount > 0 && (
                  <div style={{ display: "flex", justifyContent: "space-between", width: 200, color: "var(--admin-success)" }}>
                    <span>Discount:</span>
                    <span>-{formatCurrency(order.discount)}</span>
                  </div>
                )}
                <div style={{ display: "flex", justifyContent: "space-between", width: 200, fontWeight: 700, fontSize: 18, paddingTop: 8, borderTop: "1px solid var(--admin-border)" }}>
                  <span>Total:</span>
                  <span>{formatCurrency(order.total)}</span>
                </div>
              </div>
            </div>
          </div>

          {/* Timeline */}
          <div className="admin-card">
            <div className="card-header">
              <h3 className="card-title">Order Timeline</h3>
            </div>
            <div className="card-body">
              <OrderTimeline events={order.timeline} />
            </div>
          </div>
        </div>

        {/* Right Column */}
        <div>
          {/* Update Status */}
          <div className="admin-card" style={{ marginBottom: 24 }}>
            <div className="card-header">
              <h3 className="card-title">Update Status</h3>
            </div>
            <div className="card-body">
              <select
                className="admin-select"
                value={order.status}
                onChange={(e) => handleStatusChange(e.target.value)}
                disabled={updatingStatus}
              >
                {orderStatuses.map(s => (
                  <option key={s.value} value={s.value}>{s.label}</option>
                ))}
              </select>
            </div>
          </div>

          {/* Tracking Info */}
          <div className="admin-card" style={{ marginBottom: 24 }}>
            <div className="card-header" style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
              <h3 className="card-title" style={{ display: "flex", alignItems: "center", gap: 8 }}>
                <TruckIcon /> Tracking
              </h3>
            </div>
            <div className="card-body">
              {order.tracking_number ? (
                <div>
                  <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
                    <span style={{ color: "#6b7280", fontSize: 13 }}>Carrier:</span>
                    <span style={{ fontWeight: 500, textTransform: "capitalize" }}>{order.carrier || "-"}</span>
                  </div>
                  <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
                    <span style={{ color: "#6b7280", fontSize: 13 }}>Tracking #:</span>
                    <span style={{ fontWeight: 500, fontFamily: "monospace", fontSize: 13 }}>{order.tracking_number}</span>
                  </div>
                  {order.estimated_delivery_date && (
                    <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
                      <span style={{ color: "#6b7280", fontSize: 13 }}>Est. Delivery:</span>
                      <span style={{ fontWeight: 500 }}>{order.estimated_delivery_date}</span>
                    </div>
                  )}
                  <button
                    className="admin-btn btn-secondary btn-sm btn-block"
                    style={{ marginTop: 8 }}
                    onClick={() => {
                      setTrackingData({
                        tracking_number: order.tracking_number || "",
                        carrier: order.carrier || "correios",
                        estimated_delivery: order.estimated_delivery_date || "",
                      });
                      setShowTrackingForm(true);
                    }}
                  >
                    Update Tracking
                  </button>
                </div>
              ) : !showTrackingForm ? (
                <button
                  className="admin-btn btn-primary btn-block"
                  onClick={() => setShowTrackingForm(true)}
                >
                  <TruckIcon /> Add Tracking Number
                </button>
              ) : null}

              {showTrackingForm && (
                <form onSubmit={handleTrackingSubmit}>
                  <div style={{ marginBottom: 12 }}>
                    <label style={{ display: "block", fontSize: 13, fontWeight: 500, marginBottom: 4, color: "#374151" }}>
                      Carrier
                    </label>
                    <select
                      className="admin-select"
                      value={trackingData.carrier}
                      onChange={(e) => setTrackingData(prev => ({ ...prev, carrier: e.target.value }))}
                    >
                      {carriers.map(c => (
                        <option key={c.value} value={c.value}>{c.label}</option>
                      ))}
                    </select>
                  </div>
                  <div style={{ marginBottom: 12 }}>
                    <label style={{ display: "block", fontSize: 13, fontWeight: 500, marginBottom: 4, color: "#374151" }}>
                      Tracking Number *
                    </label>
                    <input
                      type="text"
                      className="form-control"
                      placeholder="e.g. BR123456789BR"
                      value={trackingData.tracking_number}
                      onChange={(e) => setTrackingData(prev => ({ ...prev, tracking_number: e.target.value }))}
                      required
                    />
                  </div>
                  <div style={{ marginBottom: 16 }}>
                    <label style={{ display: "block", fontSize: 13, fontWeight: 500, marginBottom: 4, color: "#374151" }}>
                      Est. Delivery Date
                    </label>
                    <input
                      type="date"
                      className="form-control"
                      value={trackingData.estimated_delivery}
                      onChange={(e) => setTrackingData(prev => ({ ...prev, estimated_delivery: e.target.value }))}
                    />
                  </div>
                  <div style={{ display: "flex", gap: 8 }}>
                    <button
                      type="submit"
                      className="admin-btn btn-primary btn-sm"
                      disabled={savingTracking}
                      style={{ flex: 1 }}
                    >
                      {savingTracking ? "Saving..." : "Save & Notify"}
                    </button>
                    <button
                      type="button"
                      className="admin-btn btn-ghost btn-sm"
                      onClick={() => setShowTrackingForm(false)}
                    >
                      Cancel
                    </button>
                  </div>
                </form>
              )}
            </div>
          </div>

          {/* Customer Info */}
          <div className="admin-card" style={{ marginBottom: 24 }}>
            <div className="card-header">
              <h3 className="card-title">Customer</h3>
            </div>
            <div className="card-body">
              <div style={{ marginBottom: 16 }}>
                <div style={{ fontWeight: 600, marginBottom: 4 }}>{order.customer.name}</div>
                <div style={{ fontSize: 14, color: "#6b7280" }}>{order.customer.email}</div>
                <div style={{ fontSize: 14, color: "#6b7280" }}>{order.customer.phone}</div>
              </div>
              <Link
                href={`/admin/customers/${order.customer.id}`}
                className="admin-btn btn-secondary btn-sm btn-block"
              >
                View Customer
              </Link>
            </div>
          </div>

          {/* Shipping Address */}
          <div className="admin-card" style={{ marginBottom: 24 }}>
            <div className="card-header">
              <h3 className="card-title">Shipping Address</h3>
            </div>
            <div className="card-body">
              <div style={{ fontSize: 14, lineHeight: 1.6 }}>
                {order.shipping_address.street}<br />
                {order.shipping_address.neighborhood}<br />
                {order.shipping_address.city}, {order.shipping_address.state}<br />
                {order.shipping_address.postal_code}
              </div>
            </div>
          </div>

          {/* Payment Info */}
          <div className="admin-card">
            <div className="card-header">
              <h3 className="card-title">Payment</h3>
            </div>
            <div className="card-body">
              <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
                <span style={{ color: "#6b7280" }}>Method:</span>
                <span style={{ fontWeight: 500, textTransform: "capitalize" }}>
                  {order.payment_method.replace("_", " ")}
                </span>
              </div>
              <div style={{ display: "flex", justifyContent: "space-between" }}>
                <span style={{ color: "#6b7280" }}>Status:</span>
                <StatusBadge status={order.payment_status} type="payment" />
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
