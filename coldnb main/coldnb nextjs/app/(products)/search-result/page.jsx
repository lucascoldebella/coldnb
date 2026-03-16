import Footer1 from "@/components/footers/Footer1";
import Header1 from "@/components/headers/Header1";
import Topbar6 from "@/components/headers/Topbar6";
import SearchProducts from "@/components/products/SearchProducts";
import React, { Suspense } from "react";

export const metadata = {
  title: "Search Products - Coldnb",
  description: "Search our jewelry collection. Find rings, necklaces, earrings and more.",
};

export default function SearchResultPage() {
  return (
    <>
      <Topbar6 bgColor="bg-main" />
      <Header1 />
      <div
        className="page-title"
        style={{ backgroundImage: "url(/images/section/page-title.jpg)" }}
      >
        <div className="container-full">
          <div className="row">
            <div className="col-12">
              <h3 className="heading text-center">Search</h3>
            </div>
          </div>
        </div>
      </div>
      <Suspense fallback={<div className="flat-spacing text-center"><div className="spinner-border" /></div>}>
        <SearchProducts />
      </Suspense>

      <Footer1 />
    </>
  );
}
