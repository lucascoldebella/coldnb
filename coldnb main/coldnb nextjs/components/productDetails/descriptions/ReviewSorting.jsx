import React, { useState } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function ReviewSorting() {
  const { t } = useLanguage();
  const [selectedOption, setSelectedOption] = useState(t("reviews.mostRecent"));

  const handleSelect = (value) => {
    setSelectedOption(value);
  };

  return (
    <div className="tf-dropdown-sort" data-bs-toggle="dropdown">
      <div className="btn-select">
        <span className="text-sort-value">{selectedOption}</span>
        <span className="icon icon-arrow-down" />
      </div>
      <div className="dropdown-menu">
        {[t("reviews.mostRecent"), t("reviews.oldest"), t("reviews.mostPopular")].map((option) => (
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
