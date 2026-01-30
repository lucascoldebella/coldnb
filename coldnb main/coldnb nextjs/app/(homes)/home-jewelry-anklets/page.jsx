import Features from "@/components/common/Features";
import ShopGram from "@/components/common/ShopGram2";
import Footer1 from "@/components/footers/Footer1";
import Header1 from "@/components/headers/Header1";
import Topbar2 from "@/components/headers/Topbar2";
import Banner from "@/components/homes/Jewelry-trendset/Banner";
import Blogs from "@/components/homes/Jewelry-trendset/Blogs";
import Collectons from "@/components/homes/Jewelry-trendset/Collectons";
import Hero from "@/components/homes/Jewelry-trendset/Hero";
import Products from "@/components/homes/Jewelry-trendset/Products";
import Products2 from "@/components/homes/Jewelry-trendset/Products2";
import Testimonials from "@/components/homes/Jewelry-trendset/Testimonials";
import React from "react";

export const metadata = {
  title:
    "Home Jewelry Trendset || ColdnbMain - eCommerce",
  description: "ColdnbMain - eCommerce",
};

export default function HomeJewelryTrendsetPage() {
  return (
    <>
      <Header1 />
      <Topbar2 />
      <Hero />
      <Products />
      <Banner />
      <Products2 parentClass="flat-spacing" />
      <Collectons />
      <Testimonials />
      <Blogs />
      <Features />
      <ShopGram />
      <Footer1 />
    </>
  );
}
