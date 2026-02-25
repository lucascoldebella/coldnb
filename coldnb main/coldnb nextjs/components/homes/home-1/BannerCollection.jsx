"use client";
import React from "react";
import Image from "next/image";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";

const getDefaultBanners = (t) => [
  {
    imgSrc: "/images/collections/banner-collection/banner-cls1.jpg",
    title: t("homepage.crossbodyBag"),
    description: t("homepage.beachToParty"),
    btnLink: "/shop-collection",
    textColor: "dark",
    position: "left",
  },
  {
    imgSrc: "/images/collections/banner-collection/banner-cls2.jpg",
    title: t("homepage.capsuleCollection"),
    description: t("homepage.reservedSpecial"),
    btnLink: "/shop-collection",
    textColor: "white",
    position: "right",
  },
];

export default function BannerCollection({ data }) {
  const { t } = useLanguage();
  const defaultBanners = getDefaultBanners(t);
  const banners = data && data.length > 0
    ? data.map((b) => ({
        imgSrc: b.image_url,
        title: b.title || "",
        description: b.subtitle || "",
        btnLink: b.button_link || "/shop-collection",
        btnText: b.button_text || t("homepage.shopNow"),
        textColor: b.text_color || "dark",
        position: b.position || "left",
      }))
    : defaultBanners;

  return (
    <section className="flat-spacing pt-0">
      <div className="container">
        <div className="tf-grid-layout md-col-2">
          {banners.map((banner, index) => {
            const isWhite = banner.textColor === "white";
            const className = isWhite ? "collection-position hover-img" : "collection-default hover-img";

            return (
              <div key={index} className={className}>
                <a className="img-style">
                  <Image
                    className="lazyload"
                    data-src={banner.imgSrc}
                    alt={banner.title || "banner-cls"}
                    src={banner.imgSrc}
                    width={945}
                    height={isWhite ? 945 : 709}
                  />
                </a>
                <div className="content">
                  <h3 className="title">
                    <Link
                      href={banner.btnLink}
                      className={`link ${isWhite ? "text-white wow fadeInUp" : "wow fadeInUp"}`}
                    >
                      {banner.title}
                    </Link>
                  </h3>
                  <p className={`desc ${isWhite ? "text-white" : ""} wow fadeInUp`}>
                    {banner.description}
                  </p>
                  <div className="wow fadeInUp">
                    <Link
                      href={banner.btnLink}
                      className={`btn-line ${isWhite ? "style-white" : ""}`}
                    >
                      {banner.btnText || "Shop Now"}
                    </Link>
                  </div>
                </div>
              </div>
            );
          })}
        </div>
      </div>
    </section>
  );
}
