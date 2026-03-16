"use client";

import { useState, useEffect } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { loyaltyApi } from "@/lib/api/loyaltyApi";
import toast from "react-hot-toast";

export default function LoyaltyDashboard() {
  const { t } = useLanguage();
  const [balance, setBalance] = useState(0);
  const [history, setHistory] = useState([]);
  const [rewards, setRewards] = useState([]);
  const [loading, setLoading] = useState(true);
  const [redeeming, setRedeeming] = useState(null);

  useEffect(() => {
    Promise.all([
      loyaltyApi.getBalance().then((res) => res.data?.data || res.data),
      loyaltyApi.getHistory().then((res) => res.data?.data || res.data || []),
      loyaltyApi.getRewards().then((res) => res.data?.data || res.data || []),
    ])
      .then(([balanceData, historyData, rewardsData]) => {
        setBalance(balanceData?.balance || 0);
        setHistory(Array.isArray(historyData) ? historyData : []);
        setRewards(Array.isArray(rewardsData) ? rewardsData : []);
      })
      .catch(() => toast.error("Failed to load loyalty data"))
      .finally(() => setLoading(false));
  }, []);

  const handleRedeem = async (reward) => {
    const msg = t("loyalty.redeemConfirm").replace("{points}", reward.points_cost);
    if (!confirm(msg)) return;

    setRedeeming(reward.id);
    try {
      const res = await loyaltyApi.redeem(reward.id);
      const data = res.data?.data || res.data;
      const successMsg = t("loyalty.redeemSuccess").replace("{code}", data.discount_code);
      toast.success(successMsg, { duration: 8000 });
      setBalance(data.new_balance);
      // Refresh history
      const histRes = await loyaltyApi.getHistory();
      setHistory(histRes.data?.data || histRes.data || []);
    } catch (err) {
      const msg = err.response?.data?.error || t("loyalty.redeemError");
      toast.error(msg);
    } finally {
      setRedeeming(null);
    }
  };

  const formatDate = (dateStr) => {
    if (!dateStr) return "";
    return new Date(dateStr).toLocaleDateString("pt-BR", {
      day: "numeric",
      month: "short",
      year: "numeric",
    });
  };

  if (loading) {
    return (
      <div className="my-account-content">
        <div className="d-flex justify-content-center" style={{ padding: 40 }}>
          <div className="tf-loading" />
        </div>
      </div>
    );
  }

  return (
    <div className="my-account-content">
      {/* Balance Card */}
      <div
        className="text-center mb-4 p-4"
        style={{
          background: "linear-gradient(135deg, #111827 0%, #374151 100%)",
          borderRadius: 12,
          color: "#fff",
        }}
      >
        <div style={{ fontSize: 14, opacity: 0.8, marginBottom: 4 }}>
          {t("loyalty.balance")}
        </div>
        <div style={{ fontSize: 48, fontWeight: 700 }}>{balance}</div>
        <div style={{ fontSize: 14, opacity: 0.7 }}>{t("loyalty.points")}</div>
        <div style={{ fontSize: 12, opacity: 0.5, marginTop: 8 }}>
          {t("loyalty.earnInfo")}
        </div>
      </div>

      {/* Available Rewards */}
      <h6 className="mb-3">{t("loyalty.rewards")}</h6>
      {rewards.length === 0 ? (
        <p className="text-secondary mb-4">{t("loyalty.noRewards")}</p>
      ) : (
        <div className="row g-3 mb-4">
          {rewards.map((reward) => (
            <div key={reward.id} className="col-md-6">
              <div
                className="p-3"
                style={{
                  border: "1px solid #e5e7eb",
                  borderRadius: 8,
                  height: "100%",
                  display: "flex",
                  flexDirection: "column",
                  justifyContent: "space-between",
                }}
              >
                <div>
                  <div className="fw-bold">{reward.name}</div>
                  {reward.description && (
                    <div className="text-secondary" style={{ fontSize: 13 }}>
                      {reward.description}
                    </div>
                  )}
                  <div className="mt-2" style={{ fontSize: 13, color: "#6b7280" }}>
                    {reward.reward_type === "percentage"
                      ? `${reward.reward_value}% off`
                      : `R$ ${parseFloat(reward.reward_value).toFixed(2)} off`}
                  </div>
                </div>
                <div className="mt-3 d-flex align-items-center justify-content-between">
                  <span
                    className="fw-bold"
                    style={{ color: balance >= reward.points_cost ? "#111827" : "#9ca3af" }}
                  >
                    {reward.points_cost} {t("loyalty.points")}
                  </span>
                  <button
                    className="tf-btn btn-fill radius-3"
                    style={{ padding: "6px 16px", fontSize: 13 }}
                    disabled={balance < reward.points_cost || redeeming === reward.id}
                    onClick={() => handleRedeem(reward)}
                  >
                    <span className="text">
                      {redeeming === reward.id
                        ? "..."
                        : balance < reward.points_cost
                          ? t("loyalty.insufficient")
                          : t("loyalty.redeem")}
                    </span>
                  </button>
                </div>
              </div>
            </div>
          ))}
        </div>
      )}

      {/* Points History */}
      <h6 className="mb-3">{t("loyalty.history")}</h6>
      {history.length === 0 ? (
        <p className="text-secondary">{t("loyalty.noHistory")}</p>
      ) : (
        <div className="table-responsive">
          <table className="table">
            <thead>
              <tr>
                <th>Date</th>
                <th>Description</th>
                <th className="text-end">Points</th>
              </tr>
            </thead>
            <tbody>
              {history.map((entry) => (
                <tr key={entry.id}>
                  <td style={{ fontSize: 13, color: "#6b7280" }}>
                    {formatDate(entry.created_at)}
                  </td>
                  <td>
                    {entry.reason}
                    {entry.reference_id && (
                      <span className="text-secondary" style={{ fontSize: 12 }}>
                        {" "}
                        ({entry.reference_id})
                      </span>
                    )}
                  </td>
                  <td
                    className="text-end fw-bold"
                    style={{ color: entry.points >= 0 ? "#10b981" : "#ef4444" }}
                  >
                    {entry.points >= 0 ? "+" : ""}
                    {entry.points}
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
