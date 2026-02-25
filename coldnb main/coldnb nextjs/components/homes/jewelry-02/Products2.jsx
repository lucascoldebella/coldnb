"use client";
import ProductCard1 from "@/components/productCards/ProductCard1";
import { getProducts } from "@/lib/shopApi";
import React, { useEffect, useState } from "react";
import { Pagination } from "swiper/modules";
import { Swiper, SwiperSlide } from "swiper/react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function Products2({ parentClass = "" }) {
  const { t } = useLanguage();
  const [products, setProducts] = useState([]);

  useEffect(() => {
    getProducts({ per_page: 8 })
      .then((res) => setProducts(res.products || []))
      .catch(() => {});
  }, []);

  if (products.length === 0) return null;

  return (
    <section className={parentClass}>
      <div className="container">
        <div className="heading-section text-center wow fadeInUp">
          <h3 className="heading">{t("homepage.trendyCollection")}</h3>
          <p className="subheading text-secondary">
            {t("homepage.freshStyles")}
          </p>
        </div>
        <Swiper
          className="swiper tf-sw-latest"
          dir="ltr"
          spaceBetween={15}
          breakpoints={{
            0: { slidesPerView: 2, spaceBetween: 15 },
            768: { slidesPerView: 3, spaceBetween: 30 },
            1200: { slidesPerView: 4, spaceBetween: 30 },
          }}
          modules={[Pagination]}
          pagination={{
            clickable: true,
            el: ".spd63",
          }}
        >
          {products.map((product, i) => (
            <SwiperSlide key={product.id || i} className="swiper-slide">
              <ProductCard1 product={product} />
            </SwiperSlide>
          ))}

          <div className="sw-pagination-latest sw-dots type-circle justify-content-center spd63" />
        </Swiper>
      </div>
    </section>
  );
}
