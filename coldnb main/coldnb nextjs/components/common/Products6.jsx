"use client";

import ProductCard1 from "@/components/productCards/ProductCard1";
import { products12 } from "@/data/products";
import { Swiper, SwiperSlide } from "swiper/react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function Products6() {
  const { t } = useLanguage();
  return (
    <section className="flat-spacing">
      <div className="container">
        <div className="heading-section text-center wow fadeInUp">
          <h3 className="heading">{t("homepage.topTrending")}</h3>
          <p className="subheading text-secondary">
            {t("homepage.browseTopTrending")}
          </p>
        </div>

        <Swiper
          spaceBetween={15}
          dir="ltr"
          className="swiper tf-sw-latest"
          breakpoints={{
            0: { slidesPerView: 2, spaceBetween: 15 },

            768: { slidesPerView: 3, spaceBetween: 30 },
            1200: { slidesPerView: 4, spaceBetween: 30 },
          }}
        >
          {products12.slice(4).map((product, index) => (
            <SwiperSlide key={index}>
              <ProductCard1 product={product} />
            </SwiperSlide>
          ))}
        </Swiper>
      </div>
    </section>
  );
}
