"use client";
import React, { useState } from "react";

export default function Pagination({
  totalPages: externalTotalPages,
  currentPage: externalCurrentPage,
  onPageChange,
}) {
  // Fallback to internal state when no external props are provided
  const [internalPage, setInternalPage] = useState(1);
  const totalPages = externalTotalPages || 3;
  const currentPage = externalCurrentPage || internalPage;

  const handlePageClick = (page) => {
    if (page >= 1 && page <= totalPages) {
      if (onPageChange) {
        onPageChange(page);
      } else {
        setInternalPage(page);
      }
    }
  };

  const renderPageNumbers = () => {
    // Show max 5 page numbers with ellipsis for large page counts
    const pages = [];
    let start = Math.max(1, currentPage - 2);
    let end = Math.min(totalPages, start + 4);
    if (end - start < 4) {
      start = Math.max(1, end - 4);
    }

    for (let page = start; page <= end; page++) {
      pages.push(
        <li
          key={page}
          className={page === currentPage ? "active" : ""}
          onClick={() => handlePageClick(page)}
        >
          <div className="pagination-item text-button">{page}</div>
        </li>
      );
    }
    return pages;
  };

  return (
    <>
      <li onClick={() => handlePageClick(currentPage - 1)}>
        <a
          className={`pagination-item text-button ${
            currentPage === 1 ? "disabled" : ""
          }`}
        >
          <i className="icon-arrLeft" />
        </a>
      </li>
      {renderPageNumbers()}
      <li onClick={() => handlePageClick(currentPage + 1)}>
        <a
          className={`pagination-item text-button ${
            currentPage === totalPages ? "disabled" : ""
          }`}
        >
          <i className="icon-arrRight" />
        </a>
      </li>
    </>
  );
}
