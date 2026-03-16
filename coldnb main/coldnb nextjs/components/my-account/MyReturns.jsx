"use client";
import React, { useEffect, useState } from "react";
import { returnsApi } from "@/lib/userApi";
import { useLanguage } from "@/lib/i18n/LanguageContext";

const STATUS_COLORS = {
  requested: "#f59e0b",
  under_review: "#3b82f6",
  approved: "#10b981",
  rejected: "#ef4444",
  refunded: "#8b5cf6",
};

const STATUS_LABELS = {
  requested: "returns.statusRequested",
  under_review: "returns.statusUnderReview",
  approved: "returns.statusApproved",
  rejected: "returns.statusRejected",
  refunded: "returns.statusRefunded",
};

export default function MyReturns() {
  const { t } = useLanguage();
  const [returns, setReturns] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  useEffect(() => {
    loadReturns();
  }, []);

  const loadReturns = async () => {
    try {
      setLoading(true);
      const res = await returnsApi.list();
      const data = res.data?.data?.returns || res.data?.data || [];
      setReturns(Array.isArray(data) ? data : []);
    } catch (err) {
      setError(err.response?.data?.error || t("returns.loadError"));
    } finally {
      setLoading(false);
    }
  };

  const formatDate = (dateStr) => {
    if (!dateStr) return "—";
    return new Date(dateStr).toLocaleDateString("pt-BR", {
      day: "2-digit",
      month: "2-digit",
      year: "numeric",
    });
  };

  if (loading) {
    return (
      <div className="my-account-content">
        <div className="text-center py-5">
          <div className="spinner-border" role="status" />
        </div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="my-account-content">
        <div className="text-center py-5">
          <p className="text-danger">{error}</p>
          <button onClick={loadReturns} className="tf-btn btn-fill radius-3 mt-3">
            <span>{t("common.tryAgain")}</span>
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="my-account-content">
      <h5 className="fw-5 mb_20">{t("returns.title")}</h5>

      {returns.length === 0 ? (
        <div className="text-center py-5">
          <p className="text-secondary">{t("returns.noReturns")}</p>
        </div>
      ) : (
        <div className="table-responsive">
          <table className="table align-middle">
            <thead>
              <tr>
                <th>{t("returns.orderNumber")}</th>
                <th>{t("returns.reason")}</th>
                <th>{t("returns.status")}</th>
                <th>{t("returns.date")}</th>
                <th>{t("returns.refundAmount")}</th>
              </tr>
            </thead>
            <tbody>
              {returns.map((item) => (
                <tr key={item.id}>
                  <td>
                    <span className="fw-5">#{item.order_number || item.order_id?.slice(0, 8)}</span>
                  </td>
                  <td>{item.reason || "—"}</td>
                  <td>
                    <span
                      style={{
                        display: "inline-block",
                        padding: "4px 12px",
                        borderRadius: 20,
                        fontSize: 13,
                        fontWeight: 500,
                        color: "#fff",
                        backgroundColor: STATUS_COLORS[item.status] || "#6b7280",
                      }}
                    >
                      {t(STATUS_LABELS[item.status] || "returns.statusRequested")}
                    </span>
                  </td>
                  <td>{formatDate(item.created_at)}</td>
                  <td>
                    {item.refund_amount
                      ? `R$ ${parseFloat(item.refund_amount).toFixed(2)}`
                      : "—"}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}
