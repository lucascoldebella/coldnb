"use client";
import React, { useState, useCallback } from "react";

import ProductCard1 from "../productCards/ProductCard1";
import { searchProducts } from "@/lib/shopApi";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function SearchModal() {
  const { t } = useLanguage();
  const [loading, setLoading] = useState(false);
  const [searchTerm, setSearchTerm] = useState("");
  const [results, setResults] = useState([]);
  const [hasSearched, setHasSearched] = useState(false);

  const handleSearch = useCallback(
    async (e) => {
      e.preventDefault();
      if (!searchTerm.trim()) return;

      setLoading(true);
      setHasSearched(true);
      try {
        const products = await searchProducts(searchTerm.trim());
        setResults(products);
      } catch (error) {
        console.error("Search failed:", error);
        setResults([]);
      } finally {
        setLoading(false);
      }
    },
    [searchTerm]
  );

  return (
    <div className="modal fade modal-search" id="search">
      <div className="modal-dialog modal-dialog-centered">
        <div className="modal-content">
          <div className="d-flex justify-content-between align-items-center">
            <h5>{t("search.title")}</h5>
            <span
              className="icon-close icon-close-popup"
              data-bs-dismiss="modal"
            />
          </div>
          <form className="form-search" onSubmit={handleSearch}>
            <fieldset className="text">
              <input
                type="text"
                placeholder={t("search.placeholder")}
                className=""
                name="text"
                tabIndex={0}
                value={searchTerm}
                onChange={(e) => setSearchTerm(e.target.value)}
                aria-required="true"
                required
              />
            </fieldset>
            <button className="" type="submit">
              <svg
                className="icon"
                width={20}
                height={20}
                viewBox="0 0 24 24"
                fill="none"
                xmlns="http://www.w3.org/2000/svg"
              >
                <path
                  d="M11 19C15.4183 19 19 15.4183 19 11C19 6.58172 15.4183 3 11 3C6.58172 3 3 6.58172 3 11C3 15.4183 6.58172 19 11 19Z"
                  stroke="#181818"
                  strokeWidth={2}
                  strokeLinecap="round"
                  strokeLinejoin="round"
                />
                <path
                  d="M21.35 21.0004L17 16.6504"
                  stroke="#181818"
                  strokeWidth={2}
                  strokeLinecap="round"
                  strokeLinejoin="round"
                />
              </svg>
            </button>
          </form>
          {!hasSearched && (
            <div>
              <h5 className="mb_16">{t("search.popularSearches")}</h5>
              <ul className="list-tags">
                <li>
                  <a
                    href="#"
                    className="radius-60 link"
                    onClick={(e) => {
                      e.preventDefault();
                      setSearchTerm("ring");
                    }}
                  >
                    {t("search.rings")}
                  </a>
                </li>
                <li>
                  <a
                    href="#"
                    className="radius-60 link"
                    onClick={(e) => {
                      e.preventDefault();
                      setSearchTerm("necklace");
                    }}
                  >
                    {t("search.necklaces")}
                  </a>
                </li>
                <li>
                  <a
                    href="#"
                    className="radius-60 link"
                    onClick={(e) => {
                      e.preventDefault();
                      setSearchTerm("bracelet");
                    }}
                  >
                    {t("search.bracelets")}
                  </a>
                </li>
                <li>
                  <a
                    href="#"
                    className="radius-60 link"
                    onClick={(e) => {
                      e.preventDefault();
                      setSearchTerm("earring");
                    }}
                  >
                    {t("search.earrings")}
                  </a>
                </li>
              </ul>
            </div>
          )}
          {loading && (
            <div className="text-center py-4">
              <div className="spinner-border" role="status">
                <span className="visually-hidden">{t("search.loading")}</span>
              </div>
            </div>
          )}
          {hasSearched && !loading && (
            <div>
              <h6 className="mb_16">
                {results.length > 0
                  ? `${results.length} ${t("search.resultsFound")}`
                  : t("search.noProductsFound")}
              </h6>
              {results.length > 0 && (
                <div className="tf-grid-layout tf-col-2 lg-col-3 xl-col-4">
                  {results.map((product, i) => (
                    <ProductCard1 product={product} key={i} />
                  ))}
                </div>
              )}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
