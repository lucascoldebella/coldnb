import Footer1 from "@/components/footers/Footer1";
import Header1 from "@/components/headers/Header1";
import Topbar from "@/components/headers/Topbar";
import BannerCollection from "@/components/homes/home-1/BannerCollection";
import BannerCountdown from "@/components/homes/home-1/BannerCountdown";
import Blogs from "@/components/common/Blogs";
import Collections from "@/components/homes/home-1/Collections";
import Features from "@/components/common/Features";
import Hero from "@/components/homes/home-1/Hero";
import Products from "@/components/common/Products3";
import ShopGram from "@/components/common/ShopGram";
import Testimonials from "@/components/common/Testimonials";

export const metadata = {
  title: "Home || ColdnbMain - eCommerce",
  description: "ColdnbMain - eCommerce",
};

async function getHomepageData() {
  try {
    const apiUrl = process.env.NEXT_PUBLIC_API_URL || "http://localhost:8080";
    const res = await fetch(`${apiUrl}/api/homepage`, {
      next: { revalidate: 60 },
    });
    if (!res.ok) return null;
    const json = await res.json();
    return json.data || null;
  } catch {
    return null;
  }
}

export default async function HomePage() {
  const data = await getHomepageData();

  return (
    <>
      <Topbar />
      <Header1 />
      <Hero data={data?.hero_slides} />
      <Collections data={data?.categories} />
      <Products data={data?.sections?.products_tabbed} />
      <BannerCollection data={data?.banners_collection} />
      <BannerCountdown data={data?.banner_countdown} />
      <Testimonials />
      <Blogs />
      <ShopGram />
      <Features />
      <Footer1 />
    </>
  );
}
