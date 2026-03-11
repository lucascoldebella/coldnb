"use client";

import Link from "next/link";
import { useRouter, useSearchParams } from "next/navigation";
import { useEffect, useState } from "react";
import supabase from "@/lib/supabase";
import { sanitizeNextPath } from "@/lib/authRedirect";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function AuthCallbackContent() {
  const router = useRouter();
  const searchParams = useSearchParams();
  const { t } = useLanguage();
  const [error, setError] = useState("");
  const [countdown, setCountdown] = useState(3);
  const [isConfirmSuccess, setIsConfirmSuccess] = useState(false);

  const nextPath = sanitizeNextPath(searchParams.get("next"));
  const flow = searchParams.get("flow") || "oauth";
  const providerError =
    searchParams.get("error_description") || searchParams.get("error") || "";

  useEffect(() => {
    if (!supabase) {
      setError(t("auth.authNotConfigured"));
      return;
    }

    if (providerError) {
      setError(providerError);
      return;
    }

    let active = true;
    let handledSuccess = false;
    let timeoutId;
    let countdownIntervalId;
    const authCode = searchParams.get("code");

    const finishIfAuthenticated = async () => {
      const { data } = await supabase.auth.getSession();
      return Boolean(data?.session);
    };

    const startConfirmationRedirect = () => {
      if (!active) {
        return;
      }

      setIsConfirmSuccess(true);
      setCountdown(3);

      countdownIntervalId = window.setInterval(() => {
        setCountdown((current) => {
          if (current <= 1) {
            window.clearInterval(countdownIntervalId);
            return 1;
          }

          return current - 1;
        });
      }, 1000);

      timeoutId = window.setTimeout(() => {
        if (active) {
          router.replace(nextPath);
        }
      }, 3000);
    };

    const handleAuthenticated = () => {
      if (!active || handledSuccess) {
        return;
      }

      handledSuccess = true;

      if (flow === "confirm-email") {
        startConfirmationRedirect();
        return;
      }

      router.replace(nextPath);
    };

    const { data: authListener } = supabase.auth.onAuthStateChange((_event, session) => {
      if (!active || !session) return;
      handleAuthenticated();
    });

    const completeAuth = async () => {
      if (authCode) {
        const { data, error: exchangeError } = await supabase.auth.exchangeCodeForSession(authCode);
        if (exchangeError) {
          if (active) {
            setError(exchangeError.message || t("auth.authCallbackError"));
          }
          return;
        }

        if (data?.session) {
          handleAuthenticated();
          return;
        }
      }

      if (await finishIfAuthenticated()) {
        handleAuthenticated();
      }
    };

    completeAuth();

    if (flow !== "confirm-email") {
      timeoutId = window.setTimeout(async () => {
        if (await finishIfAuthenticated()) {
          return;
        }

        if (active) {
          setError(t("auth.authCallbackError"));
        }
      }, 6000);
    } else {
      timeoutId = window.setTimeout(async () => {
        if (await finishIfAuthenticated()) {
          handleAuthenticated();
          return;
        }

        if (active) {
          setError(t("auth.authCallbackError"));
        }
      }, 6000);
    }

    return () => {
      active = false;
      window.clearTimeout(timeoutId);
      window.clearInterval(countdownIntervalId);
      authListener.subscription.unsubscribe();
    };
  }, [flow, nextPath, providerError, router, searchParams, t]);

  return (
    <section className="flat-spacing">
      <div className="container">
        <div
          className="login-wrap"
          style={{ justifyContent: "center", minHeight: 360 }}
        >
          <div className="auth-flow-card text-center" style={{ maxWidth: 480, width: "100%" }}>
            {error ? (
              <>
                <h4 className="mb_12">{t("auth.authCallbackFailed")}</h4>
                <p className="text-secondary mb_20">{error}</p>
                <Link href="/login" className="tf-btn btn-fill radius-4">
                  <span className="text">{t("auth.backToLogin")}</span>
                </Link>
              </>
            ) : (
              <>
                {isConfirmSuccess ? (
                  <>
                    <div
                      className="mb_20"
                      style={{
                        width: 72,
                        height: 72,
                        borderRadius: "50%",
                        margin: "0 auto 20px",
                        display: "grid",
                        placeItems: "center",
                        background: "rgba(33, 150, 83, 0.12)",
                        color: "#1f8f4d",
                        fontSize: 30,
                        fontWeight: 700,
                      }}
                    >
                      ✓
                    </div>
                    <h4 className="mb_12">{t("auth.confirmEmailSuccess")}</h4>
                    <p className="text-secondary mb_12">{t("auth.redirecting")}</p>
                    <div style={{ fontSize: 36, fontWeight: 600, lineHeight: 1 }}>{countdown}</div>
                  </>
                ) : (
                  <>
                    <div className="tf-loading mb_20" style={{ margin: "0 auto 20px" }} />
                    <h4 className="mb_12">{t("auth.finishingSignIn")}</h4>
                    <p className="text-secondary">{t("auth.redirecting")}</p>
                  </>
                )}
              </>
            )}
          </div>
        </div>
      </div>
    </section>
  );
}
