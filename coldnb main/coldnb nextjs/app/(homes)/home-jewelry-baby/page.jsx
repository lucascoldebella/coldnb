import Footer1 from "@/components/footers/Footer1";
import Header13 from "@/components/headers/Header13";
import Topbar7 from "@/components/headers/Topbar7";
import Banner from "@/components/homes/home-Baby Jewelry/Banner";
import Blogs from "@/components/homes/home-Baby Jewelry/Blogs";
import Categories from "@/components/homes/home-Baby Jewelry/Categories";
import CollectionBanner from "@/components/homes/home-Baby Jewelry/CollectionBanner";
import Collections from "@/components/homes/home-Baby Jewelry/Collections";
import Features from "@/components/homes/home-Baby Jewelry/Features";
import Hero from "@/components/homes/home-Baby Jewelry/Hero";
import Products from "@/components/homes/home-Baby Jewelry/Products";
import Products2 from "@/components/homes/home-Baby Jewelry/Products2";
import React from "react";

export const metadata = {
  title: "Home Baby Jewelry || ColdnbMain - eCommerce",
  description: "ColdnbMain - eCommerce",
};
export default function page() {
  return (
    <>
      <Topbar7 />
      <Header13 />
      <Hero />
      <Collections />
      <Categories />
      <Products />
      <Features />
      <Banner />
      <Products2 />
      <Blogs />
      <CollectionBanner />
      <Footer1 dark />
    </>
  );
}
