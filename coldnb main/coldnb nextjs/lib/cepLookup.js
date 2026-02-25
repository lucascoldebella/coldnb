/**
 * Lookup Brazilian CEP (postal code) via ViaCEP API with BrasilAPI fallback.
 * Returns { street, neighborhood, city, state } or null on error.
 */
export async function lookupCep(cep) {
  const cleaned = cep.replace(/\D/g, "");
  if (cleaned.length !== 8) return null;

  // Try ViaCEP first
  try {
    const response = await fetch(`https://viacep.com.br/ws/${cleaned}/json/`);
    const data = await response.json();

    if (!data.erro) {
      return {
        street: data.logradouro || "",
        neighborhood: data.bairro || "",
        city: data.localidade || "",
        state: data.uf || "",
      };
    }
  } catch {
    // Fall through to BrasilAPI
  }

  // Fallback to BrasilAPI
  try {
    const response = await fetch(`https://brasilapi.com.br/api/cep/v1/${cleaned}`);
    if (!response.ok) return null;
    const data = await response.json();

    return {
      street: data.street || "",
      neighborhood: data.neighborhood || "",
      city: data.city || "",
      state: data.state || "",
    };
  } catch {
    return null;
  }
}

/**
 * Format CEP for display: 12345-678
 */
export function formatCep(value) {
  const digits = value.replace(/\D/g, "").slice(0, 8);
  if (digits.length > 5) {
    return `${digits.slice(0, 5)}-${digits.slice(5)}`;
  }
  return digits;
}
