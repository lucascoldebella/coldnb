"use client";
import React, { useEffect, useState } from "react";
import Link from "next/link";
import Image from "next/image";
import { useContextElement } from "@/context/Context";
import { getProductsByIds } from "@/lib/shopApi";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function Wishlist() {
  const { t } = useLanguage();
  const { removeFromWishlist, wishList } = useContextElement();
  const [items, setItems] = useState([]);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    if (wishList.length === 0) {
      setItems([]);
      return;
    }
    let cancelled = false;
    setLoading(true);
    getProductsByIds(wishList).then((products) => {
      if (!cancelled) {
        setItems(products);
        setLoading(false);
      }
    }).catch(() => {
      if (!cancelled) setLoading(false);
    });
    return () => { cancelled = true; };
  }, [wishList]);

  return (
    <div className="modal fullRight fade modal-wishlist" id="wishlist">
      <div className="modal-dialog">
        <div className="modal-content">
          <div className="header">
            <h5 className="title">{t("wishlistPage.title")}</h5>
            <span
              className="icon-close icon-close-popup"
              data-bs-dismiss="modal"
            />
          </div>
          <div className="wrap">
            <div className="tf-mini-cart-wrap">
              <div className="tf-mini-cart-main">
                <div className="tf-mini-cart-sroll">
                  {loading ? (
                    <div className="p-4 text-center">
                      <div className="spinner-border spinner-border-sm" role="status" />
                    </div>
                  ) : items.length ? (
                    <div className="tf-mini-cart-items">
                      {items.map((elm, i) => (
                        <div key={i} className="tf-mini-cart-item file-delete">
                          <div className="tf-mini-cart-image">
                            <Image
                              className="lazyload"
                              alt=""
                              src={elm.imgSrc}
                              width={600}
                              height={800}
                            />
                          </div>
                          <div className="tf-mini-cart-info flex-grow-1">
                            <div className="mb_12 d-flex align-items-center justify-content-between flex-wrap gap-12">
                              <div className="text-title">
                                <Link
                                  href={`/product-detail/${elm.id}`}
                                  className="link text-line-clamp-1"
                                >
                                  {elm.title}
                                </Link>
                              </div>
                              <div
                                className="text-button tf-btn-remove remove"
                                onClick={() => removeFromWishlist(elm.id)}
                              >
                                {t("wishlistPage.remove")}
                              </div>
                            </div>
                            <div className="d-flex align-items-center justify-content-between flex-wrap gap-12">
                              <div className="text-secondary-2">
                                {elm.sizes?.[0] || ""}{elm.colors?.[0]?.name ? `/${elm.colors[0].name}` : ""}
                              </div>
                              <div className="text-button">
                                R${elm.price.toFixed(2)}
                              </div>
                            </div>
                          </div>
                        </div>
                      ))}
                    </div>
                  ) : (
                    <div className="p-4">
                      {t("wishlistPage.empty")}{" "}
                      <Link className="btn-line" href="/shop-default-grid">
                        {t("wishlistPage.exploreProducts")}
                      </Link>
                    </div>
                  )}
                </div>
              </div>
              <div className="tf-mini-cart-bottom">
                <Link
                  href={`/wish-list`}
                  className="btn-style-2 w-100 radius-4 view-all-wishlist"
                >
                  <span className="text-btn-uppercase">{t("wishlistPage.viewAll")}</span>
                </Link>
                <Link
                  href={`/shop-default-grid`}
                  className="text-btn-uppercase"
                >
                  {t("cart.continueShopping")}
                </Link>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
