"use client";
import { useState } from "react";

export default function HomepageSectionCard({
  number,
  title,
  subtitle,
  badge,
  badgeType = "dynamic",
  collapsible = false,
  defaultOpen = true,
  children,
}) {
  const [open, setOpen] = useState(defaultOpen);

  return (
    <div className="admin-card" style={{ marginBottom: 16 }}>
      <div
        style={{
          display: "flex",
          alignItems: "center",
          gap: 12,
          padding: "14px 16px",
          borderBottom: open ? "1px solid var(--admin-border)" : "none",
          cursor: collapsible ? "pointer" : "default",
        }}
        onClick={collapsible ? () => setOpen(!open) : undefined}
      >
        {number != null && (
          <span
            style={{
              width: 28,
              height: 28,
              borderRadius: "50%",
              background: "var(--admin-primary)",
              color: "#fff",
              display: "flex",
              alignItems: "center",
              justifyContent: "center",
              fontSize: 13,
              fontWeight: 600,
              flexShrink: 0,
            }}
          >
            {number}
          </span>
        )}
        <div style={{ flex: 1 }}>
          <div style={{ fontWeight: 600, fontSize: 15 }}>{title}</div>
          {subtitle && (
            <div style={{ fontSize: 12, color: "var(--admin-text-secondary)", marginTop: 2 }}>
              {subtitle}
            </div>
          )}
        </div>
        {badge && (
          <span
            className={`admin-badge ${badgeType === "dynamic" ? "badge-success" : "badge-secondary"}`}
            style={{ fontSize: 11 }}
          >
            {badge}
          </span>
        )}
        {collapsible && (
          <span
            style={{
              fontSize: 18,
              color: "var(--admin-text-secondary)",
              transform: open ? "rotate(180deg)" : "rotate(0deg)",
              transition: "transform 0.2s",
            }}
          >
            &#9660;
          </span>
        )}
      </div>
      {open && <div style={{ padding: 16 }}>{children}</div>}
    </div>
  );
}
