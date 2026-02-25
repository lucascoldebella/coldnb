"use client";
import React from "react";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";
export default function OrderTrac() {
  const { t } = useLanguage();
  return (
    <section className="flat-spacing">
      <div className="container">
        <div className="login-wrap tracking-wrap">
          <div className="left">
            <div className="heading">
              <h4 className="mb_8">{t("orderTracking.title")}</h4>
              <p>
                {t("orderTracking.description")}
              </p>
            </div>
            <form onSubmit={(e) => e.preventDefault()} className="form-login">
              <div className="wrap">
                <fieldset>
                  <input type="text" placeholder={t("orderTracking.orderId")} />
                </fieldset>
                <fieldset>
                  <input type="text" placeholder={t("orderTracking.billingEmail")} />
                </fieldset>
              </div>
              <div className="button-submit">
                <button className="tf-btn btn-fill" type="submit">
                  <span className="text">{t("orderTracking.trackingOrders")}</span>
                </button>
              </div>
            </form>
          </div>
          <div className="right">
            <h4 className="mb_8">{t("orderTracking.alreadyHaveAccount")}</h4>
            <p className="text-secondary">
              {t("orderTracking.welcomeBack")}
            </p>
            <Link href={`/login`} className="tf-btn btn-fill">
              <span className="text">{t("login.login")}</span>
            </Link>
          </div>
        </div>
      </div>
    </section>
  );
}
