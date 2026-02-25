/**
 * Apply Brazilian phone mask: (XX) XXXXX-XXXX
 */
export function maskPhone(value) {
  const digits = value.replace(/\D/g, "").slice(0, 11);
  if (digits.length === 0) return "";
  if (digits.length <= 2) return `(${digits}`;
  if (digits.length <= 7) return `(${digits.slice(0, 2)}) ${digits.slice(2)}`;
  return `(${digits.slice(0, 2)}) ${digits.slice(2, 7)}-${digits.slice(7)}`;
}

/**
 * Remove mask, return digits only
 */
export function unmaskPhone(value) {
  return value.replace(/\D/g, "");
}
