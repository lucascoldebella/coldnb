import MarqueeSection from "@/components/common/MarqueeSection";
import Products2 from "@/components/common/Products2";
import Testimonials from "@/components/common/Testimonials";
import Header1 from "@/components/headers/Header1";
import Topbar3 from "@/components/headers/Topbar3";
import BannerCountdown from "@/components/homes/Jewelry-elegantAbode/BannerCountdown";
import Categories from "@/components/homes/Jewelry-elegantAbode/Categories";
import Hero from "@/components/homes/Jewelry-elegantAbode/Hero";
import Lookbook from "@/components/homes/Jewelry-elegantAbode/Lookbook";
import Products from "@/components/homes/Jewelry-elegantAbode/Products";
import Blogs from "@/components/common/Blogs";
import React from "react";
import Features from "@/components/common/Features";
import Brands from "@/components/common/Brands";
import Footer1 from "@/components/footers/Footer1";
import MarqueeSection2 from "@/components/common/MarqueeSection2";

export const metadata = {
  title:
    "Home Jewelry Elegant Abode || ColdnbMain - eCommerce",
  description: "ColdnbMain - eCommerce",
};

export default function HomeJewelryElegantAbodePage() {
  return (
    <>
      <Topbar3 />
      <Header1 />
      <Hero />
      <Categories />
      <Products />
      <BannerCountdown />
      <MarqueeSection2 parentClass={"tf-marquee marquee-white bg-purple-2"} />
      <Lookbook />
      <Products2 title="Top Trending" parentClass="flat-spacing pt-0" />
      <Testimonials parentClass="flat-spacing pt-0" />
      <Blogs parentClass="" readMore />
      <Features parentClass="flat-spacing-5" />
      <Brands />
      <Footer1 dark />
    </>
  );
}
