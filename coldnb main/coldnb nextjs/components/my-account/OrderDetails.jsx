"use client";
import React, { useState, useEffect } from "react";
import Image from "next/image";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { ordersApi } from "@/lib/userApi";

export default function OrderDetails({ orderId }) {
  const { t } = useLanguage();
  const [activeTab, setActiveTab] = useState(1);
  const [order, setOrder] = useState(null);
  const [isLoading, setIsLoading] = useState(true);

  useEffect(() => {
    if (!orderId) {
      setIsLoading(false);
      return;
    }
    const fetchOrder = async () => {
      try {
        const response = await ordersApi.get(orderId);
        const data = response.data?.data || response.data;
        setOrder(data);
      } catch {
        setOrder(null);
      }
      setIsLoading(false);
    };
    fetchOrder();
  }, [orderId]);

  const formatDate = (dateStr) => {
    if (!dateStr) return "";
    const date = new Date(dateStr);
    return date.toLocaleDateString("pt-BR", {
      year: "numeric",
      month: "long",
      day: "numeric",
      hour: "2-digit",
      minute: "2-digit",
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

  if (!order) {
    return (
      <div className="my-account-content">
        <div className="text-center" style={{ padding: 40 }}>
          <p className="text-secondary">{t("orders.noOrders")}</p>
          <Link href="/my-account-orders" className="tf-btn btn-fill radius-4 mt_20">
            <span className="text">{t("auth.back")}</span>
          </Link>
        </div>
      </div>
    );
  }

  const items = order.items || [];
  const history = order.history || [];
  const firstItem = items[0];

  return (
    <div className="my-account-content">
      <div className="account-order-details">
        <div className="wd-form-order">
          <div className="order-head">
            {firstItem?.product_image && (
              <figure className="img-product">
                <Image
                  alt={firstItem.product_name || "product"}
                  src={firstItem.product_image}
                  width={600}
                  height={800}
                />
              </figure>
            )}
            <div className="content">
              <div className="badge">{getStatusLabel(order.status)}</div>
              <h6 className="mt-8 fw-5">
                {t("orders.order")} #{order.order_number || order.id}
              </h6>
            </div>
          </div>
          <div className="tf-grid-layout md-col-2 gap-15">
            <div className="item">
              <div className="text-2 text_black-2">{t("orders.date")}</div>
              <div className="text-2 mt_4 fw-6">{formatDate(order.created_at)}</div>
            </div>
            <div className="item">
              <div className="text-2 text_black-2">{t("orders.status")}</div>
              <div className="text-2 mt_4 fw-6">{getStatusLabel(order.status)}</div>
            </div>
            <div className="item">
              <div className="text-2 text_black-2">{t("orders.total")}</div>
              <div className="text-2 mt_4 fw-6">{formatCurrency(order.total)}</div>
            </div>
            {order.shipping_address && (
              <div className="item">
                <div className="text-2 text_black-2">{t("orders.shippingAddress")}</div>
                <div className="text-2 mt_4 fw-6">{order.shipping_address}</div>
              </div>
            )}
          </div>
          <div className="widget-tabs style-3 widget-order-tab">
            <ul className="widget-menu-tab">
              <li
                className={`item-title ${activeTab == 1 ? "active" : ""}`}
                onClick={() => setActiveTab(1)}
              >
                <span className="inner">{t("orders.statusTimeline")}</span>
              </li>
              <li
                className={`item-title ${activeTab == 2 ? "active" : ""}`}
                onClick={() => setActiveTab(2)}
              >
                <span className="inner">{t("orders.itemDetails")}</span>
              </li>
            </ul>
            <div className="widget-content-tab">
              {/* Tab 1: Status Timeline */}
              <div className={`widget-content-inner ${activeTab == 1 ? "active" : ""}`}>
                {history.length > 0 ? (
                  <div className="widget-timeline">
                    <ul className="timeline">
                      {history.map((entry, idx) => (
                        <li key={idx}>
                          <div className={`timeline-badge ${idx === 0 ? "success" : ""}`} />
                          <div className="timeline-box">
                            <a className="timeline-panel" href="#">
                              <div className="text-2 fw-6">{getStatusLabel(entry.status)}</div>
                              <span>{formatDate(entry.created_at)}</span>
                            </a>
                            {entry.note && <p>{entry.note}</p>}
                          </div>
                        </li>
                      ))}
                    </ul>
                  </div>
                ) : (
                  <div className="widget-timeline">
                    <ul className="timeline">
                      <li>
                        <div className="timeline-badge success" />
                        <div className="timeline-box">
                          <a className="timeline-panel" href="#">
                            <div className="text-2 fw-6">{getStatusLabel(order.status)}</div>
                            <span>{formatDate(order.created_at)}</span>
                          </a>
                        </div>
                      </li>
                    </ul>
                  </div>
                )}
              </div>

              {/* Tab 2: Item Details */}
              <div className={`widget-content-inner ${activeTab == 2 ? "active" : ""}`}>
                {items.map((item, idx) => (
                  <div className="order-head mb_20" key={idx}>
                    {item.product_image && (
                      <figure className="img-product">
                        <Image
                          alt={item.product_name || "product"}
                          src={item.product_image}
                          width={600}
                          height={800}
                        />
                      </figure>
                    )}
                    <div className="content">
                      <div className="text-2 fw-6">{item.product_name}</div>
                      <div className="mt_4">
                        <span className="fw-6">{t("orders.quantity")} :</span> {item.quantity}
                      </div>
                      <div className="mt_4">
                        <span className="fw-6">{t("orders.unitPrice")} :</span> {formatCurrency(item.unit_price)}
                      </div>
                    </div>
                  </div>
                ))}
                <ul>
                  <li className="d-flex justify-content-between text-2">
                    <span>{t("orders.subtotal")}</span>
                    <span className="fw-6">{formatCurrency(order.subtotal || order.total)}</span>
                  </li>
                  {order.shipping_cost != null && (
                    <li className="d-flex justify-content-between text-2 mt_4">
                      <span>{t("orders.shippingCost")}</span>
                      <span className="fw-6">{formatCurrency(order.shipping_cost)}</span>
                    </li>
                  )}
                  {order.discount_amount != null && order.discount_amount > 0 && (
                    <li className="d-flex justify-content-between text-2 mt_4 pb_8 line-bt">
                      <span>{t("orders.discount")}</span>
                      <span className="fw-6">-{formatCurrency(order.discount_amount)}</span>
                    </li>
                  )}
                  <li className="d-flex justify-content-between text-2 mt_8">
                    <span>{t("orders.orderTotal")}</span>
                    <span className="fw-6">{formatCurrency(order.total)}</span>
                  </li>
                </ul>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
