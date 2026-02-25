"use client";
import { useState, useEffect } from "react";
import Link from "next/link";
import toast from "react-hot-toast";
import { useAdmin } from "@/context/AdminContext";
import { adminAnalytics } from "@/lib/api/adminAnalytics";
import { formatCurrency, formatDate } from "@/lib/adminUtils";
import StatCard from "@/components/admin/cards/StatCard";
import ChartCard from "@/components/admin/cards/ChartCard";
import ListCard from "@/components/admin/cards/ListCard";
import LineChart from "@/components/admin/charts/LineChart";
import PieChart from "@/components/admin/charts/PieChart";

// Icons
const RevenueIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="12" y1="1" x2="12" y2="23" />
    <path d="M17 5H9.5a3.5 3.5 0 0 0 0 7h5a3.5 3.5 0 0 1 0 7H6" />
  </svg>
);

const OrdersIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M6 2L3 6v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2V6l-3-4z" />
    <line x1="3" y1="6" x2="21" y2="6" />
    <path d="M16 10a4 4 0 0 1-8 0" />
  </svg>
);

const CustomersIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2" />
    <circle cx="9" cy="7" r="4" />
    <path d="M23 21v-2a4 4 0 0 0-3-3.87" />
    <path d="M16 3.13a4 4 0 0 1 0 7.75" />
  </svg>
);

const AlertIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z" />
    <line x1="12" y1="9" x2="12" y2="13" />
    <line x1="12" y1="17" x2="12.01" y2="17" />
  </svg>
);

const SettingsIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <circle cx="12" cy="12" r="3" />
    <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z" />
  </svg>
);

// Fallback empty data for graceful degradation on API failure
const emptyDashboardData = {
  stats: {
    revenue: { value: 0, trend: "up", change: "0%" },
    orders: { value: 0, trend: "up", change: "0%" },
    customers: { value: 0, trend: "up", change: "0%" },
    lowStock: { value: 0, trend: "up", change: "0" },
  },
  revenueChart: { labels: [], data: [] },
  orderStatus: { labels: [], data: [] },
  recentOrders: [],
  lowStockProducts: [],
};

export default function DashboardPage() {
  const { hasPermission, dashboardCards, updateDashboardCards } = useAdmin();
  const [loading, setLoading] = useState(true);
  const [data, setData] = useState(null);
  const [showCustomize, setShowCustomize] = useState(false);

  useEffect(() => {
    const fetchDashboardData = async () => {
      try {
        const [dashboardRes, ordersRes, lowStockRes, revenueRes] =
          await Promise.all([
            adminAnalytics.dashboard().catch(() => null),
            adminAnalytics.recentOrders(5).catch(() => null),
            adminAnalytics.lowStock().catch(() => null),
            adminAnalytics.revenue({ period: "monthly" }).catch(() => null),
          ]);

        const dashboard = dashboardRes?.data || {};
        const recentOrders = ordersRes?.data || [];
        const lowStockProducts = lowStockRes?.data || [];
        const revenue = revenueRes?.data || {};

        setData({
          stats: dashboard.stats || emptyDashboardData.stats,
          revenueChart: revenue.chart || emptyDashboardData.revenueChart,
          orderStatus: dashboard.orderStatus || emptyDashboardData.orderStatus,
          recentOrders,
          lowStockProducts,
        });
      } catch {
        toast.error("Failed to load dashboard data");
        setData(emptyDashboardData);
      } finally {
        setLoading(false);
      }
    };

    fetchDashboardData();
  }, []);

  const getStatusBadgeClass = (status) => {
    const classes = {
      pending: "status-pending",
      processing: "status-processing",
      shipped: "status-shipped",
      delivered: "status-delivered",
      cancelled: "status-cancelled",
    };
    return classes[status] || "status-pending";
  };

  const isCardVisible = (cardId) => {
    const card = dashboardCards.find(c => c.id === cardId);
    return card ? card.visible : true;
  };

  const toggleCardVisibility = (cardId) => {
    const updatedCards = dashboardCards.map(card =>
      card.id === cardId ? { ...card, visible: !card.visible } : card
    );
    updateDashboardCards(updatedCards);
  };

  return (
    <div className="dashboard-page">
      {/* Page Header */}
      <div className="admin-page-header">
        <h1 className="page-title">Dashboard</h1>
        <div className="page-actions">
          <button
            className="admin-btn btn-secondary"
            onClick={() => setShowCustomize(!showCustomize)}
          >
            <SettingsIcon />
            Customize
          </button>
        </div>
      </div>

      {/* Customize Panel */}
      {showCustomize && (
        <div className="admin-card" style={{ marginBottom: 24 }}>
          <div className="card-body">
            <h4 style={{ margin: "0 0 16px", fontSize: 14, fontWeight: 600 }}>
              Toggle Dashboard Cards
            </h4>
            <div style={{ display: "flex", flexWrap: "wrap", gap: 12 }}>
              {dashboardCards.map(card => (
                <label key={card.id} className="admin-checkbox">
                  <input
                    type="checkbox"
                    checked={card.visible}
                    onChange={() => toggleCardVisibility(card.id)}
                  />
                  <span className="checkbox-label" style={{ textTransform: "capitalize" }}>
                    {card.id.replace(/([A-Z])/g, " $1")}
                  </span>
                </label>
              ))}
            </div>
          </div>
        </div>
      )}

      {/* Stats Row */}
      <div className="dashboard-grid grid-stats" style={{ marginBottom: 24 }}>
        {isCardVisible("revenue") && (
          <StatCard
            icon={<RevenueIcon />}
            iconColor="success"
            value={loading ? "..." : formatCurrency(data?.stats.revenue.value || 0)}
            label="Total Revenue"
            trend={data?.stats.revenue.trend}
            trendValue={data?.stats.revenue.change}
            period="vs last month"
            loading={loading}
          />
        )}

        {isCardVisible("orders") && (
          <StatCard
            icon={<OrdersIcon />}
            iconColor="primary"
            value={loading ? "..." : data?.stats.orders.value?.toLocaleString() || "0"}
            label="Today's Orders"
            trend={data?.stats.orders.trend}
            trendValue={data?.stats.orders.change}
            period="vs yesterday"
            loading={loading}
          />
        )}

        {isCardVisible("customers") && (
          <StatCard
            icon={<CustomersIcon />}
            iconColor="info"
            value={loading ? "..." : data?.stats.customers.value?.toLocaleString() || "0"}
            label="Total Customers"
            trend={data?.stats.customers.trend}
            trendValue={data?.stats.customers.change}
            period="vs last month"
            loading={loading}
          />
        )}

        {isCardVisible("lowStock") && (
          <StatCard
            icon={<AlertIcon />}
            iconColor="warning"
            value={loading ? "..." : data?.stats.lowStock.value?.toString() || "0"}
            label="Low Stock Items"
            trend={data?.stats.lowStock.trend}
            trendValue={data?.stats.lowStock.change}
            period="items below threshold"
            loading={loading}
          />
        )}
      </div>

      {/* Charts Row */}
      <div className="dashboard-grid grid-charts" style={{ marginBottom: 24 }}>
        {isCardVisible("revenueChart") && (
          <ChartCard title="Revenue Overview" loading={loading}>
            {data && (
              <LineChart
                labels={data.revenueChart.labels}
                datasets={[
                  {
                    label: "Revenue",
                    data: data.revenueChart.data,
                    borderColor: "#4f46e5",
                  },
                ]}
                fill={true}
                yAxisPrefix="R$ "
              />
            )}
          </ChartCard>
        )}

        {isCardVisible("orderStatusChart") && (
          <ChartCard title="Orders by Status" loading={loading}>
            {data && (
              <PieChart
                labels={data.orderStatus.labels}
                data={data.orderStatus.data}
                donut={true}
                centerValue={data.orderStatus.data.reduce((a, b) => a + b, 0).toString()}
                centerText="Total Orders"
              />
            )}
          </ChartCard>
        )}
      </div>

      {/* Lists Row */}
      <div className="dashboard-grid grid-lists">
        {isCardVisible("recentOrders") && (
          <ListCard
            title="Recent Orders"
            actionLabel="View All"
            actionHref="/admin/orders"
            items={data?.recentOrders || []}
            loading={loading}
            renderItem={(order) => (
              <>
                <div className="item-content">
                  <div className="item-title">
                    <Link href={`/admin/orders/${order.id}`} style={{ color: "inherit", textDecoration: "none" }}>
                      {order.id}
                    </Link>
                  </div>
                  <div className="item-subtitle">
                    {order.customer} • {formatDate(order.date)}
                  </div>
                </div>
                <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
                  <span className={`status-badge ${getStatusBadgeClass(order.status)}`}>
                    <span className="status-dot" />
                    {order.status}
                  </span>
                  <span className="item-value">{formatCurrency(order.total)}</span>
                </div>
              </>
            )}
          />
        )}

        {isCardVisible("lowStockProducts") && (
          <ListCard
            title="Low Stock Products"
            actionLabel="View All"
            actionHref="/admin/products?stock=low"
            items={data?.lowStockProducts || []}
            loading={loading}
            renderItem={(product) => (
              <>
                <div className="item-avatar" style={{ backgroundColor: "#f3f4f6" }}>
                  {product.image ? (
                    <img src={product.image} alt={product.name} />
                  ) : (
                    <span style={{ fontSize: 12, color: "#9ca3af" }}>N/A</span>
                  )}
                </div>
                <div className="item-content">
                  <div className="item-title">
                    <Link href={`/admin/products/${product.id}`} style={{ color: "inherit", textDecoration: "none" }}>
                      {product.name}
                    </Link>
                  </div>
                </div>
                <span className={`stock-badge ${product.stock <= 2 ? "stock-out" : "stock-low"}`}>
                  {product.stock} left
                </span>
              </>
            )}
          />
        )}
      </div>
    </div>
  );
}
