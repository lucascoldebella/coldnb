"use client";
import { useState, useEffect } from "react";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";

const STORAGE_KEY = "coldnb-cookie-consent";

export default function CookieConsent() {
  const { t } = useLanguage();
  const [visible, setVisible] = useState(false);

  useEffect(() => {
    const consent = localStorage.getItem(STORAGE_KEY);
    if (!consent) {
      setVisible(true);
    }
  }, []);

  const accept = () => {
    localStorage.setItem(STORAGE_KEY, "accepted");
    setVisible(false);
  };

  const decline = () => {
    localStorage.setItem(STORAGE_KEY, "declined");
    setVisible(false);
  };

  if (!visible) return null;

  return (
    <div
      style={{
        position: "fixed",
        bottom: 0,
        left: 0,
        right: 0,
        zIndex: 9999,
        background: "var(--bg-main, #fff)",
        borderTop: "1px solid var(--line, #e5e5e5)",
        padding: "16px 0",
        boxShadow: "0 -2px 10px rgba(0,0,0,0.08)",
      }}
    >
      <div className="container">
        <div className="d-flex flex-column flex-md-row align-items-center justify-content-between gap-3">
          <p className="text-caption-1 mb-0" style={{ maxWidth: 700 }}>
            {t("cookie.message")}{" "}
            <Link href="/term-of-use" className="fw-6 link">
              {t("cookie.learnMore")}
            </Link>
          </p>
          <div className="d-flex gap-2 flex-shrink-0">
            <button
              className="tf-btn btn-fill radius-4"
              onClick={accept}
              style={{ minWidth: 100 }}
            >
              <span className="text text-button">{t("cookie.accept")}</span>
            </button>
            <button
              className="tf-btn btn-outline radius-4"
              onClick={decline}
              style={{ minWidth: 100 }}
            >
              <span className="text text-button">{t("cookie.decline")}</span>
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}
