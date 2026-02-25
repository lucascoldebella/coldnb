"use client";
import React from "react";
import Image from "next/image";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";
export default function Banner() {
  const { t } = useLanguage();
  return (
    <section>
      <div className="container">
        <div className="flat-img-with-text">
          <div className="banner banner-left wow fadeInLeft">
            <Image
              alt="banner"
              src="/images/banner/banner-w-text1.jpg"
              width={709}
              height={709}
            />
          </div>
          <div className="banner-content">
            <div className="content-text wow fadeInUp">
              <h3 className="title text-center fw-5">
                {t("homepage.specialOffer")} <br />
                {t("homepage.thisWeekOnly")}
              </h3>
              <p className="desc">{t("homepage.reservedSpecial")}</p>
            </div>
            <Link
              href={`/shop-default-grid`}
              className="tf-btn btn-fill wow fadeInUp"
            >
              <span className="text">{t("homepage.exploreCollection")}</span>
              <i className="icon icon-arrowUpRight" />
            </Link>
          </div>
          <div className="banner banner-right wow fadeInRight">
            <Image
              alt="banner"
              src="/images/banner/banner-w-text2.jpg"
              width={945}
              height={709}
            />
          </div>
        </div>
      </div>
    </section>
  );
}
