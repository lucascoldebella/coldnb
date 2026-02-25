import React from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function Shipping() {
  const { t } = useLanguage();
  return (
    <>
      {" "}
      <div className="w-100">
        <div className="text-btn-uppercase mb_12">{t("productDescription.weveGotYourBack")}</div>
        <p className="mb_12">
          {t("productDescription.oneDeliveryFee")}
        </p>
        <p className="">
          {t("productDescription.freeReturns14")}
        </p>
      </div>
      <div className="w-100">
        <div className="text-btn-uppercase mb_12">
          {t("productDescription.importDuties")}
        </div>
        <p>
          {t("productDescription.importDutiesDesc")}
        </p>
      </div>
      <div className="w-100">
        <div className="text-btn-uppercase mb_12">{t("productDescription.estimatedDeliveryLabel")}</div>
        <p className="mb_6 font-2">Express: May 10 - May 17</p>
        <p className="font-2">Sending from USA</p>
      </div>
      <div className="w-100">
        <div className="text-btn-uppercase mb_12">{t("productDescription.needMoreInfo")}</div>
        <div>
          <a
            href="#"
            className="link text-secondary text-decoration-underline mb_6 font-2"
          >
            {t("productDescription.ordersDelivery")}
          </a>
        </div>
        <div>
          <a
            href="#"
            className="link text-secondary text-decoration-underline mb_6 font-2"
          >
            {t("productDescription.returnsRefunds")}
          </a>
        </div>
        <div>
          <a
            href="#"
            className="link text-secondary text-decoration-underline font-2"
          >
            {t("productDescription.dutiesTaxes")}
          </a>
        </div>
      </div>
    </>
  );
}
