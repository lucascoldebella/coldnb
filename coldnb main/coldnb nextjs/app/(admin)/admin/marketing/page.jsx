"use client";
import { useState, useEffect, useCallback } from "react";
import Link from "next/link";
import toast from "react-hot-toast";
import { adminAnalytics } from "@/lib/api/adminAnalytics";
import { formatCurrency } from "@/lib/adminUtils";
import StatCard from "@/components/admin/cards/StatCard";
import ChartCard from "@/components/admin/cards/ChartCard";
import BarChart from "@/components/admin/charts/BarChart";
import DataTable from "@/components/admin/tables/DataTable";

// Icons
const TrendingIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="22 7 13.5 15.5 8.5 10.5 2 17" />
    <polyline points="16 7 22 7 22 13" />
  </svg>
);

const TargetIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <circle cx="12" cy="12" r="10" />
    <circle cx="12" cy="12" r="6" />
    <circle cx="12" cy="12" r="2" />
  </svg>
);

const ShoppingBagIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M6 2L3 6v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2V6l-3-4z" />
    <line x1="3" y1="6" x2="21" y2="6" />
    <path d="M16 10a4 4 0 0 1-8 0" />
  </svg>
);

const RepeatIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="17 1 21 5 17 9" />
    <path d="M3 11V9a4 4 0 0 1 4-4h14" />
    <polyline points="7 23 3 19 7 15" />
    <path d="M21 13v2a4 4 0 0 1-4 4H3" />
  </svg>
);

const formatPercent = (value) => `${Number(value || 0).toFixed(1)}%`;

const PERIOD_MAP = { "7d": "7d", "30d": "30d", "90d": "90d" };

const DEFAULT_STATS = {
  trendingProducts: { value: 0, trend: "up", change: "0" },
  conversionRate: { value: 0, trend: "up", change: "0%" },
  totalSales: { value: 0, trend: "up", change: "0%" },
  repeatCustomers: { value: 0, trend: "up", change: "0%" },
};

export default function MarketingPage() {
  const [loading, setLoading] = useState(true);
  const [stats, setStats] = useState(DEFAULT_STATS);
  const [categoryPerformance, setCategoryPerformance] = useState(null);
  const [bestSellers, setBestSellers] = useState([]);
  const [hotProducts, setHotProducts] = useState([]);
  const [period, setPeriod] = useState("30d");

  const fetchData = useCallback(async () => {
    setLoading(true);
    try {
      const params = { period: PERIOD_MAP[period] || "30d" };

      const [categoryRes, topRes, conversionRes, salesRes, trafficRes] =
        await Promise.all([
          adminAnalytics.revenueByCategory(params).catch(() => null),
          adminAnalytics.topProducts(params).catch(() => null),
          adminAnalytics.conversionRates(params).catch(() => null),
          adminAnalytics.sales(params).catch(() => null),
          adminAnalytics.traffic(params).catch(() => null),
        ]);

      // Category performance for chart
      if (categoryRes?.data) {
        const categories =
          categoryRes.data?.data || categoryRes.data?.categories || categoryRes.data || [];
        if (Array.isArray(categories) && categories.length > 0) {
          setCategoryPerformance({
            labels: categories.map((c) => c.name || c.category),
            current: categories.map((c) => c.revenue || c.total || 0),
          });
        }
      }

      // Best sellers
      if (topRes?.data) {
        const products =
          topRes.data?.data || topRes.data?.products || topRes.data || [];
        setBestSellers(Array.isArray(products) ? products : []);
      }

      // Hot/trending products (from traffic endpoint)
      if (trafficRes?.data) {
        const trending =
          trafficRes.data?.data || trafficRes.data?.products || trafficRes.data || [];
        setHotProducts(Array.isArray(trending) ? trending.slice(0, 5) : []);
      }

      // Build stats from conversion + sales data
      const convData = conversionRes?.data?.data || conversionRes?.data || {};
      const salesData = salesRes?.data?.data || salesRes?.data || {};

      setStats({
        trendingProducts: {
          value: hotProducts.length || (trafficRes?.data?.data?.length || 0),
          trend: "up",
          change: "+0",
        },
        conversionRate: {
          value: convData.conversion_rate || convData.rate || 0,
          trend: (convData.change || 0) >= 0 ? "up" : "down",
          change: `${convData.change || 0}%`,
        },
        totalSales: {
          value: salesData.total_orders || salesData.total || 0,
          trend: (salesData.change || 0) >= 0 ? "up" : "down",
          change: `${salesData.change || 0}%`,
        },
        repeatCustomers: {
          value: convData.repeat_customer_rate || 0,
          trend: "up",
          change: `${convData.repeat_change || 0}%`,
        },
      });
    } catch {
      toast.error("Failed to load marketing data");
    } finally {
      setLoading(false);
    }
  }, [period]);

  useEffect(() => {
    fetchData();
  }, [fetchData]);

  const periodOptions = [
    { value: "7d", label: "7 Days" },
    { value: "30d", label: "30 Days" },
    { value: "90d", label: "90 Days" },
  ];

  const bestSellerColumns = [
    {
      header: "Product",
      accessor: "name",
      render: (row) => (
        <Link href={`/admin/products/${row.id}`} style={{ fontWeight: 500, color: "var(--admin-text-primary)", textDecoration: "none" }}>
          {row.name || row.product_name}
        </Link>
      ),
    },
    {
      header: "Revenue",
      accessor: "revenue",
      width: "120px",
      render: (row) => (
        <span style={{ fontWeight: 600 }}>{formatCurrency(row.revenue || row.total_revenue)}</span>
      ),
    },
    {
      header: "Units Sold",
      accessor: "units",
      width: "100px",
      render: (row) => (
        <span className="admin-badge badge-primary">{row.units || row.units_sold || row.quantity || 0}</span>
      ),
    },
    {
      header: "Views",
      accessor: "views",
      width: "100px",
      render: (row) => (row.views || 0).toLocaleString(),
    },
    {
      header: "Conversion",
      accessor: "conversion",
      width: "100px",
      render: (row) => (
        <span style={{ color: (row.conversion || row.conversion_rate || 0) > 2 ? "var(--admin-success)" : "var(--admin-text-secondary)" }}>
          {formatPercent(row.conversion || row.conversion_rate)}
        </span>
      ),
    },
  ];

  return (
    <div className="marketing-page">
      <div className="admin-page-header">
        <h1 className="page-title">Marketing Intelligence</h1>
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
          icon={<TrendingIcon />}
          iconColor="primary"
          value={loading ? "..." : stats.trendingProducts.value || 0}
          label="Trending Products"
          trend={stats.trendingProducts.trend}
          trendValue={stats.trendingProducts.change}
          period="this week"
          loading={loading}
        />
        <StatCard
          icon={<TargetIcon />}
          iconColor="success"
          value={loading ? "..." : `${stats.conversionRate.value || 0}%`}
          label="Conversion Rate"
          trend={stats.conversionRate.trend}
          trendValue={stats.conversionRate.change}
          period="vs last period"
          loading={loading}
        />
        <StatCard
          icon={<ShoppingBagIcon />}
          iconColor="info"
          value={loading ? "..." : (stats.totalSales.value || 0).toLocaleString()}
          label="Total Sales"
          trend={stats.totalSales.trend}
          trendValue={stats.totalSales.change}
          period="this period"
          loading={loading}
        />
        <StatCard
          icon={<RepeatIcon />}
          iconColor="warning"
          value={loading ? "..." : `${stats.repeatCustomers.value || 0}%`}
          label="Repeat Customers"
          trend={stats.repeatCustomers.trend}
          trendValue={stats.repeatCustomers.change}
          period="of total customers"
          loading={loading}
        />
      </div>

      {/* Charts Row */}
      <div className="dashboard-grid grid-charts" style={{ marginBottom: 24 }}>
        <ChartCard title="Category Performance" loading={loading}>
          {categoryPerformance && (
            <BarChart
              labels={categoryPerformance.labels}
              datasets={[
                {
                  label: "Revenue",
                  data: categoryPerformance.current,
                },
              ]}
              yAxisPrefix="R$ "
            />
          )}
        </ChartCard>

        <div className="admin-card">
          <div className="card-header">
            <h3 className="card-title">Hot Products (Trending)</h3>
          </div>
          <div className="card-body" style={{ padding: 0 }}>
            {loading ? (
              <div style={{ padding: 40, textAlign: "center" }}>
                <div className="loading-spinner" style={{ margin: "0 auto" }} />
              </div>
            ) : hotProducts.length === 0 ? (
              <div style={{ padding: 40, textAlign: "center", color: "var(--admin-text-secondary)" }}>
                No trending data available
              </div>
            ) : (
              <div className="list-body">
                {hotProducts.map((product, index) => (
                  <div key={product.id || index} className="list-item">
                    <div
                      style={{
                        width: 28,
                        height: 28,
                        borderRadius: "50%",
                        backgroundColor: index < 3 ? "var(--admin-primary)" : "var(--admin-bg)",
                        color: index < 3 ? "white" : "var(--admin-text-secondary)",
                        display: "flex",
                        alignItems: "center",
                        justifyContent: "center",
                        fontWeight: 600,
                        fontSize: 12,
                        flexShrink: 0,
                      }}
                    >
                      {index + 1}
                    </div>
                    <div className="item-content">
                      <div className="item-title">{product.name || product.product_name}</div>
                      <div className="item-subtitle">{product.category || product.category_name}</div>
                    </div>
                    <div style={{ textAlign: "right" }}>
                      <div style={{ fontSize: 13, color: "var(--admin-success)", fontWeight: 600 }}>
                        {product.trend || `+${product.growth || 0}%`}
                      </div>
                      <div style={{ fontSize: 12, color: "#6b7280" }}>
                        {(product.views || 0).toLocaleString()} views
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Best Sellers Table */}
      <div className="admin-card">
        <div className="card-header">
          <h3 className="card-title">Best Sellers by Revenue</h3>
        </div>
        <div className="card-body" style={{ padding: 0 }}>
          <DataTable
            columns={bestSellerColumns}
            data={bestSellers}
            loading={loading}
            searchable={false}
            pagination={false}
          />
        </div>
      </div>
    </div>
  );
}
