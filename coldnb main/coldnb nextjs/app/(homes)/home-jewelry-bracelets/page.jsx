import Features from "@/components/common/Features";
import Products3 from "@/components/common/Products3";
import ShopGram from "@/components/common/ShopGram2";
import Testimonials from "@/components/common/Testimonials";
import Footer1 from "@/components/footers/Footer1";
import Header3 from "@/components/headers/Header3";
import Topbar3 from "@/components/headers/Topbar3";
import Blogs from "@/components/homes/Jewelry-classyCove/Blogs";
import Categories from "@/components/homes/Jewelry-classyCove/Categories";
import Collections from "@/components/homes/Jewelry-classyCove/Collections";
import Hero from "@/components/homes/Jewelry-classyCove/Hero";
import React from "react";

export const metadata = {
  title:
    "Home Jewelry Classycove || ColdnbMain - eCommerce",
  description: "ColdnbMain - eCommerce",
};

export default function HomeJewelryClassyCovePage() {
  return (
    <>
      <Topbar3 />
      <Header3 />
      <Hero />
      <Collections />
      <Products3 parentClass="flat-spacing" />
      <Categories />
      <Testimonials />
      <Blogs />
      <Features parentClass="flat-spacing line-top-container" />
      <ShopGram />
      <Footer1 dark />
    </>
  );
}
