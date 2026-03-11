"use client";
import React, { useRef, useState } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { buildApiUrl } from "@/lib/apiBase";
export default function Contact1() {
  const { t } = useLanguage();
  const formRef = useRef();
  const [success, setSuccess] = useState(true);
  const [showMessage, setShowMessage] = useState(false);

  const [submitting, setSubmitting] = useState(false);

  const handleShowMessage = () => {
    setShowMessage(true);
    setTimeout(() => {
      setShowMessage(false);
    }, 2000);
  };

  const sendMail = async (e) => {
    e.preventDefault();
    if (submitting) return;
    const form = formRef.current;
    setSubmitting(true);
    try {
      const res = await fetch(buildApiUrl("/api/contact"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          name: form.name.value,
          email: form.email.value,
          message: form.message.value,
        }),
      });
      if (res.ok) {
        setSuccess(true);
        form.reset();
      } else {
        setSuccess(false);
      }
    } catch {
      setSuccess(false);
    } finally {
      setSubmitting(false);
      handleShowMessage();
    }
  };
  return (
    <section className="flat-spacing pt-0">
      <div className="container">
        <div className="heading-section text-center">
          <h3 className="heading">{t("contact.getInTouch")}</h3>
          <p className="subheading">
            {t("contact.subtitle")}
          </p>
        </div>
        <div
          className={`tfSubscribeMsg  footer-sub-element ${
            showMessage ? "active" : ""
          }`}
        >
          {success ? (
            <p style={{ color: "rgb(52, 168, 83)" }}>
              {t("contact.messageSent")}
            </p>
          ) : (
            <p style={{ color: "red" }}>{t("contact.somethingWrong")}</p>
          )}
        </div>
        <form onSubmit={sendMail} ref={formRef} className="form-leave-comment">
          <div className="wrap">
            <div className="cols">
              <fieldset className="">
                <input
                  className=""
                  type="text"
                  placeholder={t("contact.yourName")}
                  name="name"
                  tabIndex={2}
                  defaultValue=""
                  aria-required="true"
                  required
                />
              </fieldset>
              <fieldset className="">
                <input
                  className=""
                  type="email"
                  placeholder={t("contact.yourEmail")}
                  name="email"
                  tabIndex={2}
                  defaultValue=""
                  aria-required="true"
                  required
                />
              </fieldset>
            </div>
            <fieldset className="">
              <textarea
                className=""
                name="message"
                rows={4}
                placeholder={t("contact.yourMessage")}
                tabIndex={2}
                aria-required="true"
                required
                defaultValue={""}
              />
            </fieldset>
          </div>
          <div className="button-submit text-center">
            <button className="tf-btn btn-fill" type="submit">
              <span className="text text-button">{t("contact.sendMessage")}</span>
            </button>
          </div>
        </form>
      </div>
    </section>
  );
}
