"use client";
import React from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function MarqueeSection2({ parentClass = "tf-marquee" }) {
  const { t } = useLanguage();
  return (
    <section className={parentClass}>
      <div className="marquee-wrapper">
        <div className="initial-child-container">
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeShippingOver")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeReturns14")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
          {/* 2 */}
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeShippingOver")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeReturns14")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
          {/* 3 */}
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeShippingOver")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeReturns14")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
          {/* 4 */}
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeShippingOver")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeReturns14")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
          {/* 5 */}
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeShippingOver")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeReturns14")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
          {/* 6 */}
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeShippingOver")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
          <div className="marquee-child-item">
            <p className="text-btn-uppercase">
              {t("homepage.freeReturns14")}
            </p>
          </div>
          <div className="marquee-child-item">
            <span className="icon icon-lightning-line" />
          </div>
        </div>
      </div>
    </section>
  );
}
