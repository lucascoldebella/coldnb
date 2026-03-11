const LOCAL_API_HOSTS = new Set(["localhost", "127.0.0.1", "0.0.0.0"]);

function normalizeBaseUrl(value) {
  if (!value) return "";
  return value.endsWith("/") ? value.slice(0, -1) : value;
}

export function getApiBaseUrl() {
  const configuredBaseUrl = normalizeBaseUrl(process.env.NEXT_PUBLIC_API_URL);

  if (typeof window === "undefined") {
    return configuredBaseUrl || "http://localhost:8080";
  }

  if (!configuredBaseUrl) {
    return "";
  }

  try {
    const parsedUrl = new URL(configuredBaseUrl);
    const browserHost = window.location.hostname;

    if (LOCAL_API_HOSTS.has(parsedUrl.hostname) && !LOCAL_API_HOSTS.has(browserHost)) {
      return "";
    }

    return normalizeBaseUrl(parsedUrl.toString());
  } catch {
    if (configuredBaseUrl.startsWith("/")) {
      return configuredBaseUrl === "/" ? "" : configuredBaseUrl;
    }

    return configuredBaseUrl;
  }
}

export function buildApiUrl(path) {
  const normalizedPath = path.startsWith("/") ? path : `/${path}`;
  return `${getApiBaseUrl()}${normalizedPath}`;
}
