"use client";
import React, { useEffect, useState } from "react";
import { Swiper, SwiperSlide } from "swiper/react";
import { Pagination } from "swiper/modules";
import ProductCard1 from "../productCards/ProductCard1";
import { getProducts, getRecommendations } from "@/lib/shopApi";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function RelatedProducts({ productId }) {
  const { t } = useLanguage();
  const [products, setProducts] = useState([]);

  useEffect(() => {
    if (productId) {
      getRecommendations(productId)
        .then((res) => setProducts(res || []))
        .catch(() => {
          // Fallback to generic products if recommendations fail
          getProducts({ per_page: 8 })
            .then((res) => setProducts(res.products || []))
            .catch(() => {});
        });
    } else {
      getProducts({ per_page: 8 })
        .then((res) => setProducts(res.products || []))
        .catch(() => {});
    }
  }, [productId]);

  if (products.length === 0) return null;

  return (
    <section className="flat-spacing">
      <div className="container flat-animate-tab">
        <ul
          className="tab-product justify-content-sm-center wow fadeInUp"
          data-wow-delay="0s"
          role="tablist"
        >
          <li className="nav-tab-item" role="presentation">
            <a href="#ralatedProducts" className="active" data-bs-toggle="tab">
              {t("product.relatedProducts")}
            </a>
          </li>
        </ul>
        <div className="tab-content">
          <div
            className="tab-pane active show"
            id="ralatedProducts"
            role="tabpanel"
          >
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
                el: ".spd4",
              }}
            >
              {products.slice(0, 8).map((product, i) => (
                <SwiperSlide key={product.id || i} className="swiper-slide">
                  <ProductCard1 product={product} />
                </SwiperSlide>
              ))}

              <div className="sw-pagination-latest spd4 sw-dots type-circle justify-content-center" />
            </Swiper>
          </div>
        </div>
      </div>
    </section>
  );
}
