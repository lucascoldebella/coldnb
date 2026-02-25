// Shared admin utilities — centralized formatting functions

export const formatCurrency = (value) => {
  return new Intl.NumberFormat("pt-BR", {
    style: "currency",
    currency: "BRL",
  }).format(value || 0);
};

export const formatDate = (dateString) => {
  if (!dateString) return "—";
  return new Date(dateString).toLocaleDateString("pt-BR", {
    day: "2-digit",
    month: "short",
    year: "numeric",
  });
};

export const formatDateTime = (dateString) => {
  if (!dateString) return "—";
  return new Date(dateString).toLocaleDateString("pt-BR", {
    day: "2-digit",
    month: "short",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
  });
};

export const formatShortDateTime = (dateString) => {
  if (!dateString) return "—";
  return new Date(dateString).toLocaleDateString("pt-BR", {
    day: "2-digit",
    month: "short",
    hour: "2-digit",
    minute: "2-digit",
  });
};

// CSV export utility
export const exportToCSV = (data, columns, filename) => {
  if (!data || data.length === 0) return;

  const headers = columns.map((c) => c.header);
  const rows = data.map((row) =>
    columns.map((c) => {
      const val = c.exportValue ? c.exportValue(row) : row[c.accessor];
      // Escape CSV values
      const str = String(val ?? "");
      return str.includes(",") || str.includes('"') || str.includes("\n")
        ? `"${str.replace(/"/g, '""')}"`
        : str;
    })
  );

  const csv = [headers.join(","), ...rows.map((r) => r.join(","))].join("\n");
  const blob = new Blob(["\uFEFF" + csv], { type: "text/csv;charset=utf-8;" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  const date = new Date().toISOString().slice(0, 10);
  link.download = `${filename}-${date}.csv`;
  link.click();
  URL.revokeObjectURL(url);
};

// File validation for image uploads
const ALLOWED_IMAGE_TYPES = [
  "image/jpeg",
  "image/png",
  "image/gif",
  "image/webp",
];
const MAX_FILE_SIZE = 5 * 1024 * 1024; // 5MB

export const validateImageFiles = (files) => {
  const errors = [];
  const validFiles = [];

  for (const file of Array.from(files)) {
    if (!ALLOWED_IMAGE_TYPES.includes(file.type)) {
      errors.push(
        `"${file.name}" is not a supported format. Use JPEG, PNG, GIF, or WebP.`
      );
      continue;
    }
    if (file.size > MAX_FILE_SIZE) {
      errors.push(
        `"${file.name}" exceeds 5MB (${(file.size / 1024 / 1024).toFixed(1)}MB).`
      );
      continue;
    }
    validFiles.push(file);
  }

  return { validFiles, errors };
};
