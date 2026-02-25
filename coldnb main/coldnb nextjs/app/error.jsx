"use client";

export default function Error({ error, reset }) {
  return (
    <div style={{ padding: "40px", textAlign: "center" }}>
      <h2>Something went wrong!</h2>
      <p>{error?.message || "An unexpected error occurred"}</p>
      <button
        onClick={() => reset()}
        style={{
          marginTop: "20px",
          padding: "10px 20px",
          cursor: "pointer",
          backgroundColor: "#333",
          color: "#fff",
          border: "none",
          borderRadius: "4px",
        }}
      >
        Try again
      </button>
    </div>
  );
}
