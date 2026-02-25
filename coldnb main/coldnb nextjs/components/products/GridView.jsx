import React from "react";
import ProductCard1 from "../productCards/ProductCard1";
import Pagination from "../common/Pagination";

export default function GridView({
  products,
  pagination = true,
  currentPage,
  totalPages,
  onPageChange,
}) {
  return (
    <>
      {products.map((product, index) => (
        <ProductCard1 key={index} product={product} gridClass="grid" />
      ))}
      {/* pagination */}
      {pagination && totalPages > 1 ? (
        <ul className="wg-pagination justify-content-center">
          <Pagination
            currentPage={currentPage}
            totalPages={totalPages}
            onPageChange={onPageChange}
          />
        </ul>
      ) : (
        ""
      )}
    </>
  );
}
