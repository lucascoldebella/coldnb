"use client";
import React from "react";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";
export default function ForgotPass() {
  const { t } = useLanguage();
  return (
    <section className="flat-spacing">
      <div className="container">
        <div className="login-wrap">
          <div className="left">
            <div className="heading">
              <h4 className="mb_8">{t("forgotPassword.title")}</h4>
              <p>{t("forgotPassword.subtitle")}</p>
            </div>
            <form onSubmit={(e) => e.preventDefault()} className="form-login">
              <div className="wrap">
                <fieldset className="">
                  <input
                    className=""
                    type="email"
                    placeholder={t("login.usernameOrEmail")}
                    name="email"
                    tabIndex={2}
                    defaultValue=""
                    aria-required="true"
                    required
                  />
                </fieldset>
              </div>
              <div className="button-submit">
                <button className="tf-btn btn-fill" type="submit">
                  <span className="text text-button">{t("forgotPassword.submit")}</span>
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
