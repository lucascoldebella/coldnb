"use client";
import React, { useEffect, useRef, useState } from "react";
import Image from "next/image";
import Link from "next/link";

import LanguageSelect from "../common/LanguageSelect";
import ToolbarBottom from "../headers/ToolbarBottom";
import ScrollTop from "../common/ScrollTop";
import { socialLinks } from "@/data/footerLinks";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function Footer1({
  border = true,
  dark = false,
  hasPaddingBottom = false,
}) {
  const { t } = useLanguage();
  const [success, setSuccess] = useState(true);
  const [showMessage, setShowMessage] = useState(false);

  const handleShowMessage = () => {
    setShowMessage(true);
    setTimeout(() => {
      setShowMessage(false);
    }, 2000);
  };

  const sendEmail = async (e) => {
    e.preventDefault();
    const email = e.target.email.value;
    console.log("Newsletter signup (configure your service):", email);
    e.target.reset();
    setSuccess(true);
    handleShowMessage();
  };

  const translatedFooterLinks = [
    {
      heading: t("footer.information"),
      items: [
        { label: t("footer.aboutUs"), href: "/about-us", isLink: true },
        { label: t("footer.ourStories"), href: "#", isLink: false },
        { label: t("footer.sizeGuide"), href: "#", isLink: false },
        { label: t("footer.contactUs"), href: "/contact", isLink: true },
        { label: t("footer.career"), href: "#", isLink: false },
        { label: t("account.myAccount"), href: "/my-account", isLink: true },
      ],
    },
    {
      heading: t("footer.customerServices"),
      items: [
        { label: t("footer.shipping"), href: "#", isLink: false },
        { label: t("footer.returnRefund"), href: "#", isLink: false },
        { label: t("footer.privacyPolicy"), href: "#", isLink: false },
        { label: t("footer.termsConditions"), href: "/term-of-use", isLink: true },
        { label: t("footer.ordersFaqs"), href: "#", isLink: false },
        { label: t("footer.myWishlist"), href: "/wish-list", isLink: true },
      ],
    },
  ];

  useEffect(() => {
    const headings = document.querySelectorAll(".footer-heading-mobile");

    const toggleOpen = (event) => {
      const parent = event.target.closest(".footer-col-block");
      const content = parent.querySelector(".tf-collapse-content");

      if (parent.classList.contains("open")) {
        parent.classList.remove("open");
        content.style.height = "0px";
      } else {
        parent.classList.add("open");
        content.style.height = content.scrollHeight + 10 + "px";
      }
    };

    headings.forEach((heading) => {
      heading.addEventListener("click", toggleOpen);
    });

    return () => {
      headings.forEach((heading) => {
        heading.removeEventListener("click", toggleOpen);
      });
    };
  }, []);

  return (
    <>
      <footer
        id="footer"
        className={`footer ${dark ? "bg-main" : ""} ${hasPaddingBottom ? "has-pb" : ""} `}
      >
        <div className={`footer-wrap ${!border ? "border-0" : ""}`}>
          <div className="footer-body">
            <div className="container">
              <div className="row">
                <div className="col-lg-4">
                  <div className="footer-infor">
                    <div className="footer-logo">
                      <Link href={`/`}>
                        <Image
                          alt=""
                          src={dark ? "/images/logo/logo-white.svg" : "/images/logo/logo.svg"}
                          width={127}
                          height={24}
                          style={{ width: "auto", height: "auto" }}
                        />
                      </Link>
                    </div>
                    <div className="footer-address">
                      <p>549 Oak St.Crystal Lake, IL 60014</p>
                      <Link
                        href={`/contact`}
                        className={`tf-btn-default fw-6 ${dark ? "style-white" : ""} `}
                      >
                        {t("footer.getDirection")}
                        <i className="icon-arrowUpRight" />
                      </Link>
                    </div>
                    <ul className="footer-info">
                      <li>
                        <i className="icon-mail" />
                        <p>contato@coldnb.com.br</p>
                      </li>
                      <li>
                        <i className="icon-phone" />
                        <p>315-666-6688</p>
                      </li>
                    </ul>
                    <ul className={`tf-social-icon ${dark ? "style-white" : ""} `}>
                      {socialLinks.map((link, index) => (
                        <li key={index}>
                          <a href={link.href} className={link.className}>
                            <i className={`icon ${link.iconClass}`} />
                          </a>
                        </li>
                      ))}
                    </ul>
                  </div>
                </div>
                <div className="col-lg-4">
                  <div className="footer-menu">
                    {translatedFooterLinks.map((section, sectionIndex) => (
                      <div className="footer-col-block" key={sectionIndex}>
                        <div className="footer-heading text-button footer-heading-mobile">
                          {section.heading}
                        </div>
                        <div className="tf-collapse-content">
                          <ul className="footer-menu-list">
                            {section.items.map((item, itemIndex) => (
                              <li className="text-caption-1" key={itemIndex}>
                                {item.isLink ? (
                                  <Link href={item.href} className="footer-menu_item">
                                    {item.label}
                                  </Link>
                                ) : (
                                  <a href={item.href} className="footer-menu_item">
                                    {item.label}
                                  </a>
                                )}
                              </li>
                            ))}
                          </ul>
                        </div>
                      </div>
                    ))}
                  </div>
                </div>
                <div className="col-lg-4">
                  <div className="footer-col-block">
                    <div className="footer-heading text-button footer-heading-mobile">
                      {t("footer.subscription")}
                    </div>
                    <div className="tf-collapse-content">
                      <div className="footer-newsletter">
                        <p className="text-caption-1">
                          {t("newsletter.subtitle")}
                        </p>
                        <div
                          className={`tfSubscribeMsg footer-sub-element ${showMessage ? "active" : ""}`}
                        >
                          {success ? (
                            <p style={{ color: "rgb(52, 168, 83)" }}>
                              {t("newsletter.success")}
                            </p>
                          ) : (
                            <p style={{ color: "red" }}>{t("newsletter.error")}</p>
                          )}
                        </div>
                        <form
                          onSubmit={sendEmail}
                          className={`form-newsletter subscribe-form ${dark ? "style-black" : ""}`}
                        >
                          <div className="subscribe-content">
                            <fieldset className="email">
                              <input
                                type="email"
                                name="email"
                                className="subscribe-email"
                                placeholder={t("newsletter.emailPlaceholder")}
                                tabIndex={0}
                                aria-required="true"
                              />
                            </fieldset>
                            <div className="button-submit">
                              <button className="subscribe-button" type="submit">
                                <i className="icon icon-arrowUpRight" />
                              </button>
                            </div>
                          </div>
                          <div className="subscribe-msg" />
                        </form>
                        <div className="tf-cart-checkbox">
                          <div className="tf-checkbox-wrapp">
                            <input
                              className=""
                              type="checkbox"
                              id="footer-Form_agree"
                              name="agree_checkbox"
                            />
                            <div>
                              <i className="icon-check" />
                            </div>
                          </div>
                          <label
                            className="text-caption-1"
                            htmlFor="footer-Form_agree"
                          >
                            {t("newsletter.bySubscribing")}{" "}
                            <Link className="fw-6 link" href={`/term-of-use`}>
                              {t("newsletter.termsOfService")}
                            </Link>{" "}
                            {t("newsletter.and")}{" "}
                            <a className="fw-6 link" href="#">
                              {t("newsletter.privacyPolicy")}
                            </a>
                            .
                          </label>
                        </div>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
          <div className="footer-bottom">
            <div className="container">
              <div className="row">
                <div className="col-12">
                  <div className="footer-bottom-wrap">
                    <div className="left">
                      <p className="text-caption-1">
                        {t("footer.copyright").replace("{year}", new Date().getFullYear())}
                      </p>
                      <div className="tf-cur justify-content-end">
                        <div className="tf-languages">
                          <LanguageSelect light={dark} />
                        </div>
                      </div>
                    </div>
                    <div className="tf-payment">
                      <p className="text-caption-1">{t("footer.payment")}</p>
                      <ul>
                        <li><Image alt="" src="/images/payment/img-1.png" width={100} height={64} /></li>
                        <li><Image alt="" src="/images/payment/img-2.png" width={100} height={64} /></li>
                        <li><Image alt="" src="/images/payment/img-3.png" width={100} height={64} /></li>
                        <li><Image alt="" src="/images/payment/img-4.png" width={98} height={64} /></li>
                        <li><Image alt="" src="/images/payment/img-5.png" width={102} height={64} /></li>
                        <li><Image alt="" src="/images/payment/img-6.png" width={98} height={64} /></li>
                      </ul>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </footer>
      <ScrollTop hasPaddingBottom={hasPaddingBottom} />
      <ToolbarBottom />
    </>
  );
}
