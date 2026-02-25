"use client";

import { useEffect, useState } from "react";
import {
  availabilityOptions,
  brands,
  colors,
  sizes,
} from "@/data/productFilterOptions";
import { getCategories } from "@/lib/shopApi";
import { useLanguage } from "@/lib/i18n/LanguageContext";

import RangeSlider from "react-range-slider-input";
export default function FilterModal({ allProps }) {
  const { t } = useLanguage();
  const [categories, setCategories] = useState([]);

  useEffect(() => {
    getCategories()
      .then((cats) => setCategories(cats))
      .catch(() => setCategories([]));
  }, []);

  return (
    <div className="offcanvas offcanvas-start canvas-filter" id="filterShop">
      <div className="canvas-wrapper">
        <div className="canvas-header">
          <h5>{t("shop.filters")}</h5>
          <span
            className="icon-close icon-close-popup"
            data-bs-dismiss="offcanvas"
            aria-label="Close"
          />
        </div>
        <div className="canvas-body">
          <div className="widget-facet facet-categories">
            <h6 className="facet-title">{t("shop.productCategories")}</h6>
            <ul className="facet-content">
              <li>
                <a
                  href="#"
                  className={`categories-item ${!allProps.category ? "fw-bold" : ""}`}
                  onClick={(e) => {
                    e.preventDefault();
                    allProps.setCategory(null);
                  }}
                >
                  {t("shop.allProducts")}
                </a>
              </li>
              {categories.map((cat, index) => (
                <li key={index}>
                  <a
                    href="#"
                    className={`categories-item ${allProps.category === cat.slug ? "fw-bold" : ""}`}
                    onClick={(e) => {
                      e.preventDefault();
                      allProps.setCategory(cat.slug);
                    }}
                  >
                    {cat.name}
                  </a>
                </li>
              ))}
            </ul>
          </div>
          <div className="widget-facet facet-price">
            <h6 className="facet-title">{t("shop.price")}</h6>

            <RangeSlider
              min={0}
              max={1000}
              value={allProps.price}
              onInput={(value) => allProps.setPrice(value)}
            />
            <div className="box-price-product mt-3">
              <div className="box-price-item">
                <span className="title-price">{t("shop.minPrice")}</span>
                <div
                  className="price-val"
                  id="price-min-value"
                  data-currency="$"
                >
                  {allProps.price[0]}
                </div>
              </div>
              <div className="box-price-item">
                <span className="title-price">{t("shop.maxPrice")}</span>
                <div
                  className="price-val"
                  id="price-max-value"
                  data-currency="$"
                >
                  {allProps.price[1]}
                </div>
              </div>
            </div>
          </div>
          <div className="widget-facet facet-size">
            <h6 className="facet-title">{t("shop.size")}</h6>
            <div className="facet-size-box size-box">
              {sizes.map((size, index) => (
                <span
                  key={index}
                  onClick={() => allProps.setSize(size)}
                  className={`size-item size-check ${
                    allProps.size === size ? "active" : ""
                  }`}
                >
                  {size}
                </span>
              ))}
              <span
                className={`size-item size-check free-size ${
                  allProps.size == "Free Size" ? "active" : ""
                } `}
                onClick={() => allProps.setSize("Free Size")}
              >
                {t("shop.freeSize")}
              </span>
            </div>
          </div>
          <div className="widget-facet facet-color">
            <h6 className="facet-title">{t("shop.colors")}</h6>
            <div className="facet-color-box">
              {colors.map((color, index) => (
                <div
                  onClick={() => allProps.setColor(color)}
                  key={index}
                  className={`color-item color-check ${
                    color == allProps.color ? "active" : ""
                  }`}
                >
                  <span className={`color ${color.className}`} />
                  {color.name}
                </div>
              ))}
            </div>
          </div>
          <div className="widget-facet facet-fieldset">
            <h6 className="facet-title">{t("shop.availability")}</h6>
            <div className="box-fieldset-item">
              {availabilityOptions.map((option, index) => (
                <fieldset
                  key={index}
                  className="fieldset-item"
                  onClick={() => allProps.setAvailability(option)}
                >
                  <input
                    type="radio"
                    name="availability"
                    className="tf-check"
                    readOnly
                    checked={allProps.availability === option}
                  />
                  <label>
                    {option.label}
                  </label>
                </fieldset>
              ))}
            </div>
          </div>
          <div className="widget-facet facet-fieldset">
            <h6 className="facet-title">{t("shop.brands")}</h6>
            <div className="box-fieldset-item">
              {brands.map((brand, index) => (
                <fieldset
                  key={index}
                  className="fieldset-item"
                  onClick={() => allProps.setBrands(brand.label)}
                >
                  <input
                    type="checkbox"
                    name="brand"
                    className="tf-check"
                    readOnly
                    checked={allProps.brands.includes(brand.label)}
                  />
                  <label>
                    {brand.label}
                  </label>
                </fieldset>
              ))}
            </div>
          </div>
        </div>
        <div className="canvas-bottom">
          <button
            id="reset-filter"
            onClick={allProps.clearFilter}
            className="tf-btn btn-reset"
          >
            {t("shop.resetFilters")}
          </button>
        </div>
      </div>
    </div>
  );
}
