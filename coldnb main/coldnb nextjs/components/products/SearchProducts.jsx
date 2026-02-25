"use client";
import React, { useEffect, useState } from "react";
import { Pagination } from "swiper/modules";
import { Swiper, SwiperSlide } from "swiper/react";
import ProductCard1 from "../productCards/ProductCard1";
import Link from "next/link";
import { getProducts } from "@/lib/shopApi";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function SearchProducts() {
  const { t } = useLanguage();
  const [products, setProducts] = useState([]);

  useEffect(() => {
    getProducts({ per_page: 4 })
      .then((res) => setProducts(res.products || []))
      .catch(() => {});
  }, []);

  return (
    <>
      {/* search */}
      <section className="flat-spacing page-search-inner">
        <div className="container">
          <div className="row justify-content-center">
            <div className="col-xl-6">
              <form
                className="form-search"
                onSubmit={(e) => e.preventDefault()}
              >
                <fieldset className="text">
                  <input
                    type="text"
                    placeholder={t("search.searching")}
                    className=""
                    name="text"
                    tabIndex={0}
                    defaultValue=""
                    aria-required="true"
                    required
                  />
                </fieldset>
                <button className="" type="submit">
                  <svg
                    className="icon"
                    width={20}
                    height={20}
                    viewBox="0 0 24 24"
                    fill="none"
                    xmlns="http://www.w3.org/2000/svg"
                  >
                    <path
                      d="M11 19C15.4183 19 19 15.4183 19 11C19 6.58172 15.4183 3 11 3C6.58172 3 3 6.58172 3 11C3 15.4183 6.58172 19 11 19Z"
                      stroke="#181818"
                      strokeWidth={2}
                      strokeLinecap="round"
                      strokeLinejoin="round"
                    />
                    <path
                      d="M21.35 21.0004L17 16.6504"
                      stroke="#181818"
                      strokeWidth={2}
                      strokeLinecap="round"
                      strokeLinejoin="round"
                    />
                  </svg>
                </button>
              </form>
              <div className="tf-col-quicklink">
                <span className="title">{t("shop.quickLink")}</span>
                <Link className="link" href={`/shop-default-grid`}>
                  {t("blog.fashion")}
                </Link>
              </div>
            </div>
          </div>
        </div>
      </section>
      {/* Top pick */}
      {products.length > 0 && (
        <section className="flat-spacing pt-0">
          <div className="container">
            <div className="heading-section text-center wow fadeInUp">
              <h3 className="heading">{t("shop.productRecent")}</h3>
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
                el: ".spd4",
              }}
            >
              {products.map((product, i) => (
                <SwiperSlide key={product.id || i} className="swiper-slide">
                  <ProductCard1 product={product} />
                </SwiperSlide>
              ))}

              <div className="sw-pagination-latest spd4 sw-dots type-circle justify-content-center" />
            </Swiper>
          </div>
        </section>
      )}
    </>
  );
}
