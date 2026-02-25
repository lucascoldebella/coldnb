"use client";
import React from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function Faqs() {
  const { t } = useLanguage();
  return (
    <section className="flat-spacing">
      <div className="container">
        <div className="page-faqs-wrap">
          <div className="list-faqs">
            <div>
              <h5 className="faqs-title">{t("faq.howToBuy")}</h5>
              <ul
                className="accordion-product-wrap style-faqs"
                id="accordion-faq-1"
              >
                <li className="accordion-product-item">
                  <a
                    href="#accordion-1"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-1"
                  >
                    <h6>
                      {t("faq.q1")}
                    </h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-1"
                    className="collapse"
                    data-bs-parent="#accordion-faq-1"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-2"
                    className="accordion-title current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-2"
                  >
                    <h6>
                      I have a promotional or discount code. How do I use it for
                      an online purchase?
                    </h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-2"
                    className="collapse show"
                    data-bs-parent="#accordion-faq-1"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-3"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-3"
                  >
                    <h6>{t("faq.q3")}</h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-3"
                    className="collapse"
                    data-bs-parent="#accordion-faq-1"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-4"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-4"
                  >
                    <h6>{t("faq.q4")}</h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-4"
                    className="collapse"
                    data-bs-parent="#accordion-faq-1"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-5"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-5"
                  >
                    <h6>
                      {t("faq.q5")}
                    </h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-5"
                    className="collapse"
                    data-bs-parent="#accordion-faq-1"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
              </ul>
            </div>
            <div>
              <h5 className="faqs-title">{t("faq.exchangesReturns")}</h5>
              <ul
                className="accordion-product-wrap style-faqs"
                id="accordion-faq-2"
              >
                <li className="accordion-product-item">
                  <a
                    href="#accordion-6"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-6"
                  >
                    <h6>{t("faq.q6")}</h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-6"
                    className="collapse"
                    data-bs-parent="#accordion-faq-2"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-7"
                    className="accordion-title current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-7"
                  >
                    <h6>
                      {t("faq.q7")}
                    </h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-7"
                    className="collapse show"
                    data-bs-parent="#accordion-faq-2"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-8"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-8"
                  >
                    <h6>{t("faq.q8")}</h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-8"
                    className="collapse"
                    data-bs-parent="#accordion-faq-2"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-9"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-9"
                  >
                    <h6>{t("faq.q9")}</h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-9"
                    className="collapse"
                    data-bs-parent="#accordion-faq-2"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-10"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-10"
                  >
                    <h6>{t("faq.q10")}</h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-10"
                    className="collapse"
                    data-bs-parent="#accordion-faq-2"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
              </ul>
            </div>
            <div>
              <h5 className="faqs-title">{t("faq.refundQuestions")}</h5>
              <ul
                className="accordion-product-wrap style-faqs"
                id="accordion-faq-3"
              >
                <li className="accordion-product-item">
                  <a
                    href="#accordion-11"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-11"
                  >
                    <h6>
                      {t("faq.q11")}
                    </h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-11"
                    className="collapse"
                    data-bs-parent="#accordion-faq-3"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-12"
                    className="accordion-title current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-12"
                  >
                    <h6>
                      {t("faq.q12")}
                    </h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-12"
                    className="collapse show"
                    data-bs-parent="#accordion-faq-2"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-13"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-13"
                  >
                    <h6>
                      {t("faq.q13")}
                    </h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-13"
                    className="collapse"
                    data-bs-parent="#accordion-faq-2"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-14"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-14"
                  >
                    <h6>
                      {t("faq.q14")}
                    </h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-14"
                    className="collapse"
                    data-bs-parent="#accordion-faq-2"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
                <li className="accordion-product-item">
                  <a
                    href="#accordion-15"
                    className="accordion-title collapsed current"
                    data-bs-toggle="collapse"
                    aria-expanded="true"
                    aria-controls="accordion-15"
                  >
                    <h6>{t("faq.q15")}</h6>
                    <span className="btn-open-sub" />
                  </a>
                  <div
                    id="accordion-15"
                    className="collapse"
                    data-bs-parent="#accordion-faq-2"
                  >
                    <div className="accordion-faqs-content">
                      <p className="text-secondary">
                        {t("faq.faqAnswer")}
                      </p>
                    </div>
                  </div>
                </li>
              </ul>
            </div>
          </div>
          <div className="ask-question sticky-top">
            <div className="ask-question-wrap">
              <h5 className="mb_4">{t("faq.askYourQuestion")}</h5>
              <p className="mb_20 text-secondary">
                {t("faq.askAnything")}
              </p>
              <form
                className="form-leave-comment"
                onSubmit={(e) => e.preventDefault()}
              >
                <fieldset className="mb_20">
                  <div className="text-caption-1 mb_8">{t("faq.name")}</div>
                  <input
                    className=""
                    type="text"
                    placeholder={t("faq.yourName")}
                    name="text"
                    tabIndex={2}
                    defaultValue=""
                    aria-required="true"
                    required
                  />
                </fieldset>
                <fieldset className="mb_20">
                  <div className="text-caption-1 mb_8">
                    {t("faq.howCanWeHelp")}
                  </div>
                  <div className="tf-select">
                    <select className="">
                      <option>{t("faq.exchangesReturns")}</option>
                      <option>{t("faq.other")}</option>
                    </select>
                  </div>
                </fieldset>
                <fieldset className="mb_20">
                  <div className="text-caption-1 mb_8">{t("contact.message")}</div>
                  <textarea
                    className=""
                    rows={4}
                    placeholder={t("faq.yourMessage")}
                    tabIndex={2}
                    aria-required="true"
                    required
                    defaultValue={""}
                  />
                </fieldset>
                <div className="button-submit">
                  <button className="btn-style-2 w-100" type="submit">
                    <span className="text text-button">{t("faq.sendRequest")}</span>
                  </button>
                </div>
              </form>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
