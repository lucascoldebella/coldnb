"use client";
import Link from "next/link";
import React, { useEffect, useState } from "react";
import Image from "next/image";
import { getProducts } from "@/lib/shopApi";
import { Swiper, SwiperSlide } from "swiper/react";
import ProductCard1 from "../productCards/ProductCard1";
import useNavigationData from "@/lib/hooks/useNavigationData";
import { usePathname } from "next/navigation";
import { useLanguage } from "@/lib/i18n/LanguageContext";

/* Collect all items from a menu for active-state matching */
function getAllItems(menu) {
  if (!menu || !menu.groups) return [];
  const items = [];
  for (const group of menu.groups) {
    for (const item of group.items || []) {
      items.push(item);
    }
  }
  return items;
}

function isActive(items, pathname) {
  return items.some((item) => item.href.split("/")[1] === pathname.split("/")[1]);
}

/* ---- Mega Grid Menu (Inicio) ---- */
function MegaGridMenu({ menu, pathname, t }) {
  const items = getAllItems(menu);
  return (
    <li className={`menu-item ${isActive(items, pathname) ? "active" : ""}`}>
      <a href="#" className="item-link">
        {t(menu.translation_key || "nav.home")}
        <i className="icon icon-arrow-down" />
      </a>
      <div className="sub-menu mega-menu">
        <div className="container">
          <div className="row-demo">
            {items.slice(0, 12).map((item) => (
              <div
                className={`demo-item ${pathname.split("/")[1] === item.href.split("/")[1] ? "active" : ""}`}
                key={item.id || item.href}
              >
                <Link href={item.href}>
                  <div className="demo-image position-relative">
                    {item.image_url && (
                      <Image
                        className="lazyload"
                        data-src={item.image_url}
                        alt={item.image_alt || item.label}
                        src={item.image_url}
                        width={273}
                        height={300}
                      />
                    )}
                    {item.badge && (
                      <div className="demo-label">
                        <span className={`demo-${item.badge.toLowerCase()}`}>{item.badge}</span>
                      </div>
                    )}
                  </div>
                  <span className="demo-name">{item.label}</span>
                </Link>
              </div>
            ))}
          </div>
          {items.length > 12 && (
            <div className="text-center view-all-demo">
              <a href="#modalDemo" data-bs-toggle="modal" className="tf-btn">
                <span className="text">{t("nav.viewAllDemos")}</span>
              </a>
            </div>
          )}
        </div>
      </div>
    </li>
  );
}

/* ---- Mega Columns Menu (Loja, Produtos) ---- */
function MegaColumnsMenu({ menu, pathname, t, navProducts }) {
  const items = getAllItems(menu);
  const groups = menu.groups || [];
  const showProducts = menu.show_products;
  const hasBanner = !!menu.banner_image_url;

  // Calculate column width based on number of groups + extras
  const groupCount = groups.length;
  const extraCols = (showProducts ? 1 : 0) + (hasBanner ? 1 : 0);
  const totalCols = groupCount + extraCols;
  const groupColSize = totalCols <= 4 ? Math.floor(12 / totalCols) : 2;
  const productColSize = showProducts ? 4 : 0;
  const bannerColSize = hasBanner ? (12 - groupCount * groupColSize) : 0;

  return (
    <li className={`menu-item ${isActive(items, pathname) ? "active" : ""}`}>
      <a href="#" className="item-link">
        {t(menu.translation_key || menu.name)}
        <i className="icon icon-arrow-down" />
      </a>
      <div className="sub-menu mega-menu">
        <div className="container">
          <div className="row">
            {groups.map((group) => (
              <div key={group.id} className={`col-lg-${showProducts ? 2 : groupColSize}`}>
                <div className="mega-menu-item">
                  {group.title && (
                    <div className="menu-heading">
                      {group.translation_key ? t(group.translation_key) : group.title}
                    </div>
                  )}
                  <ul className="menu-list">
                    {(group.items || []).map((item) => (
                      <li
                        key={item.id || item.href}
                        className={`menu-item-li ${pathname.split("/")[1] === item.href.split("/")[1] ? "active" : ""}`}
                      >
                        <Link
                          href={item.href}
                          className={`menu-link-text ${item.badge ? "position-relative" : ""}`}
                        >
                          {item.label}
                          {item.badge && (
                            <div className="demo-label">
                              <span className="demo-new">{item.badge}</span>
                            </div>
                          )}
                        </Link>
                      </li>
                    ))}
                  </ul>
                </div>
              </div>
            ))}

            {/* Product carousel (Loja) */}
            {showProducts && navProducts.length > 0 && (
              <div className={`col-lg-${productColSize}`}>
                <div className="wrapper-sub-shop">
                  <div className="menu-heading">{t("nav.recentProducts")}</div>
                  <Swiper
                    dir="ltr"
                    className="swiper tf-product-header"
                    slidesPerView={2}
                    spaceBetween={20}
                  >
                    {navProducts
                      .map((elm) => ({ ...elm, colors: null }))
                      .map((elm, i) => (
                        <SwiperSlide key={i} className="swiper-slide">
                          <ProductCard1 product={elm} />
                        </SwiperSlide>
                      ))}
                  </Swiper>
                </div>
              </div>
            )}

            {/* Banner column (Produtos) */}
            {hasBanner && !showProducts && (
              <div className={`col-lg-${bannerColSize}`}>
                <div className="menu-heading">
                  {menu.banner_title ? t(menu.banner_title) : t("nav.bestSeller")}
                </div>
                <div className="sec-cls-header">
                  <div className="collection-position hover-img">
                    <Link href={menu.banner_link || "/shop-collection"} className="img-style">
                      <Image
                        className="lazyload"
                        data-src={menu.banner_image_url}
                        alt="banner-cls"
                        src={menu.banner_image_url}
                        width={300}
                        height={400}
                      />
                    </Link>
                    <div className="content">
                      <div className="title-top">
                        <h4 className="title">
                          <Link href={menu.banner_link || "/shop-collection"} className="link text-white wow fadeInUp">
                            {t("nav.shopOurTopPicks")}
                          </Link>
                        </h4>
                        <p className="desc text-white wow fadeInUp">
                          {t("nav.reservedForSpecial")}
                        </p>
                      </div>
                      <div>
                        <Link href={menu.banner_link || "/shop-collection"} className="tf-btn btn-md btn-white">
                          <span className="text">{t("nav.shopNow")}</span>
                          <i className="icon icon-arrowUpRight" />
                        </Link>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            )}
          </div>
        </div>
      </div>
    </li>
  );
}

/* ---- Simple Dropdown Menu (Blog, Paginas) ---- */
function SimpleMenu({ menu, pathname, t }) {
  const items = getAllItems(menu);
  return (
    <li className={`menu-item position-relative ${isActive(items, pathname) ? "active" : ""}`}>
      <a href="#" className="item-link">
        {t(menu.translation_key || menu.name)}
        <i className="icon icon-arrow-down" />
      </a>
      <div className="sub-menu submenu-default">
        <ul className="menu-list">
          {items.map((item) => (
            <li
              key={item.id || item.href}
              className={`menu-item-li ${pathname.split("/")[1] === item.href.split("/")[1] ? "active" : ""}`}
            >
              <Link href={item.href} className="menu-link-text">
                {item.label}
              </Link>
            </li>
          ))}
        </ul>
      </div>
    </li>
  );
}

export default function Nav() {
  const pathname = usePathname();
  const { t } = useLanguage();
  const { menusBySlug } = useNavigationData();
  const [navProducts, setNavProducts] = useState([]);

  useEffect(() => {
    getProducts({ per_page: 4 })
      .then((res) => setNavProducts(res.products || []))
      .catch(() => {});
  }, []);

  // Render menus in order: inicio, loja, produtos, blog, paginas
  const menuOrder = ["inicio", "loja", "produtos", "blog", "paginas"];

  return (
    <>
      {menuOrder.map((slug) => {
        const menu = menusBySlug[slug];
        if (!menu) return null;

        switch (menu.menu_type) {
          case "mega_grid":
            return <MegaGridMenu key={slug} menu={menu} pathname={pathname} t={t} />;
          case "mega_columns":
            return <MegaColumnsMenu key={slug} menu={menu} pathname={pathname} t={t} navProducts={navProducts} />;
          case "simple":
            return <SimpleMenu key={slug} menu={menu} pathname={pathname} t={t} />;
          default:
            return <SimpleMenu key={slug} menu={menu} pathname={pathname} t={t} />;
        }
      })}
    </>
  );
}
