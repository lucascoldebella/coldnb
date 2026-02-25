import React, { useState } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function ProductSearch() {
  const { t } = useLanguage();
  const [selectedOption, setSelectedOption] = useState(t("shop.all"));

  const handleSelect = (value) => {
    setSelectedOption(value);
  };

  return (
    <form onSubmit={(e) => e.preventDefault()}>
      <div className="form-search-select">
        <div className="tf-dropdown-sort" data-bs-toggle="dropdown">
          <div className="btn-select">
            <span className="text-sort-value">{selectedOption}</span>
            <span className="icon icon-arrow-down" />
          </div>
          <div className="dropdown-menu">
            {[
              t("shop.all"),
              t("shop.bestSelling"),
              t("shop.alphaAZ"),
              t("shop.alphaZA"),
              t("shop.priceLowHigh"),
              t("shop.priceHighLow"),
              t("shop.dateOldNew"),
              t("shop.dateNewOld"),
            ].map((option) => (
              <div
                key={option}
                className={`select-item ${
                  selectedOption === option ? "active" : ""
                }`}
                onClick={() => handleSelect(option)}
              >
                <span className="text-value-item">{option}</span>
              </div>
            ))}
          </div>
        </div>
        <input type="text" placeholder={t("shop.searchPlaceholder")} />
        <button className="tf-btn">
          <span className="text">{t("shop.searchButton")}</span>
        </button>
      </div>
    </form>
  );
}
