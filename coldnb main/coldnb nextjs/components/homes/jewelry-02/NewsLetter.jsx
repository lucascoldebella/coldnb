"use client";
import React from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";


export default function NewsLetter() {
  const { t } = useLanguage();
  return (
    <section className="section-newsletter">
      <div className="content">
        <h3 className="heading text-white wow fadeInUp">
          {t("homepage.signUpGet20")}
        </h3>
        <p className="text text-white wow fadeInUp" data-wow-delay="0.1s">
          {t("homepage.signUpPromo")}
        </p>
        <form
          className="form-newsletter subscribe-form wow fadeInUp"
          data-wow-delay="0.2s"
          id="subscribe-form"
          onSubmit={(e) => e.preventDefault()}
        >
          <div id="subscribe-content" className="subscribe-content">
            <fieldset className="email">
              <input
                type="email"
                name="email-form"
                id="subscribe-email"
                className="subscribe-email"
                placeholder={t("homepage.enterEmail")}
                tabIndex={0}
                aria-required="true"
              />
            </fieldset>
            <div className="button-submit">
              <button
                className="subscribe-button text-btn-uppercase font-2"
                type="button"
                id="subscribe-button"
              >
                {t("newsletter.subscribe")}
              </button>
            </div>
          </div>
          <div id="subscribe-msg" className="subscribe-msg" />
        </form>
      </div>
    </section>
  );
}
