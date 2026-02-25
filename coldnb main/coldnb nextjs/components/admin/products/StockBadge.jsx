"use client";

export default function StockBadge({ quantity, threshold = 10 }) {
  let status, label;

  if (quantity <= 0) {
    status = "stock-out";
    label = "Out of Stock";
  } else if (quantity <= threshold) {
    status = "stock-low";
    label = `${quantity} left`;
  } else {
    status = "stock-in";
    label = "In Stock";
  }

  return <span className={`stock-badge ${status}`}>{label}</span>;
}
