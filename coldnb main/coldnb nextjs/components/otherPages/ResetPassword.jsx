"use client";
import React, { useState, useEffect } from "react";
import Link from "next/link";
import { useRouter } from "next/navigation";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import supabase from "@/lib/supabase";

export default function ResetPassword() {
  const { t } = useLanguage();
  const router = useRouter();
  const [password, setPassword] = useState("");
  const [confirm, setConfirm] = useState("");
  const [loading, setLoading] = useState(false);
  const [done, setDone] = useState(false);
  const [error, setError] = useState("");
  const [ready, setReady] = useState(false);

  // Supabase sends the user to this page with a hash fragment containing
  // access_token + type=recovery. We need to detect that session.
  useEffect(() => {
    if (!supabase) return;

    // onAuthStateChange fires with SIGNED_IN + recovery session
    const { data: { subscription } } = supabase.auth.onAuthStateChange(
      (event) => {
        if (event === "PASSWORD_RECOVERY") {
          setReady(true);
        }
      }
    );

    // Also check if already has an active recovery session (page reload case)
    supabase.auth.getSession().then(({ data }) => {
      if (data?.session) setReady(true);
    });

    return () => subscription.unsubscribe();
  }, []);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError("");

    if (password.length < 6) {
      setError(t("resetPassword.tooShort"));
      return;
    }
    if (password !== confirm) {
      setError(t("resetPassword.mismatch"));
      return;
    }

    setLoading(true);
    const { error: updateError } = await supabase.auth.updateUser({ password });
    setLoading(false);

    if (updateError) {
      setError(updateError.message || t("resetPassword.error"));
    } else {
      setDone(true);
      setTimeout(() => router.push("/login"), 3000);
    }
  };

  if (done) {
    return (
      <section className="flat-spacing">
        <div className="container">
          <div className="login-wrap">
            <div className="left">
              <div className="heading">
                <h4 className="mb_8">{t("resetPassword.successTitle")}</h4>
                <p>{t("resetPassword.successDesc")}</p>
              </div>
              <Link href="/login" className="tf-btn btn-fill mt_20">
                <span className="text text-button">{t("auth.backToLogin")}</span>
              </Link>
            </div>
          </div>
        </div>
      </section>
    );
  }

  if (!ready) {
    return (
      <section className="flat-spacing">
        <div className="container">
          <div className="login-wrap">
            <div className="left">
              <div className="heading">
                <h4 className="mb_8">{t("resetPassword.waitingTitle")}</h4>
                <p>{t("resetPassword.waitingDesc")}</p>
              </div>
              <div className="d-flex justify-content-start mt_20">
                <div className="spinner-border" role="status">
                  <span className="visually-hidden">{t("common.loading")}</span>
                </div>
              </div>
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
              <h4 className="mb_8">{t("resetPassword.title")}</h4>
              <p>{t("resetPassword.subtitle")}</p>
            </div>
            <form onSubmit={handleSubmit} className="form-login">
              <div className="wrap">
                <fieldset>
                  <input
                    type="password"
                    placeholder={t("resetPassword.newPassword")}
                    value={password}
                    onChange={(e) => setPassword(e.target.value)}
                    required
                    minLength={6}
                  />
                </fieldset>
                <fieldset className="mt_12">
                  <input
                    type="password"
                    placeholder={t("resetPassword.confirmPassword")}
                    value={confirm}
                    onChange={(e) => setConfirm(e.target.value)}
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
                    {loading ? t("common.loading") : t("resetPassword.submit")}
                  </span>
                </button>
              </div>
            </form>
          </div>
        </div>
      </div>
    </section>
  );
}
