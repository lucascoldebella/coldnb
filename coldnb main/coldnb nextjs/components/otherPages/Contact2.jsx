"use client";
import React, { useRef, useState } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { buildApiUrl } from "@/lib/apiBase";
export default function Contact2() {
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
    <section className="flat-spacing">
      <div className="container">
        <div className="contact-us-content">
          <div className="left">
            <h4>{t("contact.getInTouch")}</h4>
            <p className="text-secondary-2">
              {t("contact.subtitle")}
            </p>
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
            <form
              onSubmit={sendMail}
              ref={formRef}
              id="contactform"
              className="form-leave-comment"
            >
              <div className="wrap">
                <div className="cols">
                  <fieldset className="">
                    <input
                      className=""
                      type="text"
                      placeholder={t("contact.yourName")}
                      name="name"
                      id="name"
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
                      id="email"
                      tabIndex={2}
                      defaultValue=""
                      aria-required="true"
                      required
                    />
                  </fieldset>
                </div>
                <fieldset className="">
                  <textarea
                    name="message"
                    id="message"
                    rows={4}
                    placeholder={t("contact.yourMessage")}
                    tabIndex={2}
                    aria-required="true"
                    required
                    defaultValue={""}
                  />
                </fieldset>
              </div>
              <div className="button-submit send-wrap">
                <button className="tf-btn btn-fill" type="submit">
                  <span className="text text-button">{t("contact.sendMessage")}</span>
                </button>
              </div>
            </form>
          </div>
          <div className="right">
            <h4>{t("contact.information")}</h4>
            <div className="mb_20">
              <div className="text-title mb_8">{t("contact.phoneLabel")}</div>
              <p className="text-secondary">+1 666 234 8888</p>
            </div>
            <div className="mb_20">
              <div className="text-title mb_8">{t("contact.emailLabel")}</div>
              <p className="text-secondary">contato@coldnb.com.br</p>
            </div>
            <div className="mb_20">
              <div className="text-title mb_8">{t("contact.addressLabel")}</div>
              <p className="text-secondary">
                2163 Phillips Gap Rd, West Jefferson, North Carolina, United
                States
              </p>
            </div>
            <div>
              <div className="text-title mb_8">{t("contact.openTime")}</div>
              <p className="mb_4 open-time">
                <span className="text-secondary">{t("contact.monSat")}</span> 7:30am -
                8:00pm PST
              </p>
              <p className="open-time">
                <span className="text-secondary">{t("contact.sunday")}</span> 9:00am - 5:00pm
                PST
              </p>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
