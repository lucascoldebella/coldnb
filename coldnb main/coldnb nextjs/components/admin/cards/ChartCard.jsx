"use client";

export default function ChartCard({
  title,
  actions,
  children,
  loading = false,
  className = "",
}) {
  if (loading) {
    return (
      <div className={`chart-card ${className}`}>
        <div className="chart-header">
          <div className="skeleton skeleton-title" style={{ width: 120 }} />
        </div>
        <div className="chart-body">
          <div className="chart-loading">
            <div className="loading-spinner" />
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className={`chart-card ${className}`}>
      {title && (
        <div className="chart-header">
          <h3 className="chart-title">{title}</h3>
          {actions && <div className="chart-actions">{actions}</div>}
        </div>
      )}
      <div className="chart-body">{children}</div>
    </div>
  );
}
