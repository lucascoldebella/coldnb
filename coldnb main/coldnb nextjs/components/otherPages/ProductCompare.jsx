"use client";
import { useContextElement } from "@/context/Context";
import { getProductsByIds } from "@/lib/shopApi";
import Image from "next/image";
import Link from "next/link";
import React, { useEffect, useState } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function ProductCompare() {
  const { t } = useLanguage();
  const {
    compareItem,
    addProductToCart,
    isAddedToCartProducts,
  } = useContextElement();
  const [items, setItems] = useState([]);

  useEffect(() => {
    if (compareItem.length === 0) {
      setItems([]);
      return;
    }
    let cancelled = false;
    getProductsByIds(compareItem).then((products) => {
      if (!cancelled) setItems(products);
    }).catch(() => {});
    return () => { cancelled = true; };
  }, [compareItem]);

  return (
    <section className="flat-spacing">
      <div className="container">
        {!items.length ? (
          <div>
            {t("comparePage.empty")}{" "}
            <Link className="btn-line" href="/shop-default-grid">
              {t("wishlistPage.exploreProducts")}
            </Link>
          </div>
        ) : (
          ""
        )}
        {items.length ? (
          <div className="tf-compare-table">
            <div className="tf-compare-row tf-compare-grid">
              <div className="tf-compare-col d-md-block d-none" />
              {items.map((elm, i) => (
                <div key={i} className="tf-compare-col">
                  <div className="tf-compare-item">
                    <Link
                      className="tf-compare-image"
                      href={`/product-detail/${elm.id}`}
                    >
                      <Image
                        className="lazyload"
                        alt="img-compare"
                        src={elm.imgSrc}
                        width={600}
                        height={800}
                      />
                    </Link>
                    <div className="tf-compare-content">
                      <Link
                        className="link text-title text-line-clamp-1"
                        href={`/product-detail/${elm.id}`}
                      >
                        {elm.title}
                      </Link>
                      <p className="desc text-caption-1">
                        {elm.category || ""}
                      </p>
                    </div>
                  </div>
                </div>
              ))}
            </div>

            <div className="tf-compare-row">
              <div className="tf-compare-col tf-compare-field d-md-block d-none">
                <h6>{t("comparePage.price")}</h6>
              </div>
              {items.map((elm, i) => (
                <div
                  key={i}
                  className="tf-compare-col tf-compare-field text-center"
                >
                  <span className="price">R${elm.price.toFixed(2)}</span>
                </div>
              ))}
            </div>
            <div className="tf-compare-row">
              <div className="tf-compare-col tf-compare-field d-md-block d-none">
                <h6>{t("comparePage.brand")}</h6>
              </div>
              {items.map((elm, i) => (
                <div
                  key={i}
                  className="tf-compare-col tf-compare-field text-center"
                >
                  <span className="brand">{elm.brand || "-"}</span>
                </div>
              ))}
            </div>
            <div className="tf-compare-row">
              <div className="tf-compare-col tf-compare-field d-md-block d-none">
                <h6>{t("comparePage.size")}</h6>
              </div>
              {items.map((elm, i) => (
                <div
                  key={i}
                  className="tf-compare-col tf-compare-field text-center"
                >
                  <span className="size">{elm.sizes?.join(", ") || "-"}</span>
                </div>
              ))}
            </div>
            <div className="tf-compare-row">
              <div className="tf-compare-col tf-compare-field d-md-block d-none">
                <h6>{t("comparePage.color")}</h6>
              </div>
              {items.map((elm, i) => (
                <div
                  key={i}
                  className="tf-compare-col tf-compare-field text-center"
                >
                  <div className="list-compare-color justify-content-center">
                    {elm.colors?.length > 0 ? elm.colors.map((c, ci) => (
                      <span key={ci} className="item" style={{ backgroundColor: c.code || "#ccc" }} />
                    )) : <span>-</span>}
                  </div>
                </div>
              ))}
            </div>
            <div className="tf-compare-row">
              <div className="tf-compare-col tf-compare-field d-md-block d-none">
                <h6>{t("comparePage.addToCart")}</h6>
              </div>
              {items.map((elm, i) => (
                <div
                  key={i}
                  className="tf-compare-col tf-compare-field tf-compare-viewcart text-center"
                >
                  <a
                    className="btn-view-cart"
                    onClick={() => addProductToCart(elm)}
                  >
                    {isAddedToCartProducts(elm.id)
                      ? t("product.alreadyAdded")
                      : t("comparePage.addToCart")}
                  </a>
                </div>
              ))}
            </div>
          </div>
        ) : (
          ""
        )}
      </div>
    </section>
  );
}
