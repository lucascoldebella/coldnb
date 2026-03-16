"use client";

import { useState, useEffect } from "react";
import { useParams } from "next/navigation";
import { ordersApi } from "@/lib/userApi";

export default function InvoicePage() {
  const params = useParams();
  const orderId = params.id;
  const [order, setOrder] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    if (!orderId) return;
    ordersApi
      .get(orderId)
      .then((res) => setOrder(res.data?.data || res.data))
      .catch(() => setOrder(null))
      .finally(() => setLoading(false));
  }, [orderId]);

  const formatCurrency = (value) => {
    if (value == null) return "R$ 0,00";
    return new Intl.NumberFormat("pt-BR", {
      style: "currency",
      currency: "BRL",
    }).format(value);
  };

  const formatDate = (dateStr) => {
    if (!dateStr) return "";
    return new Date(dateStr).toLocaleDateString("pt-BR", {
      year: "numeric",
      month: "long",
      day: "numeric",
    });
  };

  if (loading) {
    return (
      <div style={{ textAlign: "center", padding: 60 }}>
        <p>Loading invoice...</p>
      </div>
    );
  }

  if (!order) {
    return (
      <div style={{ textAlign: "center", padding: 60 }}>
        <h3>Order not found</h3>
        <p>Please check the order ID and try again.</p>
      </div>
    );
  }

  const items = order.items || [];
  const subtotal = items.reduce(
    (sum, item) => sum + parseFloat(item.price || 0) * (item.quantity || 1),
    0
  );

  return (
    <>
      <style jsx global>{`
        @media print {
          body * { visibility: hidden; }
          .invoice-container, .invoice-container * { visibility: visible; }
          .invoice-container { position: absolute; left: 0; top: 0; width: 100%; }
          .no-print { display: none !important; }
        }
        .invoice-container {
          max-width: 800px;
          margin: 0 auto;
          padding: 40px;
          font-family: 'Helvetica Neue', Arial, sans-serif;
          color: #1f2937;
          line-height: 1.6;
        }
        .invoice-header {
          display: flex;
          justify-content: space-between;
          align-items: flex-start;
          margin-bottom: 32px;
          padding-bottom: 24px;
          border-bottom: 2px solid #111827;
        }
        .invoice-logo { font-size: 28px; font-weight: 700; letter-spacing: 1px; }
        .invoice-meta { text-align: right; font-size: 14px; color: #6b7280; }
        .invoice-meta h2 { font-size: 24px; color: #111827; margin: 0 0 8px; }
        .invoice-addresses {
          display: flex;
          gap: 40px;
          margin-bottom: 32px;
        }
        .invoice-addresses > div { flex: 1; }
        .invoice-addresses h4 {
          font-size: 12px;
          text-transform: uppercase;
          letter-spacing: 1px;
          color: #6b7280;
          margin: 0 0 8px;
        }
        .invoice-table { width: 100%; border-collapse: collapse; margin-bottom: 24px; }
        .invoice-table th {
          text-align: left;
          padding: 10px 12px;
          font-size: 12px;
          text-transform: uppercase;
          letter-spacing: 0.5px;
          color: #6b7280;
          border-bottom: 2px solid #e5e7eb;
        }
        .invoice-table td {
          padding: 12px;
          border-bottom: 1px solid #f3f4f6;
          font-size: 14px;
        }
        .invoice-table .text-right { text-align: right; }
        .invoice-totals {
          display: flex;
          justify-content: flex-end;
          margin-bottom: 32px;
        }
        .invoice-totals table { width: 280px; }
        .invoice-totals td { padding: 6px 0; font-size: 14px; }
        .invoice-totals .total-row td {
          font-weight: 700;
          font-size: 18px;
          padding-top: 12px;
          border-top: 2px solid #111827;
        }
        .invoice-footer {
          text-align: center;
          font-size: 12px;
          color: #9ca3af;
          padding-top: 24px;
          border-top: 1px solid #e5e7eb;
        }
      `}</style>

      <div className="invoice-container">
        <div className="no-print" style={{ textAlign: "right", marginBottom: 20 }}>
          <button
            onClick={() => window.print()}
            style={{
              padding: "10px 24px",
              background: "#111827",
              color: "#fff",
              border: "none",
              borderRadius: 8,
              cursor: "pointer",
              fontWeight: 600,
            }}
          >
            Print / Save as PDF
          </button>
        </div>

        <div className="invoice-header">
          <div>
            <div className="invoice-logo">Coldnb</div>
            <div style={{ fontSize: 13, color: "#6b7280", marginTop: 4 }}>
              Jewelry &amp; Accessories
            </div>
          </div>
          <div className="invoice-meta">
            <h2>INVOICE</h2>
            <div>Order: <strong>{order.order_number}</strong></div>
            <div>Date: {formatDate(order.created_at)}</div>
            <div>Status: {order.status}</div>
          </div>
        </div>

        <div className="invoice-addresses">
          <div>
            <h4>Bill To</h4>
            <div><strong>{order.shipping_name || "Customer"}</strong></div>
            {order.shipping_street && <div>{order.shipping_street}</div>}
            {order.shipping_street_2 && <div>{order.shipping_street_2}</div>}
            {(order.shipping_city || order.shipping_state) && (
              <div>
                {order.shipping_city}
                {order.shipping_city && order.shipping_state ? ", " : ""}
                {order.shipping_state}
              </div>
            )}
            {order.shipping_zip && <div>CEP: {order.shipping_zip}</div>}
            {order.shipping_phone && <div>Phone: {order.shipping_phone}</div>}
          </div>
          <div>
            <h4>Payment</h4>
            <div>Method: {order.payment_method || "Pending"}</div>
            <div>Payment status: {order.payment_status || "pending"}</div>
          </div>
        </div>

        <table className="invoice-table">
          <thead>
            <tr>
              <th style={{ width: "45%" }}>Item</th>
              <th>SKU</th>
              <th className="text-right">Price</th>
              <th className="text-right">Qty</th>
              <th className="text-right">Total</th>
            </tr>
          </thead>
          <tbody>
            {items.map((item, i) => (
              <tr key={item.id || i}>
                <td>
                  <strong>{item.product_name || item.name}</strong>
                  {item.color_name && <span style={{ color: "#6b7280" }}> — {item.color_name}</span>}
                  {item.size_name && <span style={{ color: "#6b7280" }}> / {item.size_name}</span>}
                </td>
                <td style={{ color: "#6b7280" }}>{item.product_sku || item.sku || "-"}</td>
                <td className="text-right">{formatCurrency(item.price)}</td>
                <td className="text-right">{item.quantity}</td>
                <td className="text-right">
                  {formatCurrency(parseFloat(item.price || 0) * (item.quantity || 1))}
                </td>
              </tr>
            ))}
          </tbody>
        </table>

        <div className="invoice-totals">
          <table>
            <tbody>
              <tr>
                <td>Subtotal</td>
                <td className="text-right">{formatCurrency(subtotal)}</td>
              </tr>
              {order.shipping_cost != null && parseFloat(order.shipping_cost) > 0 && (
                <tr>
                  <td>Shipping</td>
                  <td className="text-right">{formatCurrency(order.shipping_cost)}</td>
                </tr>
              )}
              {order.discount_amount != null && parseFloat(order.discount_amount) > 0 && (
                <tr>
                  <td>Discount</td>
                  <td className="text-right" style={{ color: "#10b981" }}>
                    -{formatCurrency(order.discount_amount)}
                  </td>
                </tr>
              )}
              <tr className="total-row">
                <td>Total</td>
                <td className="text-right">{formatCurrency(order.total)}</td>
              </tr>
            </tbody>
          </table>
        </div>

        {order.customer_notes && (
          <div style={{ marginBottom: 24, padding: 16, background: "#f9fafb", borderRadius: 8 }}>
            <strong style={{ fontSize: 12, textTransform: "uppercase", color: "#6b7280" }}>
              Notes
            </strong>
            <p style={{ margin: "8px 0 0", fontSize: 14 }}>{order.customer_notes}</p>
          </div>
        )}

        <div className="invoice-footer">
          <p>Thank you for your purchase!</p>
          <p>Coldnb &mdash; Jewelry &amp; Accessories</p>
        </div>
      </div>
    </>
  );
}
