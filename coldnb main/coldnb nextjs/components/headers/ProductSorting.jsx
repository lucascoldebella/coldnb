import React, { useState } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function ProductSorting() {
  const { t } = useLanguage();
  const [selectedOption, setSelectedOption] = useState(t("shop.superSale"));

  const handleSelect = (value) => {
    setSelectedOption(value);
  };

  return (
    <div className="tf-dropdown-sort" data-bs-toggle="dropdown">
      <div className="btn-select">
        <span className="icon icon-tag" />
        <span className="text-sort-value">{selectedOption}</span>
        <span className="icon icon-arrow-down" />
      </div>
      <div className="dropdown-menu">
        {[
          t("shop.superSale"),
          t("shop.bestSelling"),
          t("shop.alphaAZ"),
          t("shop.alphaZA"),
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
  );
}
