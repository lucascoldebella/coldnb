"use client";
import React from "react";
import Image from "next/image";
import Link from "next/link";
import CountdownTimer from "@/components/common/Countdown";
import { useLanguage } from "@/lib/i18n/LanguageContext";

const getDefaultBanner = (t) => ({
  title: t("homepage.limitedTimeDeals"),
  subtitle: t("homepage.upTo50Off"),
  btnText: t("homepage.shopNow"),
  btnLink: "/shop-default-grid",
  imgSrc: "/images/banner/img-countdown1.png",
  countdownTarget: "2025-06-31T23:59:59",
});

export default function BannerCountdown({ data }) {
  const { t } = useLanguage();
  const defaultBanner = getDefaultBanner(t);
  const banner = data
    ? {
        title: data.title || defaultBanner.title,
        subtitle: data.discount_label || defaultBanner.subtitle,
        btnText: data.button_text || defaultBanner.btnText,
        btnLink: data.button_link || defaultBanner.btnLink,
        imgSrc: data.image_url || defaultBanner.imgSrc,
        countdownTarget: data.countdown_end_at || defaultBanner.countdownTarget,
      }
    : defaultBanner;

  return (
    <section className="bg-surface flat-spacing flat-countdown-banner">
      <div className="container">
        <div className="row align-items-center">
          <div className="col-lg-5">
            <div className="banner-left">
              <div className="box-title">
                <h3 className="wow fadeInUp">{banner.title}</h3>
                <p className="text-secondary wow fadeInUp">
                  {banner.subtitle}
                </p>
              </div>
              <div className="btn-banner wow fadeInUp">
                <Link href={banner.btnLink} className="tf-btn btn-fill">
                  <span className="text">{banner.btnText}</span>
                  <i className="icon icon-arrowUpRight" />
                </Link>
              </div>
            </div>
          </div>
          <div className="col-lg-2">
            <div className="banner-img">
              <Image
                className="lazyload"
                data-src={banner.imgSrc}
                alt="banner"
                src={banner.imgSrc}
                width={607}
                height={655}
              />
            </div>
          </div>
          <div className="col-lg-5">
            <div className="banner-right">
              <div className="tf-countdown-lg">
                <div
                  className="js-countdown"
                  data-timer={1007500}
                  data-labels="Days,Hours,Mins,Secs"
                >
                  <CountdownTimer style={2} targetDate={banner.countdownTarget} />
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
