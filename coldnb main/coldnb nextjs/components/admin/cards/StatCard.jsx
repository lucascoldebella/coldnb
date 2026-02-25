"use client";

const TrendUpIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="23 6 13.5 15.5 8.5 10.5 1 18" />
    <polyline points="17 6 23 6 23 12" />
  </svg>
);

const TrendDownIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="23 18 13.5 8.5 8.5 13.5 1 6" />
    <polyline points="17 18 23 18 23 12" />
  </svg>
);

export default function StatCard({
  icon,
  iconColor = "primary",
  value,
  label,
  trend,
  trendValue,
  period = "vs last month",
  loading = false,
}) {
  if (loading) {
    return (
      <div className="stat-card">
        <div className="stat-header">
          <div className="skeleton skeleton-avatar" style={{ width: 48, height: 48 }} />
        </div>
        <div className="stat-content">
          <div className="skeleton skeleton-title" style={{ width: "60%", height: 32 }} />
          <div className="skeleton skeleton-text" style={{ width: "40%", marginTop: 8 }} />
        </div>
      </div>
    );
  }

  return (
    <div className="stat-card">
      <div className="stat-header">
        <div className={`stat-icon icon-${iconColor}`}>
          {icon}
        </div>
      </div>

      <div className="stat-content">
        <div className="stat-value">{value}</div>
        <div className="stat-label">{label}</div>
      </div>

      {(trend !== undefined || trendValue !== undefined) && (
        <div className="stat-footer">
          <span className={`stat-trend ${trend === "up" ? "trend-up" : "trend-down"}`}>
            {trend === "up" ? <TrendUpIcon /> : <TrendDownIcon />}
            {trendValue}
          </span>
          <span className="stat-period">{period}</span>
        </div>
      )}
    </div>
  );
}
