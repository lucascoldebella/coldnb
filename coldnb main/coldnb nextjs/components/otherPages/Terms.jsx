"use client";

import { useEffect, useState } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

const sectionIds = [
  "terms",
  "limitations",
  "revisions-and-errata",
  "site-terms",
  "risks",
];

// Dynamic sections object will be created inside component to access t()

export default function Terms() {
  const { t } = useLanguage();
  const [activeSection, setActiveSection] = useState(sectionIds[0]);

  const sections = [
    { id: 1, text: t("terms.terms"), scroll: "terms" },
    { id: 2, text: t("terms.limitations"), scroll: "limitations" },
    {
      id: 3,
      text: t("terms.revisions"),
      scroll: "revisions-and-errata",
    },
    {
      id: 4,
      text: t("terms.siteTerms"),
      scroll: "site-terms",
    },
    { id: 5, text: t("terms.risks"), scroll: "risks" },
  ];

  useEffect(() => {
    // Create an IntersectionObserver to track visibility of sections
    const observer = new IntersectionObserver(
      (entries) => {
        entries.forEach((entry) => {
          if (entry.isIntersecting) {
            // Update active section when the section is visible in the viewport
            setActiveSection(entry.target.id);
          }
        });
      },
      {
        rootMargin: "-50% 0px", // Trigger when section is 50% visible
      }
    );

    // Observe each section
    sectionIds.forEach((id) => {
      const element = document.getElementById(id);
      if (element) {
        observer.observe(element);
      }
    });

    return () => {
      // Cleanup the observer when the component unmounts
      observer.disconnect();
    };
  }, [sectionIds]);

  const handleClick = (id) => {
    document
      .getElementById(id)
      .scrollIntoView({ behavior: "smooth", block: "center" });
  };

  return (
    <section className="flat-spacing">
      <div className="container">
        <div className="terms-of-use-wrap">
          <div className="left sticky-top">
            {sections.map(({ id, text, scroll, isActive }) => (
              <h6
                key={id}
                onClick={() => handleClick(scroll)}
                className={`btn-scroll-target ${
                  activeSection == scroll ? "active" : ""
                }`}
              >
                {id}. {text}
              </h6>
            ))}
          </div>
          <div className="right">
            <h4 className="heading">{t("terms.termsOfUse")}</h4>
            <div className="terms-of-use-item item-scroll-target" id="terms">
              <h5 className="terms-of-use-title">1. {t("terms.terms")}</h5>
              <div className="terms-of-use-content">
                <p>
                  Lorem ipsum dolor sit amet, consectetur adipiscing elit.
                  Integer sed euismod justo, sit amet efficitur dui. Aliquam
                  sodales vestibulum velit, eget sollicitudin quam. Donec non
                  aliquam eros. Etiam sit amet lectus vel justo dignissim
                  condimentum.
                </p>
                <p>
                  In malesuada neque quis libero laoreet posuere. In consequat
                  vitae ligula quis rutrum. Morbi dolor orci, maximus a pulvinar
                  sed, bibendum ac lacus. Suspendisse in consectetur lorem.
                  Pellentesque habitant morbi tristique senectus et netus et
                  malesuada fames ac turpis egestas. Aliquam elementum, est sed
                  interdum cursus, felis ex pharetra nisi, ut elementum tortor
                  urna eu nulla. Donec rhoncus in purus quis blandit.
                </p>
                <p>
                  Etiam eleifend metus at nunc ultricies facilisis. Morbi
                  finibus tristique interdum. Nullam vel eleifend est, eu
                  posuere risus. Vestibulum ligula ex, ullamcorper sit amet
                  molestie
                </p>
              </div>
            </div>
            <div
              className="terms-of-use-item item-scroll-target"
              id="limitations"
            >
              <h5 className="terms-of-use-title">2. {t("terms.limitations")}</h5>
              <div className="terms-of-use-content">
                <p>
                  Lorem ipsum dolor sit amet, consectetur adipiscing elit.
                  Integer sed euismod justo, sit amet efficitur dui. Aliquam
                  sodales vestibulum velit, eget sollicitudin quam. Donec non
                  aliquam eros. Etiam sit amet lectus vel justo dignissim
                  condimentum.
                </p>
                <p>
                  In malesuada neque quis libero laoreet posuere. In consequat
                  vitae ligula quis rutrum. Morbi dolor orci, maximus a pulvinar
                  sed, bibendum ac lacus. Suspendisse in consectetur lorem.
                  Pellentesque habitant morbi tristique senectus et netus et
                  malesuada fames ac turpis egestas. Aliquam elementum, est sed
                  interdum cursus, felis ex pharetra nisi, ut elementum tortor
                  urna eu nulla. Donec rhoncus in purus quis blandit.
                </p>
                <p>
                  Etiam eleifend metus at nunc ultricies facilisis. Morbi
                  finibus tristique interdum. Nullam vel eleifend est, eu
                  posuere risus. Vestibulum ligula ex, ullamcorper sit amet
                  molestie
                </p>
              </div>
            </div>
            <div
              className="terms-of-use-item item-scroll-target"
              id="revisions-and-errata"
            >
              <h5 className="terms-of-use-title">3. {t("terms.revisions")}</h5>
              <div className="terms-of-use-content">
                <p>
                  Lorem ipsum dolor sit amet, consectetur adipiscing elit.
                  Integer sed euismod justo, sit amet efficitur dui. Aliquam
                  sodales vestibulum velit, eget sollicitudin quam. Donec non
                  aliquam eros. Etiam sit amet lectus vel justo dignissim
                  condimentum.
                </p>
                <p>
                  In malesuada neque quis libero laoreet posuere. In consequat
                  vitae ligula quis rutrum. Morbi dolor orci, maximus a pulvinar
                  sed, bibendum ac lacus. Suspendisse in consectetur lorem.
                  Pellentesque habitant morbi tristique senectus et netus et
                  malesuada fames ac turpis egestas. Aliquam elementum, est sed
                  interdum cursus, felis ex pharetra nisi, ut elementum tortor
                  urna eu nulla. Donec rhoncus in purus quis blandit.
                </p>
                <p>
                  Etiam eleifend metus at nunc ultricies facilisis. Morbi
                  finibus tristique interdum. Nullam vel eleifend est, eu
                  posuere risus. Vestibulum ligula ex, ullamcorper sit amet
                  molestie
                </p>
              </div>
            </div>
            <div
              className="terms-of-use-item item-scroll-target"
              id="site-terms"
            >
              <h5 className="terms-of-use-title">
                4. {t("terms.siteTerms")}
              </h5>
              <div className="terms-of-use-content">
                <p>
                  Lorem ipsum dolor sit amet, consectetur adipiscing elit.
                  Integer sed euismod justo, sit amet efficitur dui. Aliquam
                  sodales vestibulum velit, eget sollicitudin quam. Donec non
                  aliquam eros. Etiam sit amet lectus vel justo dignissim
                  condimentum.
                </p>
                <p>
                  In malesuada neque quis libero laoreet posuere. In consequat
                  vitae ligula quis rutrum. Morbi dolor orci, maximus a pulvinar
                  sed, bibendum ac lacus. Suspendisse in consectetur lorem.
                  Pellentesque habitant morbi tristique senectus et netus et
                  malesuada fames ac turpis egestas. Aliquam elementum, est sed
                  interdum cursus, felis ex pharetra nisi, ut elementum tortor
                  urna eu nulla. Donec rhoncus in purus quis blandit.
                </p>
                <p>
                  Etiam eleifend metus at nunc ultricies facilisis. Morbi
                  finibus tristique interdum. Nullam vel eleifend est, eu
                  posuere risus. Vestibulum ligula ex, ullamcorper sit amet
                  molestie
                </p>
              </div>
            </div>
            <div className="terms-of-use-item item-scroll-target" id="risks">
              <h5 className="terms-of-use-title">5. {t("terms.risks")}</h5>
              <div className="terms-of-use-content">
                <p>
                  Lorem ipsum dolor sit amet, consectetur adipiscing elit.
                  Integer sed euismod justo, sit amet efficitur dui. Aliquam
                  sodales vestibulum velit, eget sollicitudin quam. Donec non
                  aliquam eros. Etiam sit amet lectus vel justo dignissim
                  condimentum.
                </p>
                <p>
                  In malesuada neque quis libero laoreet posuere. In consequat
                  vitae ligula quis rutrum. Morbi dolor orci, maximus a pulvinar
                  sed, bibendum ac lacus. Suspendisse in consectetur lorem.
                  Pellentesque habitant morbi tristique senectus et netus et
                  malesuada fames ac turpis egestas. Aliquam elementum, est sed
                  interdum cursus, felis ex pharetra nisi, ut elementum tortor
                  urna eu nulla. Donec rhoncus in purus quis blandit.
                </p>
                <p>
                  Etiam eleifend metus at nunc ultricies facilisis. Morbi
                  finibus tristique interdum. Nullam vel eleifend est, eu
                  posuere risus. Vestibulum ligula ex, ullamcorper sit amet
                  molestie
                </p>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
