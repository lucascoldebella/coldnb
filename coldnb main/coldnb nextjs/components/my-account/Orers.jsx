"use client";
import React, { useState, useEffect } from "react";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { ordersApi } from "@/lib/userApi";

export default function Orers() {
  const { t } = useLanguage();
  const [orders, setOrders] = useState([]);
  const [isLoading, setIsLoading] = useState(true);

  useEffect(() => {
    const fetchOrders = async () => {
      try {
        const response = await ordersApi.list({ page: 1, limit: 20 });
        const data = response.data?.data || response.data?.orders || response.data || [];
        setOrders(Array.isArray(data) ? data : []);
      } catch {
        setOrders([]);
      }
      setIsLoading(false);
    };
    fetchOrders();
  }, []);

  const formatDate = (dateStr) => {
    if (!dateStr) return "";
    const date = new Date(dateStr);
    return date.toLocaleDateString("pt-BR", {
      year: "numeric",
      month: "long",
      day: "numeric",
    });
  };

  const formatCurrency = (value) => {
    if (value == null) return "R$ 0,00";
    return new Intl.NumberFormat("pt-BR", {
      style: "currency",
      currency: "BRL",
    }).format(value);
  };

  const getStatusLabel = (status) => {
    const map = {
      pending: t("orders.pending"),
      processing: t("orders.processing"),
      shipped: t("orders.shipped"),
      delivered: t("orders.delivered"),
      cancelled: t("orders.cancelled"),
      on_hold: t("orders.onHold"),
    };
    return map[status] || status;
  };

  if (isLoading) {
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
      <div className="account-orders">
        <div className="wrap-account-order">
          {orders.length === 0 ? (
            <div className="text-center" style={{ padding: 40 }}>
              <p className="text-secondary">{t("orders.noOrders")}</p>
              <Link href="/shop" className="tf-btn btn-fill radius-4 mt_20">
                <span className="text">{t("nav.shop")}</span>
              </Link>
            </div>
          ) : (
            <table>
              <thead>
                <tr>
                  <th className="fw-6">{t("orders.order")}</th>
                  <th className="fw-6">{t("orders.date")}</th>
                  <th className="fw-6">{t("orders.status")}</th>
                  <th className="fw-6">{t("orders.total")}</th>
                  <th className="fw-6">{t("orders.actions")}</th>
                </tr>
              </thead>
              <tbody>
                {orders.map((order) => (
                  <tr className="tf-order-item" key={order.id}>
                    <td>#{order.order_number || order.id}</td>
                    <td>{formatDate(order.created_at)}</td>
                    <td>{getStatusLabel(order.status)}</td>
                    <td>
                      {formatCurrency(order.total)}{" "}
                      {order.item_count != null && (
                        <span>
                          {t("orders.items").replace("itens", `${order.item_count} ${t("orders.items")}`)}
                        </span>
                      )}
                    </td>
                    <td>
                      <Link
                        href={`/my-account-orders-details?id=${order.id}`}
                        className="tf-btn btn-fill radius-4"
                      >
                        <span className="text">{t("orders.view")}</span>
                      </Link>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          )}
        </div>
      </div>
    </div>
  );
}
