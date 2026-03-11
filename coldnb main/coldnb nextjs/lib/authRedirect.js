const DEFAULT_AUTH_PATH = "/my-account";

export function sanitizeNextPath(value, fallback = DEFAULT_AUTH_PATH) {
  if (typeof value !== "string") {
    return fallback;
  }

  const trimmed = value.trim();
  if (!trimmed || !trimmed.startsWith("/") || trimmed.startsWith("//")) {
    return fallback;
  }

  return trimmed;
}

export function appendNextPath(basePath, nextPath) {
  const safeBasePath = sanitizeNextPath(basePath, DEFAULT_AUTH_PATH);
  const safeNextPath = sanitizeNextPath(nextPath, "");

  if (!safeNextPath) {
    return safeBasePath;
  }

  const url = new URL(safeBasePath, "https://coldnb.local");
  url.searchParams.set("next", safeNextPath);
  return `${url.pathname}${url.search}${url.hash}`;
}

export function getSameOriginReferrerPath(fallback = "") {
  if (typeof window === "undefined" || typeof document === "undefined") {
    return fallback;
  }

  if (!document.referrer) {
    return fallback;
  }

  try {
    const referrerUrl = new URL(document.referrer);

    if (referrerUrl.origin !== window.location.origin) {
      return fallback;
    }

    const candidate = `${referrerUrl.pathname}${referrerUrl.search}${referrerUrl.hash}`;
    if (candidate === "/login" || candidate.startsWith("/auth/")) {
      return fallback;
    }

    return sanitizeNextPath(candidate, fallback);
  } catch {
    return fallback;
  }
}
