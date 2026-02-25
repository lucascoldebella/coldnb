"use client";
import Link from "next/link";

export default function ListCard({
  title,
  actionLabel,
  actionHref,
  items = [],
  renderItem,
  emptyMessage = "No items to display",
  loading = false,
}) {
  if (loading) {
    return (
      <div className="list-card">
        <div className="list-header">
          <div className="skeleton skeleton-title" style={{ width: 100 }} />
        </div>
        <div className="list-body">
          {[1, 2, 3, 4, 5].map((i) => (
            <div key={i} className="list-item">
              <div className="skeleton skeleton-avatar" />
              <div className="item-content" style={{ flex: 1 }}>
                <div className="skeleton skeleton-text" style={{ width: "70%" }} />
                <div className="skeleton skeleton-text" style={{ width: "40%", marginTop: 4 }} />
              </div>
            </div>
          ))}
        </div>
      </div>
    );
  }

  return (
    <div className="list-card">
      <div className="list-header">
        <h3 className="list-title">{title}</h3>
        {actionLabel && actionHref && (
          <Link href={actionHref} className="list-action">
            {actionLabel}
          </Link>
        )}
      </div>
      <div className="list-body">
        {items.length === 0 ? (
          <div className="list-empty">{emptyMessage}</div>
        ) : (
          items.map((item, index) => (
            <div key={item.id || index} className="list-item">
              {renderItem(item)}
            </div>
          ))
        )}
      </div>
    </div>
  );
}
