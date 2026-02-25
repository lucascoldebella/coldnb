"use client";

export default function LoadingSpinner({ size = "md", className = "" }) {
  const sizes = {
    sm: 16,
    md: 24,
    lg: 40,
    xl: 56,
  };

  const dimension = sizes[size] || sizes.md;

  return (
    <div
      className={`loading-spinner ${className}`}
      style={{
        width: dimension,
        height: dimension,
        border: `${Math.max(2, dimension / 10)}px solid var(--admin-border)`,
        borderTopColor: "var(--admin-primary)",
        borderRadius: "50%",
        animation: "spin 0.8s linear infinite",
      }}
    />
  );
}
