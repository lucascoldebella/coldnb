"use client";
import React from "react";
import Image from "next/image";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function LanguageSelect({ light = false }) {
  const { lang, setLang } = useLanguage();

  const toggle = () => {
    setLang(lang === "pt-BR" ? "en" : "pt-BR");
  };

  return (
    <button
      onClick={toggle}
      className={`lang-toggle ${light ? "lang-toggle-light" : ""}`}
      title={lang === "pt-BR" ? "Switch to English" : "Mudar para Português"}
      style={{
        display: "inline-flex",
        alignItems: "center",
        gap: "6px",
        background: "none",
        border: "none",
        cursor: "pointer",
        padding: "4px 8px",
        borderRadius: "4px",
        fontSize: "13px",
        color: light ? "#fff" : "#181818",
        transition: "opacity 0.2s",
      }}
    >
      <Image
        src={lang === "pt-BR" ? "/images/country/br.svg" : "/images/country/us.svg"}
        width={24}
        height={16}
        alt={lang === "pt-BR" ? "Português" : "English"}
        style={{ borderRadius: "2px", objectFit: "cover" }}
      />
      <span style={{ fontWeight: 500 }}>
        {lang === "pt-BR" ? "PT" : "EN"}
      </span>
    </button>
  );
}
