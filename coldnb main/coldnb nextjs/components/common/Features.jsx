"use client";

import { useLanguage } from "@/lib/i18n/LanguageContext";
import { Pagination } from "swiper/modules";
import { Swiper, SwiperSlide } from "swiper/react";

const featureIcons = [
  { id: 1, icon: "icon-return", titleKey: "features.returns14Title", descKey: "features.returns14Desc" },
  { id: 2, icon: "icon-shipping", titleKey: "features.shippingTitle", descKey: "features.shippingDesc" },
  { id: 3, icon: "icon-headset", titleKey: "features.supportTitle", descKey: "features.supportDesc" },
  { id: 4, icon: "icon-sealCheck", titleKey: "features.discountsTitle", descKey: "features.discountsDesc" },
];

export default function Features({ parentClass = "flat-spacing" }) {
  const { t } = useLanguage();
  return (
    <section className={parentClass}>
      <div className="container">
        <Swiper
          dir="ltr"
          className="swiper tf-sw-iconbox"
          spaceBetween={15}
          breakpoints={{
            1200: { slidesPerView: 4 },
            768: { slidesPerView: 3 },
            576: { slidesPerView: 2 },
            0: { slidesPerView: 1 },
          }}
          modules={[Pagination]}
          pagination={{
            clickable: true,
            el: ".spd2",
          }}
        >
          {featureIcons.map((item) => (
            <SwiperSlide key={item.id}>
              <div className="tf-icon-box">
                <div className="icon-box">
                  <span className={`icon ${item.icon}`} />
                </div>
                <div className="content text-center">
                  <h6>{t(item.titleKey)}</h6>
                  <p className="text-secondary">{t(item.descKey)}</p>
                </div>
              </div>
            </SwiperSlide>
          ))}
          <div className="sw-pagination-iconbox spd2 sw-dots type-circle justify-content-center" />
        </Swiper>
      </div>
    </section>
  );
}
