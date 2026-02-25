"use client";
import React from "react";
import Link from "next/link";
import LanguageSelect from "../common/LanguageSelect";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function Topbar() {
  const { t } = useLanguage();
  return (
    <div className="tf-topbar bg-main">
      <div className="container">
        <div className="tf-topbar_wrap d-flex align-items-center justify-content-center justify-content-xl-between">
          <ul className="topbar-left">
            <li>
              <a className="text-caption-1 text-white" href="tel:315-666-6688">
                315-666-6688
              </a>
            </li>
            <li>
              <a className="text-caption-1 text-white" href="#">
                contato@coldnb.com.br
              </a>
            </li>
            <li>
              <Link
                className="text-caption-1 text-white text-decoration-underline"
                href={`/store-list`}
              >
                {t("topbar.ourStore")}
              </Link>
            </li>
          </ul>
          <div className="topbar-right d-none d-xl-block">
            <div className="tf-cur justify-content-end">
              <div className="tf-languages position-relative">
                <LanguageSelect light />
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
