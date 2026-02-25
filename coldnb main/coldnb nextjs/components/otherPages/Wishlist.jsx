"use client";

import { useContextElement } from "@/context/Context";
import { useEffect, useState } from "react";
import ProductCard1 from "../productCards/ProductCard1";
import Link from "next/link";
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
    <section className="flat-spacing">
      <div className="container">
        {loading ? (
          <div className="text-center p-5">
            <div className="spinner-border" role="status" />
          </div>
        ) : items.length ? (
          <div className="tf-grid-layout tf-col-2 md-col-3 xl-col-4">
            {items.map((product, i) => (
              <ProductCard1 key={product.id || i} product={product} />
            ))}
          </div>
        ) : (
          <div className="p-5">
            {t("wishlistPage.empty")}{" "}
            <Link className="btn-line" href="/shop-default-grid">
              {t("wishlistPage.exploreProducts")}
            </Link>
          </div>
        )}
      </div>
    </section>
  );
}
