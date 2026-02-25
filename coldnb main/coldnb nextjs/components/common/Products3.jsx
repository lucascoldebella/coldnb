"use client";
import ProductCard1 from "@/components/productCards/ProductCard1";
import { getProducts } from "@/lib/shopApi";
import React, { useEffect, useState, useMemo } from "react";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";

const defaultTabs = ["New Arrivals", "Best Seller", "On Sale"];

export default function Products3({ parentClass = "flat-spacing-3", data }) {
  const { t } = useLanguage();
  const tabItems = useMemo(() => {
    if (data?.config?.tabs) {
      return data.config.tabs.map((tab) => tab.name);
    }
    return defaultTabs;
  }, [data]);

  const [activeItem, setActiveItem] = useState(tabItems[0]);
  const [selectedItems, setSelectedItems] = useState([]);
  const [animating, setAnimating] = useState(false);

  useEffect(() => {
    setAnimating(true);
    const timer = setTimeout(() => {
      if (data?.tab_products && data.tab_products[activeItem]?.length > 0) {
        setSelectedItems(
          data.tab_products[activeItem].map((p) => ({
            id: p.id,
            title: p.name,
            price: parseFloat(p.price) || 0,
            oldPrice: p.compare_at_price ? parseFloat(p.compare_at_price) : null,
            imgSrc: p.primary_image_url || "/images/products/default.jpg",
            slug: p.slug,
            isNew: p.is_new,
            isSale: p.is_sale,
          }))
        );
      } else {
        // Fetch from API when no homepage data is available
        getProducts({ per_page: 8 })
          .then((res) => setSelectedItems(res.products || []))
          .catch(() => setSelectedItems([]));
      }
      setAnimating(false);
    }, 300);
    return () => clearTimeout(timer);
  }, [activeItem, data]);

  return (
    <section className={parentClass}>
      <div className="container">
        <div className="flat-animate-tab">
          <ul className="tab-product justify-content-sm-center" role="tablist">
            {tabItems.map((item) => (
              <li key={item} className="nav-tab-item">
                <a
                  href="#"
                  className={activeItem === item ? "active" : ""}
                  onClick={(e) => {
                    e.preventDefault();
                    setActiveItem(item);
                  }}
                >
                  {item}
                </a>
              </li>
            ))}
          </ul>
          <div className="tab-content">
            <div
              className={`tab-pane active show tabFilter ${animating ? "" : "filtered"}`}
              id="newArrivals"
              role="tabpanel"
            >
              <div className="tf-grid-layout tf-col-2 lg-col-3 xl-col-4">
                {selectedItems.map((product, i) => (
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
