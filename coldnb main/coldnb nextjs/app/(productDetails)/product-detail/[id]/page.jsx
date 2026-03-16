import Footer1 from "@/components/footers/Footer1";
import Header1 from "@/components/headers/Header1";
import Topbar6 from "@/components/headers/Topbar6";
import Breadcumb from "@/components/productDetails/Breadcumb";
import Descriptions1 from "@/components/productDetails/descriptions/Descriptions1";
import Details1 from "@/components/productDetails/details/Details1";
import RelatedProducts from "@/components/productDetails/RelatedProducts";
import { notFound } from "next/navigation";
import React from "react";

// Fetch product from backend API
async function getProduct(id) {
  try {
    const apiUrl = process.env.NEXT_PUBLIC_API_URL || "http://localhost:8080";
    const response = await fetch(`${apiUrl}/api/products/${id}`, {
      cache: "no-store", // Don't cache to always get fresh data
    });

    if (!response.ok) {
      return null;
    }

    const data = await response.json();

    if (data.success && data.data) {
      // Transform backend data to match frontend product structure
      const product = data.data;
      return {
        id: product.id,
        title: product.name,
        name: product.name,
        price: parseFloat(product.price) || 0,
        compareAtPrice: product.compare_at_price ? parseFloat(product.compare_at_price) : null,
        description: product.description || "",
        shortDescription: product.short_description || "",
        sku: product.sku,
        slug: product.slug,
        category: product.category_name || "Uncategorized",
        brand: product.brand || "",
        stock: product.stock_quantity || 0,
        isActive: product.is_active,
        isFeatured: product.is_featured,
        isNew: product.is_new,
        isSale: product.is_sale,
        imgSrc: product.images?.[0]?.url || "/images/products/placeholder.jpg",
        images: product.images?.map((img, index) => ({
          id: img.id || index + 1,
          url: img.url,
          src: img.url,
          is_primary: img.is_primary,
          alt: product.name || "",
        })) || [],
        colors: product.colors?.map(c => ({ name: c.name, code: c.hex_code })) || [],
        sizes: product.sizes?.map(s => s.name) || [],
      };
    }
    return null;
  } catch (error) {
    console.error("Error fetching product:", error);
    return null;
  }
}

export async function generateMetadata({ params }) {
  const { id } = await params;
  const product = await getProduct(id);
  const siteUrl = process.env.NEXT_PUBLIC_SITE_URL || "https://coldnb.com";

  if (!product) {
    return {
      title: "Product Not Found - Coldnb",
    };
  }

  const title = `${product.title} - Coldnb`;
  const description = (product.shortDescription || product.description || "Handcrafted jewelry by Coldnb").slice(0, 160);
  const imageUrl = product.imgSrc?.startsWith("http") ? product.imgSrc : `${siteUrl}${product.imgSrc}`;
  const productUrl = `${siteUrl}/product-detail/${id}`;

  return {
    title,
    description,
    alternates: { canonical: productUrl },
    openGraph: {
      title,
      description,
      url: productUrl,
      siteName: "Coldnb",
      images: [{ url: imageUrl, width: 800, height: 800, alt: product.title }],
      type: "website",
      locale: "pt_BR",
    },
    twitter: {
      card: "summary_large_image",
      title,
      description,
      images: [imageUrl],
    },
  };
}

export default async function ProductDetailPage({ params }) {
  const { id } = await params;
  const siteUrl = process.env.NEXT_PUBLIC_SITE_URL || "https://coldnb.com";

  // Fetch product from database
  const product = await getProduct(id);

  // If not found, show 404
  if (!product) {
    notFound();
  }

  const imageUrl = product.imgSrc?.startsWith("http") ? product.imgSrc : `${siteUrl}${product.imgSrc}`;

  const jsonLd = {
    "@context": "https://schema.org",
    "@type": "Product",
    name: product.title,
    description: product.shortDescription || product.description || "",
    image: imageUrl,
    sku: product.sku || "",
    brand: { "@type": "Brand", name: product.brand || "Coldnb" },
    category: product.category,
    offers: {
      "@type": "Offer",
      url: `${siteUrl}/product-detail/${id}`,
      priceCurrency: "BRL",
      price: product.price.toFixed(2),
      availability: product.stock > 0
        ? "https://schema.org/InStock"
        : "https://schema.org/OutOfStock",
      seller: { "@type": "Organization", name: "Coldnb" },
    },
  };

  return (
    <>
      <script
        type="application/ld+json"
        dangerouslySetInnerHTML={{ __html: JSON.stringify(jsonLd) }}
      />
      <Topbar6 bgColor="bg-main" />
      <Header1 />
      <Breadcumb product={product} />
      <Details1 product={product} />
      <Descriptions1 />
      <RelatedProducts productId={product.id} />
      <Footer1 hasPaddingBottom />
    </>
  );
}
