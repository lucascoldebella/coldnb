"use client";
import ProductCard1 from "@/components/productCards/ProductCard1";
import { getProducts } from "@/lib/shopApi";
import React, { useEffect, useState } from "react";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";
export default function Products({ parentClass = "flat-spacing-3 pt-0" }) {
  const { t } = useLanguage();
  const [products, setProducts] = useState([]);

  useEffect(() => {
    getProducts({ per_page: 8 })
      .then((res) => setProducts(res.products || []))
      .catch(() => {});
  }, []);

  return (
    <section className={parentClass}>
      <div className="container">
        <div className="heading-section text-center wow fadeInUp">
          <h3>{t("homepage.todaysTopPicks")}</h3>
        </div>
        <div className="flat-animate-tab">
          <div className="tab-content">
            <div
              className="tab-pane active show tabFilter filtered"
              id="newArrivals2"
              role="tabpanel"
            >
              <div className="tf-grid-layout tf-col-2 lg-col-3 xl-col-4">
                {products.map((product, i) => (
                  <ProductCard1 key={product.id || i} product={product} />
                ))}
              </div>
              <div className="sec-btn text-center">
                <Link href={`/shop-default-grid`} className="btn-line">
                  {t("homepage.viewAllProducts")}
                </Link>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
