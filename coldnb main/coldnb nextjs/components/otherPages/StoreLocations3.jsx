"use client";
import React from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function StoreLocations3() {
  const { t } = useLanguage();
  return (
    <section className="flat-spacing">
      <div className="container">
        <div className="row">
          <div className="col-12">
            <div className="contact-us-map">
              <div className="wrap-map">
                <div
                  id="map-contact"
                  className="map-contact"
                  data-map-zoom={16}
                  data-map-scroll="true"
                >
                  <iframe
                    src="https://www.google.com/maps/embed?pb=!1m18!1m12!1m3!1d3023.712380803123!2d-74.00594108459235!3d40.712775279328264!2m3!1f0!2f0!3f0!3m2!1i1024!2i768!4f13.1!3m3!1m2!1s0x89c25a3168ef8b1f%3A0xe28a875ee38c8ed1!2sNew+York%2C+NY%2C+USA!5e0!3m2!1sen!2s!4v1617203294845!5m2!1sen!2s"
                    width={600}
                    height={450}
                    style={{ border: 0, width: "100%", height: "100%" }}
                    allowFullScreen=""
                    loading="lazy"
                  />
                </div>
              </div>
              <div className="right">
                <h4>{t("storeLocations.information")}</h4>
                <div className="mb_20">
                  <div className="text-title mb_8">{t("storeLocations.phoneLabel")}</div>
                  <p className="text-secondary">+1 666 234 8888</p>
                </div>
                <div className="mb_20">
                  <div className="text-title mb_8">{t("storeLocations.emailLabel")}</div>
                  <p className="text-secondary">contato@coldnb.com.br</p>
                </div>
                <div className="mb_20">
                  <div className="text-title mb_8">{t("storeLocations.addressLabel")}</div>
                  <p className="text-secondary">
                    2163 Phillips Gap Rd, West Jefferson, North Carolina, United
                    States
                  </p>
                </div>
                <div>
                  <div className="text-title mb_8">{t("storeLocations.openTime")}</div>
                  <p className="mb_4 open-time">
                    <span className="text-secondary">{t("contact.monSat")}</span> 7:30am -
                    8:00pm PST
                  </p>
                  <p className="open-time">
                    <span className="text-secondary">{t("contact.sunday")}</span> 9:00am -
                    5:00pm PST
                  </p>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
