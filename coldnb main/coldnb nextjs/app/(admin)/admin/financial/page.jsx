"use client";
import { useState, useEffect, useCallback } from "react";
import StatCard from "@/components/admin/cards/StatCard";
import ChartCard from "@/components/admin/cards/ChartCard";
import LineChart from "@/components/admin/charts/LineChart";
import BarChart from "@/components/admin/charts/BarChart";
import PieChart from "@/components/admin/charts/PieChart";
import { adminAnalytics } from "@/lib/api/adminAnalytics";
import toast from "react-hot-toast";
import { formatCurrency, formatDate } from "@/lib/adminUtils";

// Icons
const StoreIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z" />
    <polyline points="9 22 9 12 15 12 15 22" />
  </svg>
);

const CalendarIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <rect x="3" y="4" width="18" height="18" rx="2" ry="2" />
    <line x1="16" y1="2" x2="16" y2="6" />
    <line x1="8" y1="2" x2="8" y2="6" />
    <line x1="3" y1="10" x2="21" y2="10" />
  </svg>
);

const TrendUpIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="23 6 13.5 15.5 8.5 10.5 1 18" />
    <polyline points="17 6 23 6 23 12" />
  </svg>
);

const ReceiptIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M4 2v20l2-1 2 1 2-1 2 1 2-1 2 1 2-1 2 1V2l-2 1-2-1-2 1-2-1-2 1-2-1-2 1-2-1z" />
    <path d="M16 8h-6a2 2 0 1 0 0 4h4a2 2 0 1 1 0 4H8" />
    <path d="M12 17V7" />
  </svg>
);

const PERIOD_MAP = {
  week: "7d",
  month: "30d",
  quarter: "90d",
  year: "1y",
};

const DEFAULT_DATA = {
  stats: {
    storeValue: { value: 0, trend: "up", change: "0%" },
    monthlyRevenue: { value: 0, trend: "up", change: "0%" },
    weeklyRevenue: { value: 0, trend: "up", change: "0%" },
    avgOrderValue: { value: 0, trend: "up", change: "0%" },
  },
  revenueOverTime: { labels: [], current: [], previous: [] },
  revenueByCategory: { labels: [], data: [] },
  paymentMethods: { labels: [], data: [] },
  topTransactions: [],
};

export default function FinancialPage() {
  const [loading, setLoading] = useState(true);
  const [data, setData] = useState(null);
  const [period, setPeriod] = useState("year");
  const [compareEnabled, setCompareEnabled] = useState(true);

  const fetchData = useCallback(async (selectedPeriod) => {
    setLoading(true);
    try {
      const params = { period: PERIOD_MAP[selectedPeriod] || "1y" };

      const [revenueRes, categoryRes, salesRes, paymentRes] = await Promise.all([
        adminAnalytics.revenue(params).catch(() => null),
        adminAnalytics.revenueByCategory(params).catch(() => null),
        adminAnalytics.sales(params).catch(() => null),
        adminAnalytics.paymentMethods(params).catch(() => null),
      ]);

      setData({
        stats: revenueRes?.stats ?? DEFAULT_DATA.stats,
        revenueOverTime: revenueRes?.revenueOverTime ?? DEFAULT_DATA.revenueOverTime,
        revenueByCategory: categoryRes ?? DEFAULT_DATA.revenueByCategory,
        paymentMethods: paymentRes ?? DEFAULT_DATA.paymentMethods,
        topTransactions: salesRes?.topTransactions ?? DEFAULT_DATA.topTransactions,
      });
    } catch {
      toast.error("Failed to load financial data");
      setData(DEFAULT_DATA);
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    fetchData(period);
  }, [period, fetchData]);

  const periodOptions = [
    { value: "week", label: "7D" },
    { value: "month", label: "30D" },
    { value: "quarter", label: "90D" },
    { value: "year", label: "1Y" },
  ];

  return (
    <div className="financial-page">
      <div className="admin-page-header">
        <h1 className="page-title">Financial Overview</h1>
        <div className="page-actions">
          <div className="btn-tabs">
            {periodOptions.map((opt) => (
              <button
                key={opt.value}
                className={`tab-btn ${period === opt.value ? "active" : ""}`}
                onClick={() => setPeriod(opt.value)}
              >
                {opt.label}
              </button>
            ))}
          </div>
        </div>
      </div>

      {/* Stats Row */}
      <div className="dashboard-grid grid-stats" style={{ marginBottom: 24 }}>
        <StatCard
          icon={<StoreIcon />}
          iconColor="primary"
          value={loading ? "..." : formatCurrency(data?.stats.storeValue.value || 0)}
          label="Total Store Value"
          trend={data?.stats.storeValue.trend}
          trendValue={data?.stats.storeValue.change}
          period="inventory value"
          loading={loading}
        />
        <StatCard
          icon={<CalendarIcon />}
          iconColor="success"
          value={loading ? "..." : formatCurrency(data?.stats.monthlyRevenue.value || 0)}
          label="Monthly Revenue"
          trend={data?.stats.monthlyRevenue.trend}
          trendValue={data?.stats.monthlyRevenue.change}
          period="vs last month"
          loading={loading}
        />
        <StatCard
          icon={<TrendUpIcon />}
          iconColor="info"
          value={loading ? "..." : formatCurrency(data?.stats.weeklyRevenue.value || 0)}
          label="Weekly Revenue"
          trend={data?.stats.weeklyRevenue.trend}
          trendValue={data?.stats.weeklyRevenue.change}
          period="vs last week"
          loading={loading}
        />
        <StatCard
          icon={<ReceiptIcon />}
          iconColor="warning"
          value={loading ? "..." : formatCurrency(data?.stats.avgOrderValue.value || 0)}
          label="Average Order Value"
          trend={data?.stats.avgOrderValue.trend}
          trendValue={data?.stats.avgOrderValue.change}
          period="vs last month"
          loading={loading}
        />
      </div>

      {/* Revenue Over Time Chart */}
      <div className="admin-card" style={{ marginBottom: 24 }}>
        <div className="card-header">
          <h3 className="card-title">Revenue Over Time</h3>
          <div className="card-actions">
            <label className="admin-checkbox" style={{ fontSize: 13 }}>
              <input
                type="checkbox"
                checked={compareEnabled}
                onChange={(e) => setCompareEnabled(e.target.checked)}
              />
              <span className="checkbox-label">Compare to previous period</span>
            </label>
          </div>
        </div>
        <div className="card-body" style={{ height: 350 }}>
          {loading ? (
            <div className="chart-loading">
              <div className="loading-spinner" />
            </div>
          ) : (
            <LineChart
              labels={data.revenueOverTime.labels}
              datasets={[
                {
                  label: "Current Period",
                  data: data.revenueOverTime.current,
                  borderColor: "#4f46e5",
                },
                ...(compareEnabled
                  ? [
                      {
                        label: "Previous Period",
                        data: data.revenueOverTime.previous,
                        borderColor: "#d1d5db",
                      },
                    ]
                  : []),
              ]}
              fill={true}
              yAxisPrefix="R$ "
            />
          )}
        </div>
      </div>

      {/* Charts Row */}
      <div className="dashboard-grid grid-charts" style={{ marginBottom: 24 }}>
        <ChartCard title="Revenue by Category" loading={loading}>
          {data && (
            <BarChart
              labels={data.revenueByCategory.labels}
              datasets={[
                {
                  label: "Revenue",
                  data: data.revenueByCategory.data,
                },
              ]}
              yAxisPrefix="R$ "
              horizontal={true}
            />
          )}
        </ChartCard>

        <ChartCard title="Payment Methods" loading={loading}>
          {data && (
            <PieChart
              labels={data.paymentMethods.labels}
              data={data.paymentMethods.data}
              donut={true}
              centerValue={`${data.paymentMethods.data[0]}%`}
              centerText="Credit Card"
            />
          )}
        </ChartCard>
      </div>

      {/* Top Transactions */}
      <div className="admin-card">
        <div className="card-header">
          <h3 className="card-title">Top Transactions</h3>
        </div>
        <div className="card-body" style={{ padding: 0 }}>
          <table className="admin-table">
            <thead>
              <tr>
                <th>Order</th>
                <th>Customer</th>
                <th>Date</th>
                <th style={{ textAlign: "right" }}>Amount</th>
              </tr>
            </thead>
            <tbody>
              {loading ? (
                <tr>
                  <td colSpan={4}>
                    <div className="table-loading" style={{ padding: 40 }}>
                      <div className="loading-spinner" />
                    </div>
                  </td>
                </tr>
              ) : (
                data?.topTransactions.map((tx) => (
                  <tr key={tx.id}>
                    <td style={{ fontWeight: 600 }}>{tx.order}</td>
                    <td>{tx.customer}</td>
                    <td style={{ color: "#6b7280" }}>
                      {formatDate(tx.date)}
                    </td>
                    <td style={{ textAlign: "right", fontWeight: 600, color: "var(--admin-success)" }}>
                      {formatCurrency(tx.amount)}
                    </td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}
