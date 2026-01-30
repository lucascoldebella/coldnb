import Features from "@/components/common/Features";
import Footer1 from "@/components/footers/Footer1";
import Header2 from "@/components/headers/Header2";
import BannerTab from "@/components/common/BannerTab";
import Collections from "@/components/homes/Jewelry-main/Collections";
import Collections2 from "@/components/homes/Jewelry-main/Collections2";
import Hero from "@/components/homes/Jewelry-main/Hero";
import Products from "@/components/common/Products";
import ShopGram from "@/components/common/ShopGram2";
import Testimonials from "@/components/homes/Jewelry-main/Testimonials";
import React from "react";
import Features2 from "@/components/common/Features2";

export const metadata = {
  title:
    "Home Jewelry Main || ColdnbMain - eCommerce",
  description: "ColdnbMain - eCommerce",
};

export default function HomeJewelryMainPage() {
  return (
    <>
      <Header2 />
      <Hero />
      <Collections />
      <BannerTab />
      <Products />
      <Collections2 />
      <Features2 />
      <Testimonials />
      <ShopGram />
      <Footer1 />
    </>
  );
}
