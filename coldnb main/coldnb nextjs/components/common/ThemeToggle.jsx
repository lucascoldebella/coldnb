"use client";
import { useTheme } from "@/context/ThemeContext";

export default function ThemeToggle({ light = false }) {
  const { theme, toggleTheme } = useTheme();
  const isDark = theme === "dark";

  return (
    <button
      onClick={toggleTheme}
      title={isDark ? "Switch to light mode" : "Switch to dark mode"}
      aria-label={isDark ? "Switch to light mode" : "Switch to dark mode"}
      style={{
        background: "none",
        border: "none",
        cursor: "pointer",
        padding: "4px",
        display: "inline-flex",
        alignItems: "center",
        gap: "6px",
      }}
    >
      {/* Track */}
      <span
        style={{
          position: "relative",
          width: "48px",
          height: "26px",
          borderRadius: "13px",
          background: isDark
            ? "linear-gradient(135deg, #1a1a3e, #2d2d5e)"
            : "linear-gradient(135deg, #87CEEB, #FDB813)",
          border: `2px solid ${isDark ? "#6366f1" : "#f59e0b"}`,
          display: "flex",
          alignItems: "center",
          transition: "all 0.4s ease",
          boxShadow: isDark
            ? "0 0 8px rgba(99, 102, 241, 0.4), inset 0 1px 3px rgba(0,0,0,0.3)"
            : "0 0 8px rgba(245, 158, 11, 0.3), inset 0 1px 3px rgba(0,0,0,0.1)",
          overflow: "hidden",
          flexShrink: 0,
        }}
      >
        {/* Sun icon (left side) */}
        <span
          style={{
            position: "absolute",
            left: "5px",
            top: "50%",
            transform: "translateY(-50%)",
            fontSize: "14px",
            lineHeight: 1,
            opacity: isDark ? 0.3 : 1,
            transition: "opacity 0.3s ease",
            zIndex: 1,
          }}
        >
          ☀️
        </span>

        {/* Moon icon (right side) */}
        <span
          style={{
            position: "absolute",
            right: "5px",
            top: "50%",
            transform: "translateY(-50%)",
            fontSize: "13px",
            lineHeight: 1,
            opacity: isDark ? 1 : 0.3,
            transition: "opacity 0.3s ease",
            zIndex: 1,
          }}
        >
          🌙
        </span>

        {/* Knob */}
        <span
          style={{
            position: "absolute",
            top: "2px",
            left: isDark ? "23px" : "2px",
            width: "20px",
            height: "20px",
            borderRadius: "50%",
            background: isDark ? "#e8e8ff" : "#ffffff",
            boxShadow: "0 2px 4px rgba(0,0,0,0.3)",
            transition: "left 0.4s cubic-bezier(0.68, -0.15, 0.27, 1.15)",
            zIndex: 2,
          }}
        />
      </span>
    </button>
  );
}
