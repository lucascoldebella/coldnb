"use client";
import { useState, useEffect } from "react";
import axios from "axios";
import { buildApiUrl } from "@/lib/apiBase";
import {
  demoItems,
  shopLayout,
  shopFeatures,
  productStyles,
  otherShopMenus,
  productLinks,
  blogLinks,
  otherPageLinks,
} from "@/data/menu";

/**
 * Build static fallback menu structure matching the API shape.
 * Used when API is unavailable.
 */
function buildStaticFallback() {
  const toItems = (arr) =>
    arr.map((item, i) => ({
      id: i + 1,
      label: item.name,
      href: item.href,
      image_url: item.src || null,
      image_alt: item.alt || null,
      badge: item.badge || null,
      sort_order: i,
    }));

  return {
    inicio: {
      id: 1, name: "Inicio", slug: "inicio", menu_type: "mega_grid",
      show_products: false, translation_key: "nav.home",
      groups: [{ id: 1, title: null, items: toItems(demoItems) }],
    },
    loja: {
      id: 2, name: "Loja", slug: "loja", menu_type: "mega_columns",
      show_products: true, products_count: 4, translation_key: "nav.shop",
      groups: [
        { id: 2, title: "Shop Layout", translation_key: "nav.shopLayout", items: toItems(shopLayout) },
        { id: 3, title: "Shop Features", translation_key: "nav.shopFeatures", items: toItems(shopFeatures) },
        { id: 4, title: "Products Hover", translation_key: "nav.productsHover", items: toItems(productStyles) },
        { id: 5, title: "My Pages", translation_key: "nav.myPages", items: toItems(otherShopMenus) },
      ],
    },
    produtos: {
      id: 3, name: "Produtos", slug: "produtos", menu_type: "mega_columns",
      show_products: false, translation_key: "nav.products",
      banner_image_url: "/images/collections/cls-header.jpg",
      banner_link: "/shop-collection",
      banner_title: "nav.bestSeller",
      groups: [
        { id: 6, title: "Products Layout", translation_key: "nav.productsLayout", items: toItems(productLinks) },
      ],
    },
    blog: {
      id: 4, name: "Blog", slug: "blog", menu_type: "simple",
      show_products: false, translation_key: "nav.blog",
      groups: [{ id: 7, title: null, items: toItems(blogLinks) }],
    },
    paginas: {
      id: 5, name: "Paginas", slug: "paginas", menu_type: "simple",
      show_products: false, translation_key: "nav.pages",
      groups: [{ id: 8, title: null, items: toItems(otherPageLinks) }],
    },
  };
}

/**
 * Fetch navigation data from API, with static fallback.
 * Returns { menusBySlug, allItems, loading }
 */
export default function useNavigationData() {
  const [menusBySlug, setMenusBySlug] = useState(buildStaticFallback);
  const [allItems, setAllItems] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    let cancelled = false;

    async function fetchNav() {
      try {
        const res = await axios.get(buildApiUrl("/api/navigation"), { timeout: 5000 });
        const menus = res?.data?.data?.menus;
        if (!Array.isArray(menus) || menus.length === 0) {
          throw new Error("Empty response");
        }

        if (cancelled) return;

        const bySlug = {};
        const items = [];
        for (const menu of menus) {
          bySlug[menu.slug] = menu;
          for (const group of menu.groups || []) {
            for (const item of group.items || []) {
              items.push(item);
            }
          }
        }
        setMenusBySlug(bySlug);
        setAllItems(items);
      } catch {
        // Keep static fallback
      } finally {
        if (!cancelled) {
          setLoading(false);
        }
      }
    }

    fetchNav();
    return () => { cancelled = true; };
  }, []);

  return { menusBySlug, allItems, loading };
}
