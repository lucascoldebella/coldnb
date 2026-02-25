"use client";
import React from "react";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function Breadcumb({ product }) {
  const { t } = useLanguage();
  const pathname = usePathname();
  return (
    <div className="tf-breadcrumb">
      <div className="container">
        <div className="tf-breadcrumb-wrap">
          <div className="tf-breadcrumb-list">
            <Link href={`/`} className="text text-caption-1">
              {t("product.homepage")}
            </Link>

            <i className="icon icon-arrRight" />
            <span className="text text-caption-1">{product.title}</span>
          </div>
          <div className="tf-breadcrumb-prev-next">
            <Link
              href={`/${pathname.split("/")[1]}/${
                product.id <= 1 ? 1 : product.id - 1
              }`}
              className="tf-breadcrumb-prev"
            >
              <i className="icon icon-arrLeft" />
            </Link>
            <Link href="/shop-default-grid" className="tf-breadcrumb-back">
              <i className="icon icon-squares-four" />
            </Link>
            <Link
              href={`/${pathname.split("/")[1]}/${product.id + 1}`}
              className="tf-breadcrumb-next"
            >
              <i className="icon icon-arrRight" />
            </Link>
          </div>
        </div>
      </div>
    </div>
  );
}
