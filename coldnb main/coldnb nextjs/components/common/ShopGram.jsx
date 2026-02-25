"use client";
import { getProducts } from "@/lib/shopApi";
import React, { useEffect, useState } from "react";
import { Swiper, SwiperSlide } from "swiper/react";
import Image from "next/image";
import Link from "next/link";
import { Pagination } from "swiper/modules";
import { useLanguage } from "@/lib/i18n/LanguageContext";
export default function ShopGram({ parentClass = "" }) {
  const { t } = useLanguage();
  const [products, setProducts] = useState([]);

  useEffect(() => {
    getProducts({ per_page: 5 })
      .then((res) => setProducts(res.products || []))
      .catch(() => {});
  }, []);

  if (products.length === 0) return null;

  return (
    <section className={parentClass}>
      <div className="container">
        <div className="heading-section text-center">
          <h3 className="heading wow fadeInUp">{t("homepage.shopInstagram")}</h3>
          <p className="subheading text-secondary wow fadeInUp">
            {t("homepage.instagramDesc")}
          </p>
        </div>
        <Swiper
          dir="ltr"
          className="swiper tf-sw-shop-gallery"
          spaceBetween={10}
          breakpoints={{
            1200: { slidesPerView: 5 },
            768: { slidesPerView: 3 },
            0: { slidesPerView: 2 },
          }}
          modules={[Pagination]}
          pagination={{
            clickable: true,
            el: ".spb222",
          }}
        >
          {products.map((item, i) => (
            <SwiperSlide key={item.id || i}>
              <div
                className="gallery-item hover-overlay hover-img wow fadeInUp"
              >
                <div className="img-style">
                  <Image
                    className="lazyload img-hover"
                    alt="image-gallery"
                    src={item.imgSrc}
                    width={640}
                    height={640}
                  />
                </div>
                <Link
                  href={`/product-detail/${item.id}`}
                  className="box-icon hover-tooltip"
                >
                  <span className="icon icon-eye" />
                  <span className="tooltip">{t("homepage.viewProduct")}</span>
                </Link>
              </div>
            </SwiperSlide>
          ))}
          <div className="sw-pagination-gallery sw-dots type-circle justify-content-center spb222"></div>
        </Swiper>
      </div>
    </section>
  );
}
