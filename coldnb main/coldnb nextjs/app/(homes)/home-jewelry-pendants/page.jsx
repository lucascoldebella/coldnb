import Blogs from "@/components/common/Blogs";
import Brands from "@/components/common/Brands";
import Features from "@/components/common/Features";
import Products4 from "@/components/common/Products4";
import Footer1 from "@/components/footers/Footer1";
import Header6 from "@/components/headers/Header6";
import Topbar2 from "@/components/headers/Topbar2";
import BannerTab from "@/components/homes/Jewelry-tiktok/BannerTab";
import Collections from "@/components/homes/Jewelry-tiktok/Collections";
import Hero from "@/components/homes/Jewelry-tiktok/Hero";
import MarqueeSection from "@/components/common/MarqueeSection2";
import Testimonials from "@/components/homes/Jewelry-tiktok/Testimonials3";
import Tiktok from "@/components/homes/Jewelry-tiktok/Tiktok";
import React from "react";
import Features2 from "@/components/common/Features2";

export const metadata = {
  title:
    "Home Jewelry Tiktok || ColdnbMain - eCommerce",
  description: "ColdnbMain - eCommerce",
};

export default function HomeJewelryTiktokPage() {
  return (
    <>
      <Topbar2 />
      <Header6 />
      <Hero />
      <Collections />
      <Products4 parentClass="flat-spacing pt-0" />
      <MarqueeSection />
      <BannerTab />
      <Tiktok />
      <Testimonials />
      <Blogs parentClass="flat-spacing" />
      <Features2 parentClass="flat-spacing line-top-container" />
      <Brands />
      <Footer1 dark />
    </>
  );
}
