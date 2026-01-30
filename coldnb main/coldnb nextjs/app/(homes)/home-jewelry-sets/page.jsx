import Categories from "@/components/common/Categories";
import Features from "@/components/common/Features";
import MarqueeSection2 from "@/components/common/MarqueeSection2";
import Products5 from "@/components/common/Products5";
import Tiktok from "@/components/common/Tiktok";
import Footer1 from "@/components/footers/Footer1";
import Header9 from "@/components/headers/Header9";
import Topbar4 from "@/components/headers/Topbar4";
import Hero from "@/components/homes/Jewelry/Hero";
import Lookbook from "@/components/homes/Jewelry/Lookbook";
import ShopGram from "@/components/homes/Jewelry/ShopGram";
import SingleProduct from "@/components/homes/Jewelry/SingleProduct";
import SkinBeforeAfter from "@/components/homes/Jewelry/SkinBeforeAfter";
import Testimonials from "@/components/common/Testimonials2";
import React from "react";

export const metadata = {
  title:
    "Home Skillcare || ColdnbMain - eCommerce",
  description: "ColdnbMain - eCommerce",
};

export default function HomeJewelryPage() {
  return (
    <>
      <Topbar4 />
      <Header9 />
      <Hero />

      <Tiktok parentClass="flat-spacing" />
      <Categories />
      <MarqueeSection2 />
      <Products5 />
      <Lookbook />
      <Testimonials />
      <SkinBeforeAfter />
      <SingleProduct />
      <Features />
      <ShopGram parentClass="flat-spacing pt-0" />
      <Footer1 />
    </>
  );
}
