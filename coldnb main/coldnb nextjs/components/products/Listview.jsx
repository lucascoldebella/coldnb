import React from "react";
import ProductsCards6 from "../productCards/ProductsCards6";
import Pagination from "../common/Pagination";

export default function Listview({
  products,
  pagination = true,
  currentPage,
  totalPages,
  onPageChange,
}) {
  return (
    <>
      {/* card product list 1 */}
      {products.map((product, i) => (
        <ProductsCards6 product={product} key={i} />
      ))}
      {/* pagination */}
      {pagination && totalPages > 1 ? (
        <ul className="wg-pagination ">
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
