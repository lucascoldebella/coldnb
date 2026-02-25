"use client";

import { collections as staticCollections } from "@/data/collections";
import { Swiper, SwiperSlide } from "swiper/react";
import Image from "next/image";
import Link from "next/link";
import { Navigation, Pagination } from "swiper/modules";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function Collections({ data }) {
  const { t } = useLanguage();
  const items = data && data.length > 0
    ? data.map((cat) => ({
        imgSrc: cat.image_url || "/images/collections/collection-circle/cls-circle1.jpg",
        alt: cat.name,
        title: cat.name,
        count: `${cat.product_count || 0} ${t("homepage.items")}`,
        slug: cat.slug,
      }))
    : staticCollections;

  return (
    <section className="flat-spacing-2 pb_0">
      <div className="container">
        <div className="heading-section-2 wow fadeInUp">
          <h3>{t("homepage.categoriesYouMightLike")}</h3>
          <Link href={`/shop-collection`} className="btn-line">
            {t("homepage.viewAllCollection")}
          </Link>
        </div>
        <div
          className="flat-collection-circle wow fadeInUp"
          data-wow-delay="0.1s"
        >
          <Swiper
            dir="ltr"
            slidesPerView={5}
            spaceBetween={20}
            breakpoints={{
              1200: { slidesPerView: 5, spaceBetween: 20 },
              1000: { slidesPerView: 4, spaceBetween: 20 },
              768: { slidesPerView: 3, spaceBetween: 20 },
              480: { slidesPerView: 2, spaceBetween: 15 },
              0: { slidesPerView: 2, spaceBetween: 15 },
            }}
            modules={[Pagination, Navigation]}
            pagination={{
              clickable: true,
              el: ".spd54",
            }}
            navigation={{
              prevEl: ".snbp12",
              nextEl: ".snbn12",
            }}
          >
            {items.map((collection, index) => (
              <SwiperSlide key={index}>
                <div className="collection-circle hover-img">
                  <Link href={collection.slug ? `/shop-collection?category=${collection.slug}` : `/shop-collection`} className="img-style" style={{ display: 'block', aspectRatio: '1', borderRadius: '50%', overflow: 'hidden' }}>
                    <Image
                      className="lazyload"
                      data-src={collection.imgSrc}
                      alt={collection.alt}
                      src={collection.imgSrc}
                      width={363}
                      height={363}
                      style={{ width: '100%', height: '100%', objectFit: 'cover' }}
                    />
                  </Link>
                  <div className="collection-content text-center">
                    <div>
                      <Link href={collection.slug ? `/shop-collection?category=${collection.slug}` : `/shop-collection`} className="cls-title">
                        <h6 className="text">{collection.title}</h6>
                        <i className="icon icon-arrowUpRight" />
                      </Link>
                    </div>
                    <div className="count text-secondary">
                      {collection.count}
                    </div>
                  </div>
                </div>
              </SwiperSlide>
            ))}
          </Swiper>
          <div className="d-flex d-lg-none sw-pagination-collection sw-dots type-circle justify-content-center spd54" />
          <div className="nav-prev-collection d-none d-lg-flex nav-sw style-line nav-sw-left snbp12">
            <i className="icon icon-arrLeft" />
          </div>
          <div className="nav-next-collection d-none d-lg-flex nav-sw style-line nav-sw-right snbn12">
            <i className="icon icon-arrRight" />
          </div>
        </div>
      </div>
    </section>
  );
}
