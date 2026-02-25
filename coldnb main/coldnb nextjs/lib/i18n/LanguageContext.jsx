"use client";
import React, { createContext, useContext, useState, useEffect } from "react";
import ptTranslations from "./translations/pt.json";
import enTranslations from "./translations/en.json";

const translations = { "pt-BR": ptTranslations, "en": enTranslations };
const LanguageContext = createContext({ lang: "pt-BR", setLang: () => {}, t: (k) => k });

export function LanguageProvider({ children }) {
  const [lang, setLangState] = useState("pt-BR");

  useEffect(() => {
    const saved = localStorage.getItem("coldnb-lang");
    if (saved === "pt-BR" || saved === "en") setLangState(saved);
    document.documentElement.lang = saved || "pt-BR";
  }, []);

  const setLang = (newLang) => {
    setLangState(newLang);
    localStorage.setItem("coldnb-lang", newLang);
    document.documentElement.lang = newLang;
  };

  const t = (keyPath) => {
    const keys = keyPath.split(".");
    let value = translations[lang];
    for (const key of keys) {
      if (value === undefined) return keyPath;
      value = value[key];
    }
    return value ?? keyPath;
  };

  return (
    <LanguageContext.Provider value={{ lang, setLang, t }}>
      {children}
    </LanguageContext.Provider>
  );
}

export const useLanguage = () => useContext(LanguageContext);
