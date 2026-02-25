"use client";

import LayoutHandler from "./LayoutHandler";
import Sorting from "./Sorting";
import Listview from "./Listview";
import GridView from "./GridView";
import { useCallback, useEffect, useReducer, useState } from "react";
import FilterModal from "./FilterModal";
import { initialState, reducer } from "@/reducer/filterReducer";
import FilterMeta from "./FilterMeta";
import { getProducts } from "@/lib/shopApi";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function Products1({ parentClass = "flat-spacing" }) {
  const { t } = useLanguage();
  const [activeLayout, setActiveLayout] = useState(4);
  const [state, dispatch] = useReducer(reducer, initialState);
  const {
    price,
    availability,
    color,
    size,
    brands,
    category,
    filtered,
    sortingOption,
    sorted,
    activeFilterOnSale,
    currentPage,
    itemPerPage,
    totalItems,
    totalPages,
    loading,
  } = state;

  const allProps = {
    ...state,
    setPrice: (value) => dispatch({ type: "SET_PRICE", payload: value }),

    setColor: (value) => {
      value == color
        ? dispatch({ type: "SET_COLOR", payload: "All" })
        : dispatch({ type: "SET_COLOR", payload: value });
    },
    setSize: (value) => {
      value == size
        ? dispatch({ type: "SET_SIZE", payload: "All" })
        : dispatch({ type: "SET_SIZE", payload: value });
    },
    setAvailability: (value) => {
      value == availability
        ? dispatch({ type: "SET_AVAILABILITY", payload: "All" })
        : dispatch({ type: "SET_AVAILABILITY", payload: value });
    },

    setBrands: (newBrand) => {
      const updated = [...brands].includes(newBrand)
        ? [...brands].filter((elm) => elm != newBrand)
        : [...brands, newBrand];
      dispatch({ type: "SET_BRANDS", payload: updated });
    },
    removeBrand: (newBrand) => {
      const updated = [...brands].filter((brand) => brand != newBrand);
      dispatch({ type: "SET_BRANDS", payload: updated });
    },
    setCategory: (value) => {
      dispatch({
        type: "SET_CATEGORY",
        payload: value === category ? null : value,
      });
    },
    setSortingOption: (value) =>
      dispatch({ type: "SET_SORTING_OPTION", payload: value }),
    toggleFilterWithOnSale: () => dispatch({ type: "TOGGLE_FILTER_ON_SALE" }),
    setCurrentPage: (value) =>
      dispatch({ type: "SET_CURRENT_PAGE", payload: value }),
    setItemPerPage: (value) => {
      dispatch({ type: "SET_CURRENT_PAGE", payload: 1 }),
        dispatch({ type: "SET_ITEM_PER_PAGE", payload: value });
    },
    clearFilter: () => {
      dispatch({ type: "CLEAR_FILTER" });
    },
  };

  // Map frontend sorting options to backend sort param
  const getSortParam = useCallback((option) => {
    switch (option) {
      case "Price Ascending":
        return "price_asc";
      case "Price Descending":
        return "price_desc";
      case "Title Ascending":
        return "name";
      case "Title Descending":
        return "name"; // Backend only supports name ASC; we'll reverse client-side if needed
      default:
        return "newest";
    }
  }, []);

  // Fetch products from API when filters change
  useEffect(() => {
    let cancelled = false;
    const fetchProducts = async () => {
      dispatch({ type: "SET_LOADING", payload: true });
      try {
        const params = {
          page: currentPage,
          per_page: itemPerPage,
          sort: getSortParam(sortingOption),
        };

        if (category) {
          params.category = category;
        }
        if (price[0] > 0) {
          params.min_price = price[0];
        }
        if (price[1] < 1000) {
          params.max_price = price[1];
        }
        if (activeFilterOnSale) {
          params.sale = "true";
        }

        const result = await getProducts(params);

        if (!cancelled) {
          let products = result.products;

          // Client-side filtering for Title Descending (backend only does ASC)
          if (sortingOption === "Title Descending") {
            products = [...products].reverse();
          }

          dispatch({
            type: "SET_PRODUCTS",
            payload: { products, pagination: result.pagination },
          });
        }
      } catch (error) {
        console.error("Failed to fetch products:", error);
        if (!cancelled) {
          dispatch({ type: "SET_LOADING", payload: false });
        }
      }
    };

    fetchProducts();
    return () => {
      cancelled = true;
    };
  }, [
    currentPage,
    itemPerPage,
    category,
    price,
    sortingOption,
    activeFilterOnSale,
    getSortParam,
  ]);

  return (
    <>
      <section className={parentClass}>
        <div className="container">
          <div className="tf-shop-control">
            <div className="tf-control-filter">
              <a
                href="#filterShop"
                data-bs-toggle="offcanvas"
                aria-controls="filterShop"
                className="tf-btn-filter"
              >
                <span className="icon icon-filter" />
                <span className="text">{t("shop.filters")}</span>
              </a>
              <div
                onClick={allProps.toggleFilterWithOnSale}
                className={`d-none d-lg-flex shop-sale-text ${
                  activeFilterOnSale ? "active" : ""
                }`}
              >
                <i className="icon icon-checkCircle" />
                <p className="text-caption-1">{t("shop.shopSaleOnly")}</p>
              </div>
            </div>
            <ul className="tf-control-layout">
              <LayoutHandler
                setActiveLayout={setActiveLayout}
                activeLayout={activeLayout}
              />
            </ul>
            <div className="tf-control-sorting">
              <p className="d-none d-lg-block text-caption-1">{t("shop.sortBy")}</p>
              <Sorting allProps={allProps} />
            </div>
          </div>
          <div className="wrapper-control-shop">
            <FilterMeta productLength={totalItems} allProps={allProps} />

            {loading ? (
              <div className="text-center py-5">
                <div className="spinner-border" role="status">
                  <span className="visually-hidden">{t("common.loading")}</span>
                </div>
              </div>
            ) : sorted.length === 0 ? (
              <div className="text-center py-5">
                <p>{t("shop.noProducts")}</p>
              </div>
            ) : activeLayout == 1 ? (
              <div className="tf-list-layout wrapper-shop" id="listLayout">
                <Listview
                  products={sorted}
                  currentPage={currentPage}
                  totalPages={totalPages}
                  onPageChange={allProps.setCurrentPage}
                />
              </div>
            ) : (
              <div
                className={`tf-grid-layout wrapper-shop tf-col-${activeLayout}`}
                id="gridLayout"
              >
                <GridView
                  products={sorted}
                  currentPage={currentPage}
                  totalPages={totalPages}
                  onPageChange={allProps.setCurrentPage}
                />
              </div>
            )}
          </div>
        </div>
      </section>

      <FilterModal allProps={allProps} />
    </>
  );
}
