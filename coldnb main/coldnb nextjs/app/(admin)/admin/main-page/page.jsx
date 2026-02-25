"use client";
import { useState, useEffect, useCallback } from "react";
import { useAdmin } from "@/context/AdminContext";
import { adminHeroSlides, adminBanners, adminSections, adminCampaigns } from "@/lib/api/adminHomepage";
import HomepageSectionCard from "@/components/admin/homepage/HomepageSectionCard";
import HeroSlidesManager from "@/components/admin/homepage/HeroSlidesManager";
import CategoriesManager from "@/components/admin/homepage/CategoriesManager";
import SectionsManager from "@/components/admin/homepage/SectionsManager";
import CollectionBannersManager from "@/components/admin/homepage/CollectionBannersManager";
import CountdownBannerManager from "@/components/admin/homepage/CountdownBannerManager";
import CampaignsManager from "@/components/admin/homepage/CampaignsManager";
import NavigationManager from "@/components/admin/navigation/NavigationManager";

const extractArray = (res, key) => {
  const wrapper = res?.data?.data ?? res?.data;
  if (wrapper && typeof wrapper === "object" && !Array.isArray(wrapper)) {
    const arr = wrapper[key];
    if (Array.isArray(arr)) return arr;
  }
  return Array.isArray(wrapper) ? wrapper : [];
};

export default function MainPageAdmin() {
  const { hasPermission } = useAdmin();
  const canManage = hasPermission("manage_homepage");

  const [loading, setLoading] = useState(true);
  const [data, setData] = useState({
    slides: [],
    banners: [],
    sections: [],
    campaigns: [],
  });

  const fetchData = useCallback(async () => {
    setLoading(true);
    try {
      const [slidesRes, bannersRes, sectionsRes, campaignsRes] = await Promise.allSettled([
        adminHeroSlides.list(),
        adminBanners.list(),
        adminSections.list(),
        adminCampaigns.list(),
      ]);
      setData({
        slides: slidesRes.status === "fulfilled" ? extractArray(slidesRes.value, "slides") : [],
        banners: bannersRes.status === "fulfilled" ? extractArray(bannersRes.value, "banners") : [],
        sections: sectionsRes.status === "fulfilled" ? extractArray(sectionsRes.value, "sections") : [],
        campaigns: campaignsRes.status === "fulfilled" ? extractArray(campaignsRes.value, "campaigns") : [],
      });
    } catch {
      // Fallback — keep empty arrays
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    fetchData();
  }, [fetchData]);

  if (loading) {
    return (
      <div className="admin-page">
        <div className="admin-page-header">
          <div>
            <h1 className="admin-page-title">Main Page</h1>
            <p className="admin-page-subtitle">Manage your storefront homepage content</p>
          </div>
        </div>
        <div style={{ textAlign: "center", padding: 60, color: "#9ca3af" }}>
          Loading homepage data...
        </div>
      </div>
    );
  }

  return (
    <div className="admin-page">
      <div className="admin-page-header">
        <div>
          <h1 className="admin-page-title">Main Page</h1>
          <p className="admin-page-subtitle">
            Sections are ordered as they appear on the homepage — top to bottom
          </p>
        </div>
      </div>

      {/* Navigation Menus */}
      <HomepageSectionCard
        title="Navigation Menus"
        subtitle="Manage top navigation dropdowns (Inicio, Loja, Produtos, Blog, Paginas)"
        badge="Dynamic"
        badgeType="dynamic"
        collapsible
        defaultOpen={false}
      >
        <NavigationManager hasPermission={canManage} />
      </HomepageSectionCard>

      {/* 1. Hero Slider */}
      <HomepageSectionCard
        number={1}
        title="Hero Slider"
        subtitle={`${data.slides.filter((s) => s.is_active).length} active / ${data.slides.length} total`}
        badge="Dynamic"
        badgeType="dynamic"
      >
        <HeroSlidesManager slides={data.slides} onRefresh={fetchData} hasPermission={canManage} />
      </HomepageSectionCard>

      {/* 2. Categories Carousel */}
      <HomepageSectionCard
        number={2}
        title="Categories Carousel"
        subtitle="Circular category images with product counts"
        badge="Dynamic"
        badgeType="dynamic"
      >
        <CategoriesManager />
      </HomepageSectionCard>

      {/* 3. Product Sections */}
      <HomepageSectionCard
        number={3}
        title="Product Sections"
        subtitle={`${data.sections.filter((s) => s.is_active).length} active section(s)`}
        badge="Dynamic"
        badgeType="dynamic"
      >
        <SectionsManager sections={data.sections} onRefresh={fetchData} hasPermission={canManage} />
      </HomepageSectionCard>

      {/* 4. Collection Banners */}
      <HomepageSectionCard
        number={4}
        title="Collection Banners"
        subtitle={`${data.banners.filter((b) => b.banner_type === "collection" && b.is_active).length} active banner(s)`}
        badge="Dynamic"
        badgeType="dynamic"
      >
        <CollectionBannersManager banners={data.banners} onRefresh={fetchData} hasPermission={canManage} />
      </HomepageSectionCard>

      {/* 5. Countdown Banner */}
      <HomepageSectionCard
        number={5}
        title="Countdown Banner"
        subtitle={data.banners.find((b) => b.banner_type === "countdown" && b.is_active) ? "Active countdown running" : "No active countdown"}
        badge="Dynamic"
        badgeType="dynamic"
      >
        <CountdownBannerManager banners={data.banners} onRefresh={fetchData} hasPermission={canManage} />
      </HomepageSectionCard>

      {/* 6-9. Static sections */}
      <HomepageSectionCard number={6} title="Testimonials" badge="Static" badgeType="static">
        <div style={{ fontSize: 13, color: "var(--admin-text-secondary)", padding: "8px 0" }}>
          Customer testimonials carousel. Currently using hardcoded content — will be manageable in a future update.
        </div>
      </HomepageSectionCard>

      <HomepageSectionCard number={7} title="Blog Posts" badge="Static" badgeType="static">
        <div style={{ fontSize: 13, color: "var(--admin-text-secondary)", padding: "8px 0" }}>
          Latest blog posts section. Currently using hardcoded content — will be manageable in a future update.
        </div>
      </HomepageSectionCard>

      <HomepageSectionCard number={8} title="ShopGram" badge="Static" badgeType="static">
        <div style={{ fontSize: 13, color: "var(--admin-text-secondary)", padding: "8px 0" }}>
          Instagram-style image grid. Currently using hardcoded content — will be manageable in a future update.
        </div>
      </HomepageSectionCard>

      <HomepageSectionCard number={9} title="Features" badge="Static" badgeType="static">
        <div style={{ fontSize: 13, color: "var(--admin-text-secondary)", padding: "8px 0" }}>
          Trust badges and feature icons. Currently using hardcoded content — will be manageable in a future update.
        </div>
      </HomepageSectionCard>

      {/* Campaigns — collapsible at bottom */}
      <HomepageSectionCard
        title="Campaigns"
        subtitle={`${data.campaigns.filter((c) => c.is_active).length} active campaign(s)`}
        badge="Advanced"
        badgeType="static"
        collapsible
        defaultOpen={false}
      >
        <CampaignsManager
          campaigns={data.campaigns}
          heroSlides={data.slides}
          banners={data.banners}
          onRefresh={fetchData}
          hasPermission={canManage}
        />
      </HomepageSectionCard>
    </div>
  );
}
