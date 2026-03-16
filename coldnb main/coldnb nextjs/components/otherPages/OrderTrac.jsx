"use client";
import React, { useState } from "react";
import Image from "next/image";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { getApiBaseUrl } from "@/lib/apiBase";

const statusSteps = ["pending", "confirmed", "processing", "shipped", "delivered"];

function getCarrierUrl(carrier, trackingNumber) {
  if (!carrier || !trackingNumber) return null;
  const c = carrier.toLowerCase();
  if (c === "correios" || c === "sedex" || c === "pac") {
    return `https://www.linkcorreios.com.br/?id=${trackingNumber}`;
  }
  if (c === "dhl") {
    return `https://www.dhl.com/br-pt/home/tracking.html?tracking-id=${trackingNumber}`;
  }
  return null;
}

export default function OrderTrac() {
  const { t } = useLanguage();
  const [orderNumber, setOrderNumber] = useState("");
  const [email, setEmail] = useState("");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const [order, setOrder] = useState(null);

  const handleTrack = async (e) => {
    e.preventDefault();
    if (!orderNumber.trim() || !email.trim()) {
      setError(t("orderTracking.fillAllFields") || "Please fill in all fields.");
      return;
    }

    setLoading(true);
    setError("");
    setOrder(null);

    try {
      const baseUrl = getApiBaseUrl();
      const res = await fetch(
        `${baseUrl}/api/track-order?order_number=${encodeURIComponent(orderNumber.trim())}&email=${encodeURIComponent(email.trim())}`
      );
      const data = await res.json();

      if (!res.ok) {
        setError(data.error || t("orderTracking.notFound") || "Order not found.");
        return;
      }

      setOrder(data.data || data);
    } catch {
      setError(t("orderTracking.error") || "Something went wrong. Please try again.");
    } finally {
      setLoading(false);
    }
  };

  const currentStepIndex = order ? statusSteps.indexOf(order.status) : -1;
  const carrierUrl = order ? getCarrierUrl(order.carrier, order.tracking_number) : null;

  return (
    <section className="flat-spacing">
      <div className="container">
        {!order ? (
          <div className="login-wrap tracking-wrap">
            <div className="left">
              <div className="heading">
                <h4 className="mb_8">{t("orderTracking.title")}</h4>
                <p>{t("orderTracking.description")}</p>
              </div>
              <form onSubmit={handleTrack} className="form-login">
                <div className="wrap">
                  <fieldset>
                    <input
                      type="text"
                      placeholder={t("orderTracking.orderId")}
                      value={orderNumber}
                      onChange={(e) => setOrderNumber(e.target.value)}
                    />
                  </fieldset>
                  <fieldset>
                    <input
                      type="email"
                      placeholder={t("orderTracking.billingEmail")}
                      value={email}
                      onChange={(e) => setEmail(e.target.value)}
                    />
                  </fieldset>
                </div>
                {error && (
                  <div style={{ color: "#dc2626", marginBottom: 16, fontSize: 14 }}>
                    {error}
                  </div>
                )}
                <div className="button-submit">
                  <button
                    className="tf-btn btn-fill"
                    type="submit"
                    disabled={loading}
                  >
                    <span className="text">
                      {loading
                        ? (t("orderTracking.searching") || "Searching...")
                        : t("orderTracking.trackingOrders")}
                    </span>
                  </button>
                </div>
              </form>
            </div>
            <div className="right">
              <h4 className="mb_8">{t("orderTracking.alreadyHaveAccount")}</h4>
              <p className="text-secondary">
                {t("orderTracking.welcomeBack")}
              </p>
              <Link href="/login" className="tf-btn btn-fill">
                <span className="text">{t("login.login")}</span>
              </Link>
            </div>
          </div>
        ) : (
          <div className="tracking-result">
            <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 32 }}>
              <div>
                <h4 style={{ marginBottom: 4 }}>
                  {t("orderTracking.orderLabel") || "Order"} {order.order_number}
                </h4>
                <p style={{ color: "#6b7280", fontSize: 14 }}>
                  {t("orderTracking.placedOn") || "Placed on"}{" "}
                  {new Date(order.created_at).toLocaleDateString("pt-BR", {
                    day: "2-digit",
                    month: "long",
                    year: "numeric",
                  })}
                </p>
              </div>
              <button
                className="tf-btn btn-outline"
                onClick={() => { setOrder(null); setOrderNumber(""); setEmail(""); }}
              >
                <span className="text">{t("orderTracking.trackAnother") || "Track Another Order"}</span>
              </button>
            </div>

            {/* Status Timeline */}
            <div className="admin-card" style={{ marginBottom: 24, borderRadius: 12, border: "1px solid #e5e7eb", boxShadow: "0 1px 3px rgba(0,0,0,0.06)" }}>
              <div style={{ padding: "24px 24px 8px" }}>
                <h5 style={{ marginBottom: 24 }}>{t("orderTracking.statusTitle") || "Order Status"}</h5>
                <div style={{ display: "flex", justifyContent: "space-between", position: "relative", padding: "0 16px" }}>
                  {/* Progress bar background */}
                  <div style={{ position: "absolute", top: 14, left: 44, right: 44, height: 4, backgroundColor: "#e5e7eb", borderRadius: 2 }} />
                  {/* Progress bar fill */}
                  <div style={{
                    position: "absolute", top: 14, left: 44, height: 4, borderRadius: 2,
                    backgroundColor: order.status === "cancelled" ? "#dc2626" : "#10b981",
                    width: order.status === "cancelled" ? "100%" : `${Math.max(0, currentStepIndex) / (statusSteps.length - 1) * (100 - 16)}%`,
                    transition: "width 0.5s ease"
                  }} />
                  {statusSteps.map((step, i) => {
                    const isCompleted = i <= currentStepIndex;
                    const isCurrent = i === currentStepIndex;
                    const isCancelled = order.status === "cancelled";
                    return (
                      <div key={step} style={{ display: "flex", flexDirection: "column", alignItems: "center", zIndex: 1, flex: 1 }}>
                        <div style={{
                          width: 32, height: 32, borderRadius: "50%",
                          display: "flex", alignItems: "center", justifyContent: "center",
                          fontSize: 14, fontWeight: 600,
                          backgroundColor: isCancelled ? "#fef2f2" :
                            isCompleted ? "#10b981" : "#e5e7eb",
                          color: isCancelled ? "#dc2626" :
                            isCompleted ? "#fff" : "#9ca3af",
                          border: isCurrent && !isCancelled ? "3px solid #6ee7b7" : "none",
                          transition: "all 0.3s ease"
                        }}>
                          {isCompleted && !isCancelled ? "✓" : (i + 1)}
                        </div>
                        <span style={{
                          marginTop: 8, fontSize: 12, fontWeight: isCurrent ? 600 : 400,
                          color: isCancelled ? "#dc2626" :
                            isCompleted ? "#111827" : "#9ca3af",
                          textTransform: "capitalize"
                        }}>
                          {t(`orderTracking.status.${step}`) || step}
                        </span>
                      </div>
                    );
                  })}
                </div>
                {order.status === "cancelled" && (
                  <div style={{ textAlign: "center", marginTop: 16, padding: "8px 16px", backgroundColor: "#fef2f2", borderRadius: 8, color: "#dc2626", fontSize: 14, fontWeight: 500 }}>
                    {t("orderTracking.cancelled") || "This order has been cancelled."}
                  </div>
                )}
              </div>
            </div>

            <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 24 }}>
              {/* Tracking Info */}
              <div style={{ borderRadius: 12, border: "1px solid #e5e7eb", padding: 24, boxShadow: "0 1px 3px rgba(0,0,0,0.06)" }}>
                <h5 style={{ marginBottom: 16 }}>{t("orderTracking.trackingInfo") || "Tracking Information"}</h5>
                {order.tracking_number ? (
                  <div>
                    <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 12 }}>
                      <span style={{ color: "#6b7280" }}>{t("orderTracking.carrier") || "Carrier"}:</span>
                      <span style={{ fontWeight: 500, textTransform: "capitalize" }}>{order.carrier || "-"}</span>
                    </div>
                    <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 12 }}>
                      <span style={{ color: "#6b7280" }}>{t("orderTracking.trackingNumber") || "Tracking Number"}:</span>
                      <span style={{ fontWeight: 500, fontFamily: "monospace" }}>{order.tracking_number}</span>
                    </div>
                    {order.estimated_delivery_date && (
                      <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 12 }}>
                        <span style={{ color: "#6b7280" }}>{t("orderTracking.estimatedDelivery") || "Estimated Delivery"}:</span>
                        <span style={{ fontWeight: 500 }}>
                          {new Date(order.estimated_delivery_date + "T00:00:00").toLocaleDateString("pt-BR", {
                            day: "2-digit", month: "long", year: "numeric"
                          })}
                        </span>
                      </div>
                    )}
                    {carrierUrl && (
                      <a
                        href={carrierUrl}
                        target="_blank"
                        rel="noopener noreferrer"
                        className="tf-btn btn-fill"
                        style={{ width: "100%", textAlign: "center", marginTop: 8 }}
                      >
                        <span className="text">{t("orderTracking.trackPackage") || "Track Package"}</span>
                      </a>
                    )}
                  </div>
                ) : (
                  <p style={{ color: "#9ca3af", fontStyle: "italic" }}>
                    {t("orderTracking.noTracking") || "Tracking information will be available once your order is shipped."}
                  </p>
                )}
              </div>

              {/* Order Summary */}
              <div style={{ borderRadius: 12, border: "1px solid #e5e7eb", padding: 24, boxShadow: "0 1px 3px rgba(0,0,0,0.06)" }}>
                <h5 style={{ marginBottom: 16 }}>{t("orderTracking.orderSummary") || "Order Summary"}</h5>
                <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
                  <span style={{ color: "#6b7280" }}>{t("orderTracking.items") || "Items"}:</span>
                  <span>{order.item_count}</span>
                </div>
                <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
                  <span style={{ color: "#6b7280" }}>Subtotal:</span>
                  <span>R$ {parseFloat(order.subtotal).toFixed(2)}</span>
                </div>
                {parseFloat(order.shipping_cost) > 0 && (
                  <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
                    <span style={{ color: "#6b7280" }}>{t("orderTracking.shipping") || "Shipping"}:</span>
                    <span>R$ {parseFloat(order.shipping_cost).toFixed(2)}</span>
                  </div>
                )}
                {parseFloat(order.discount_amount) > 0 && (
                  <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 8, color: "#10b981" }}>
                    <span>{t("orderTracking.discount") || "Discount"}:</span>
                    <span>-R$ {parseFloat(order.discount_amount).toFixed(2)}</span>
                  </div>
                )}
                <div style={{ display: "flex", justifyContent: "space-between", paddingTop: 12, borderTop: "1px solid #e5e7eb", fontWeight: 700, fontSize: 18 }}>
                  <span>Total:</span>
                  <span>R$ {parseFloat(order.total).toFixed(2)}</span>
                </div>
                <div style={{ marginTop: 12, fontSize: 13, color: "#6b7280" }}>
                  {t("orderTracking.destination") || "Destination"}: {order.shipping_city}, {order.shipping_state}
                </div>
              </div>
            </div>

            {/* Order Items */}
            {order.items && order.items.length > 0 && (
              <div style={{ marginTop: 24, borderRadius: 12, border: "1px solid #e5e7eb", padding: 24, boxShadow: "0 1px 3px rgba(0,0,0,0.06)" }}>
                <h5 style={{ marginBottom: 16 }}>{t("orderTracking.orderItems") || "Order Items"}</h5>
                {order.items.map((item, i) => (
                  <div key={i} style={{
                    display: "flex", alignItems: "center", gap: 16, padding: "12px 0",
                    borderBottom: i < order.items.length - 1 ? "1px solid #f3f4f6" : "none"
                  }}>
                    <div style={{ width: 56, height: 56, borderRadius: 8, backgroundColor: "#f3f4f6", overflow: "hidden", flexShrink: 0 }}>
                      {item.product_image && (
                        <Image src={item.product_image} alt={item.product_name} width={56} height={56} style={{ width: "100%", height: "100%", objectFit: "cover" }} />
                      )}
                    </div>
                    <div style={{ flex: 1 }}>
                      <div style={{ fontWeight: 500 }}>{item.product_name}</div>
                      <div style={{ fontSize: 13, color: "#6b7280" }}>
                        {t("orderTracking.qty") || "Qty"}: {item.quantity}
                      </div>
                    </div>
                    <div style={{ fontWeight: 600 }}>
                      R$ {parseFloat(item.total_price).toFixed(2)}
                    </div>
                  </div>
                ))}
              </div>
            )}

            {/* Status History */}
            {order.history && order.history.length > 0 && (
              <div style={{ marginTop: 24, borderRadius: 12, border: "1px solid #e5e7eb", padding: 24, boxShadow: "0 1px 3px rgba(0,0,0,0.06)" }}>
                <h5 style={{ marginBottom: 16 }}>{t("orderTracking.history") || "Status History"}</h5>
                {order.history.map((entry, i) => (
                  <div key={i} style={{
                    display: "flex", alignItems: "center", gap: 12, padding: "8px 0",
                    borderBottom: i < order.history.length - 1 ? "1px solid #f3f4f6" : "none"
                  }}>
                    <div style={{
                      width: 8, height: 8, borderRadius: "50%",
                      backgroundColor: i === order.history.length - 1 ? "#10b981" : "#d1d5db"
                    }} />
                    <span style={{ fontWeight: 500, textTransform: "capitalize", minWidth: 100 }}>
                      {t(`orderTracking.status.${entry.status}`) || entry.status}
                    </span>
                    <span style={{ color: "#9ca3af", fontSize: 13 }}>
                      {new Date(entry.created_at).toLocaleDateString("pt-BR", {
                        day: "2-digit", month: "short", year: "numeric", hour: "2-digit", minute: "2-digit"
                      })}
                    </span>
                  </div>
                ))}
              </div>
            )}
          </div>
        )}
      </div>
    </section>
  );
}
