"use client";

import { Suspense, useEffect, useState } from "react";
import { useSearchParams } from "next/navigation";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { buildApiUrl } from "@/lib/apiBase";
import Link from "next/link";
import Footer1 from "@/components/footers/Footer1";
import Header1 from "@/components/headers/Header1";
import Topbar6 from "@/components/headers/Topbar6";

export default function UnsubscribePage() {
  return (
    <Suspense fallback={
      <section className="flat-spacing" style={{ minHeight: "50vh" }}>
        <div className="container text-center">
          <div className="spinner-border" role="status" />
        </div>
      </section>
    }>
      <UnsubscribeContent />
    </Suspense>
  );
}

function UnsubscribeContent() {
  const { t } = useLanguage();
  const searchParams = useSearchParams();
  const [status, setStatus] = useState("loading"); // loading | success | error | invalid

  useEffect(() => {
    const email = searchParams.get("email");
    const token = searchParams.get("token");

    if (!email || !token) {
      setStatus("invalid");
      return;
    }

    fetch(buildApiUrl(`/api/email/unsubscribe?email=${encodeURIComponent(email)}&token=${encodeURIComponent(token)}`))
      .then((res) => {
        if (res.ok) {
          setStatus("success");
        } else {
          setStatus("error");
        }
      })
      .catch(() => setStatus("error"));
  }, [searchParams]);

  return (
    <>
      <Topbar6 bgColor="bg-main" />
      <Header1 />
      <section className="flat-spacing" style={{ minHeight: "50vh" }}>
        <div className="container">
          <div className="row justify-content-center">
            <div className="col-md-6 text-center">
              {status === "loading" && (
                <>
                  <div className="spinner-border mb-3" role="status" />
                  <p>{t("unsubscribe.processing")}</p>
                </>
              )}

              {status === "success" && (
                <>
                  <div style={{ fontSize: 48, marginBottom: 16, color: "#10b981" }}>
                    &#10003;
                  </div>
                  <h3 className="mb-3">{t("unsubscribe.success")}</h3>
                  <p className="text-secondary mb-4">
                    {t("unsubscribe.successMessage")}
                  </p>
                </>
              )}

              {status === "error" && (
                <>
                  <div style={{ fontSize: 48, marginBottom: 16, color: "#ef4444" }}>
                    &#10007;
                  </div>
                  <h3 className="mb-3">{t("unsubscribe.error")}</h3>
                  <p className="text-secondary mb-4">
                    {t("unsubscribe.errorMessage")}
                  </p>
                </>
              )}

              {status === "invalid" && (
                <>
                  <div style={{ fontSize: 48, marginBottom: 16, color: "#f59e0b" }}>
                    &#9888;
                  </div>
                  <h3 className="mb-3">{t("unsubscribe.invalidLink")}</h3>
                  <p className="text-secondary mb-4">
                    {t("unsubscribe.invalidLinkMessage")}
                  </p>
                </>
              )}

              {status !== "loading" && (
                <Link href="/" className="tf-btn btn-fill radius-3">
                  <span>{t("unsubscribe.backToHome")}</span>
                </Link>
              )}
            </div>
          </div>
        </div>
      </section>
      <Footer1 />
    </>
  );
}
