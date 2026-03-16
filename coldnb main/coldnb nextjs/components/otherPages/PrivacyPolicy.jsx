"use client";

import { useEffect, useState } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

const sectionIds = [
  "intro",
  "data-collected",
  "purpose",
  "legal-basis",
  "sharing",
  "cookies",
  "retention",
  "rights",
  "security",
  "contact",
  "changes",
];

export default function PrivacyPolicy() {
  const { t } = useLanguage();
  const [activeSection, setActiveSection] = useState(sectionIds[0]);

  const sections = [
    { id: 1, text: t("privacy.introTitle"), scroll: "intro" },
    { id: 2, text: t("privacy.dataCollectedTitle"), scroll: "data-collected" },
    { id: 3, text: t("privacy.purposeTitle"), scroll: "purpose" },
    { id: 4, text: t("privacy.legalBasisTitle"), scroll: "legal-basis" },
    { id: 5, text: t("privacy.sharingTitle"), scroll: "sharing" },
    { id: 6, text: t("privacy.cookiesTitle"), scroll: "cookies" },
    { id: 7, text: t("privacy.retentionTitle"), scroll: "retention" },
    { id: 8, text: t("privacy.rightsTitle"), scroll: "rights" },
    { id: 9, text: t("privacy.securityTitle"), scroll: "security" },
    { id: 10, text: t("privacy.contactTitle"), scroll: "contact" },
    { id: 11, text: t("privacy.changesTitle"), scroll: "changes" },
  ];

  useEffect(() => {
    const observer = new IntersectionObserver(
      (entries) => {
        entries.forEach((entry) => {
          if (entry.isIntersecting) {
            setActiveSection(entry.target.id);
          }
        });
      },
      { rootMargin: "-50% 0px" }
    );

    sectionIds.forEach((id) => {
      const element = document.getElementById(id);
      if (element) observer.observe(element);
    });

    return () => observer.disconnect();
  }, []);

  const handleClick = (id) => {
    document
      .getElementById(id)
      .scrollIntoView({ behavior: "smooth", block: "center" });
  };

  const renderList = (itemsKey) => {
    const items = t(itemsKey).split(";");
    return (
      <ul>
        {items.map((item, i) => (
          <li key={i}>{item.trim()}</li>
        ))}
      </ul>
    );
  };

  return (
    <section className="flat-spacing">
      <div className="container">
        <div className="terms-of-use-wrap">
          <div className="left sticky-top">
            {sections.map(({ id, text, scroll }) => (
              <h6
                key={id}
                onClick={() => handleClick(scroll)}
                className={`btn-scroll-target ${
                  activeSection === scroll ? "active" : ""
                }`}
              >
                {id}. {text}
              </h6>
            ))}
          </div>
          <div className="right">
            <h4 className="heading">{t("privacy.title")}</h4>
            <p className="text-secondary mb_20">{t("privacy.lastUpdated")}</p>

            <div className="terms-of-use-item item-scroll-target" id="intro">
              <h5 className="terms-of-use-title">1. {t("privacy.introTitle")}</h5>
              <div className="terms-of-use-content">
                <p>{t("privacy.introText")}</p>
              </div>
            </div>

            <div className="terms-of-use-item item-scroll-target" id="data-collected">
              <h5 className="terms-of-use-title">2. {t("privacy.dataCollectedTitle")}</h5>
              <div className="terms-of-use-content">
                <p>{t("privacy.dataCollectedText")}</p>
                {renderList("privacy.dataCollectedItems")}
              </div>
            </div>

            <div className="terms-of-use-item item-scroll-target" id="purpose">
              <h5 className="terms-of-use-title">3. {t("privacy.purposeTitle")}</h5>
              <div className="terms-of-use-content">
                <p>{t("privacy.purposeText")}</p>
                {renderList("privacy.purposeItems")}
              </div>
            </div>

            <div className="terms-of-use-item item-scroll-target" id="legal-basis">
              <h5 className="terms-of-use-title">4. {t("privacy.legalBasisTitle")}</h5>
              <div className="terms-of-use-content">
                <p>{t("privacy.legalBasisText")}</p>
              </div>
            </div>

            <div className="terms-of-use-item item-scroll-target" id="sharing">
              <h5 className="terms-of-use-title">5. {t("privacy.sharingTitle")}</h5>
              <div className="terms-of-use-content">
                <p>{t("privacy.sharingText")}</p>
              </div>
            </div>

            <div className="terms-of-use-item item-scroll-target" id="cookies">
              <h5 className="terms-of-use-title">6. {t("privacy.cookiesTitle")}</h5>
              <div className="terms-of-use-content">
                <p>{t("privacy.cookiesText")}</p>
              </div>
            </div>

            <div className="terms-of-use-item item-scroll-target" id="retention">
              <h5 className="terms-of-use-title">7. {t("privacy.retentionTitle")}</h5>
              <div className="terms-of-use-content">
                <p>{t("privacy.retentionText")}</p>
              </div>
            </div>

            <div className="terms-of-use-item item-scroll-target" id="rights">
              <h5 className="terms-of-use-title">8. {t("privacy.rightsTitle")}</h5>
              <div className="terms-of-use-content">
                <p>{t("privacy.rightsText")}</p>
                {renderList("privacy.rightsItems")}
              </div>
            </div>

            <div className="terms-of-use-item item-scroll-target" id="security">
              <h5 className="terms-of-use-title">9. {t("privacy.securityTitle")}</h5>
              <div className="terms-of-use-content">
                <p>{t("privacy.securityText")}</p>
              </div>
            </div>

            <div className="terms-of-use-item item-scroll-target" id="contact">
              <h5 className="terms-of-use-title">10. {t("privacy.contactTitle")}</h5>
              <div className="terms-of-use-content">
                <p>{t("privacy.contactText")}</p>
              </div>
            </div>

            <div className="terms-of-use-item item-scroll-target" id="changes">
              <h5 className="terms-of-use-title">11. {t("privacy.changesTitle")}</h5>
              <div className="terms-of-use-content">
                <p>{t("privacy.changesText")}</p>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
