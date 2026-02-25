"use client";

export default function StatusBadge({ status, type = "order" }) {
  const orderClasses = {
    pending: "status-pending",
    processing: "status-processing",
    shipped: "status-shipped",
    delivered: "status-delivered",
    cancelled: "status-cancelled",
    refunded: "status-refunded",
  };

  const paymentClasses = {
    paid: "payment-paid",
    pending: "payment-pending",
    failed: "payment-failed",
    refunded: "payment-refunded",
  };

  const classes = type === "payment" ? paymentClasses : orderClasses;
  const baseClass = type === "payment" ? "payment-badge" : "status-badge";

  const formatLabel = (s) => s.charAt(0).toUpperCase() + s.slice(1);

  return (
    <span className={`${baseClass} ${classes[status] || ""}`}>
      {type === "order" && <span className="status-dot" />}
      {formatLabel(status)}
    </span>
  );
}
