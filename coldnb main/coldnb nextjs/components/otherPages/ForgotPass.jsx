"use client";
import React, { useState } from "react";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import supabase from "@/lib/supabase";

export default function ForgotPass() {
  const { t } = useLanguage();
  const [email, setEmail] = useState("");
  const [loading, setLoading] = useState(false);
  const [sent, setSent] = useState(false);
  const [error, setError] = useState("");

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!email.trim() || !supabase) return;

    setLoading(true);
    setError("");

    const siteUrl =
      typeof window !== "undefined"
        ? window.location.origin
        : process.env.NEXT_PUBLIC_SITE_URL || "";

    const { error: resetError } = await supabase.auth.resetPasswordForEmail(
      email.trim(),
      { redirectTo: `${siteUrl}/reset-password` }
    );

    setLoading(false);

    if (resetError) {
      setError(resetError.message || t("forgotPassword.error"));
    } else {
      setSent(true);
    }
  };

  if (sent) {
    return (
      <section className="flat-spacing">
        <div className="container">
          <div className="login-wrap">
            <div className="left">
              <div className="heading">
                <h4 className="mb_8">{t("forgotPassword.sentTitle")}</h4>
                <p>{t("forgotPassword.sentDesc").replace("{email}", email)}</p>
              </div>
              <Link href="/login" className="tf-btn btn-fill">
                <span className="text text-button">{t("auth.backToLogin")}</span>
              </Link>
            </div>
          </div>
        </div>
      </section>
    );
  }

  return (
    <section className="flat-spacing">
      <div className="container">
        <div className="login-wrap">
          <div className="left">
            <div className="heading">
              <h4 className="mb_8">{t("forgotPassword.title")}</h4>
              <p>{t("forgotPassword.subtitle")}</p>
            </div>
            <form onSubmit={handleSubmit} className="form-login">
              <div className="wrap">
                <fieldset className="">
                  <input
                    className=""
                    type="email"
                    placeholder={t("login.usernameOrEmail")}
                    name="email"
                    tabIndex={2}
                    value={email}
                    onChange={(e) => setEmail(e.target.value)}
                    aria-required="true"
                    required
                  />
                </fieldset>
              </div>
              {error && (
                <p style={{ color: "#dc3545", marginBottom: 12 }}>{error}</p>
              )}
              <div className="button-submit">
                <button
                  className="tf-btn btn-fill"
                  type="submit"
                  disabled={loading}
                >
                  <span className="text text-button">
                    {loading ? t("common.loading") : t("forgotPassword.submit")}
                  </span>
                </button>
              </div>
            </form>
          </div>
          <div className="right">
            <h4 className="mb_8">{t("forgotPassword.newCustomer")}</h4>
            <p className="text-secondary">
              {t("forgotPassword.newCustomerDesc")}
            </p>
            <Link href={`/login`} className="tf-btn btn-fill">
              <span className="text text-button">{t("register.register")}</span>
            </Link>
          </div>
        </div>
      </div>
    </section>
  );
}
