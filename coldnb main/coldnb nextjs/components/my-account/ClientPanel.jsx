"use client";

import Link from "next/link";
import { useEffect, useState } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { useUserAuth } from "@/context/UserAuthContext";
import { addressesApi, ordersApi } from "@/lib/userApi";
import toast from "react-hot-toast";

function getProviderLabel(provider) {
  switch (provider) {
    case "google":
      return "Google";
    case "apple":
      return "Apple";
    case "email":
      return "E-mail";
    default:
      return provider || "E-mail";
  }
}

export default function ClientPanel() {
  const { t } = useLanguage();
  const { profile, user, isProfileComplete, deleteAccount } = useUserAuth();
  const [orderCount, setOrderCount] = useState(0);
  const [defaultAddress, setDefaultAddress] = useState(null);
  const [isDeletingAccount, setIsDeletingAccount] = useState(false);

  useEffect(() => {
    let active = true;

    const loadPanelData = async () => {
      const [ordersResponse, addressesResponse] = await Promise.all([
        ordersApi.list({ page: 1, limit: 1 }).catch(() => null),
        addressesApi.list().catch(() => null),
      ]);

      if (!active) return;

      const orderPayload = ordersResponse?.data?.data || ordersResponse?.data || [];
      const addressPayload = addressesResponse?.data?.data || addressesResponse?.data || {};

      const parsedOrderCount =
        ordersResponse?.data?.pagination?.total ||
        orderPayload?.count ||
        (Array.isArray(orderPayload?.orders) ? orderPayload.orders.length : 0) ||
        (Array.isArray(orderPayload) ? orderPayload.length : 0);

      const parsedAddresses = Array.isArray(addressPayload?.addresses)
        ? addressPayload.addresses
        : Array.isArray(addressPayload)
          ? addressPayload
          : [];

      setOrderCount(parsedOrderCount || 0);
      setDefaultAddress(parsedAddresses.find((address) => address.is_default) || null);
    };

    loadPanelData();

    return () => {
      active = false;
    };
  }, []);

  const provider =
    user?.app_metadata?.provider ||
    user?.identities?.[0]?.provider ||
    "email";

  const cards = [
    {
      title: t("clientPanel.accountStatus"),
      value: isProfileComplete
        ? t("clientPanel.accountReady")
        : t("clientPanel.accountPending"),
      description: isProfileComplete
        ? t("clientPanel.accountReadyHint")
        : t("clientPanel.accountPendingHint"),
    },
    {
      title: t("clientPanel.authenticatedWith"),
      value: getProviderLabel(provider),
      description: profile?.email || user?.email || t("clientPanel.notAvailable"),
    },
    {
      title: t("clientPanel.orders"),
      value: String(orderCount),
      description: t("clientPanel.ordersHint"),
    },
    {
      title: t("clientPanel.defaultAddress"),
      value: defaultAddress
        ? defaultAddress.label || t("account.myAddress")
        : t("clientPanel.notAvailable"),
      description: defaultAddress
        ? `${defaultAddress.city || ""}${defaultAddress.state ? `, ${defaultAddress.state}` : ""}`.trim()
        : t("clientPanel.defaultAddressHint"),
    },
  ];

  const handleDeleteAccount = async () => {
    setIsDeletingAccount(true);

    try {
      await deleteAccount();
      toast.success(t("auth.deleteAccountSuccess"));
    } catch (error) {
      toast.error(error?.response?.data?.error || error?.message || t("auth.deleteAccountError"));
    } finally {
      setIsDeletingAccount(false);
    }
  };

  return (
    <div className="mb_32">
      <div
        style={{
          padding: 24,
          borderRadius: 20,
          border: "1px solid rgba(24,24,24,0.08)",
          background: "linear-gradient(135deg, #faf7f2 0%, #ffffff 60%)",
          marginBottom: 24,
        }}
      >
        <div className="d-flex flex-column flex-lg-row justify-content-between gap-12 align-items-start">
          <div>
            <h5 className="mb_8">{t("clientPanel.title")}</h5>
            <p className="text-secondary mb_0">{t("clientPanel.subtitle")}</p>
          </div>
          <div className="d-flex gap-12 flex-wrap">
            <Link href="/my-account-orders" className="tf-btn btn-fill radius-4">
              <span className="text">{t("clientPanel.viewOrders")}</span>
            </Link>
            <Link href="/my-account-address" className="tf-btn btn-outline radius-4">
              <span className="text">{t("clientPanel.manageAddresses")}</span>
            </Link>
          </div>
        </div>
      </div>

      <div className="row">
        {cards.map((card) => (
          <div className="col-md-6 col-xl-3 mb_20" key={card.title}>
            <div
              style={{
                height: "100%",
                padding: 20,
                borderRadius: 18,
                border: "1px solid rgba(24,24,24,0.08)",
                background: "#fff",
              }}
            >
              <div className="text-caption-1 text-secondary mb_8">{card.title}</div>
              <div style={{ fontSize: 28, fontWeight: 600, lineHeight: 1.1 }}>{card.value}</div>
              <div className="text-secondary mt_8" style={{ fontSize: 14 }}>{card.description}</div>
            </div>
          </div>
        ))}
      </div>

      <div
        style={{
          padding: 24,
          borderRadius: 20,
          border: "1px solid rgba(24,24,24,0.08)",
          background: "#fff",
        }}
      >
        <div className="row align-items-center">
          <div className="col-lg-7 mb_20 mb-lg-0">
            <h6 className="mb_12">{t("clientPanel.nextFeatures")}</h6>
            <p className="text-secondary mb_16">{t("clientPanel.nextFeaturesHint")}</p>
            <div className="d-flex flex-column gap-8">
              <span>{t("clientPanel.tracking")}</span>
              <span>{t("clientPanel.returns")}</span>
              <span>{t("clientPanel.postSales")}</span>
            </div>
          </div>
          <div className="col-lg-5 text-lg-end">
            <div className="d-flex flex-column align-items-stretch align-items-lg-end gap-12">
              <Link href="/shop-default-grid" className="tf-btn btn-fill radius-4">
                <span className="text">{t("clientPanel.continueShopping")}</span>
              </Link>
              <button
                type="button"
                className="tf-btn radius-4"
                onClick={handleDeleteAccount}
                disabled={isDeletingAccount}
                style={{
                  background: "#c62828",
                  borderColor: "#c62828",
                  color: "#fff",
                }}
              >
                <span className="text">
                  {isDeletingAccount ? "..." : t("auth.deleteAccount")}
                </span>
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
