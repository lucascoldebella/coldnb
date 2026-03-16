"use client";
import React, { useState, useEffect } from "react";
import Image from "next/image";
import Link from "next/link";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { ordersApi } from "@/lib/userApi";
import { useContextElement } from "@/context/Context";
import toast from "react-hot-toast";

export default function OrderDetails({ orderId }) {
  const { t } = useLanguage();
  const { addProductToCart } = useContextElement();
  const [activeTab, setActiveTab] = useState(1);
  const [order, setOrder] = useState(null);
  const [isLoading, setIsLoading] = useState(true);
  const [reordering, setReordering] = useState(false);
  const [reorderDone, setReorderDone] = useState(false);
  const [showReturnForm, setShowReturnForm] = useState(false);
  const [returnReason, setReturnReason] = useState("defective");
  const [returnDescription, setReturnDescription] = useState("");
  const [returnLoading, setReturnLoading] = useState(false);
  const [returnSuccess, setReturnSuccess] = useState(false);
  const [returnError, setReturnError] = useState("");
  const [showCancelConfirm, setShowCancelConfirm] = useState(false);
  const [cancelLoading, setCancelLoading] = useState(false);

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

  const handleReorder = () => {
    if (items.length === 0) return;
    setReordering(true);
    items.forEach((item) => {
      addProductToCart({
        id: item.product_id,
        title: item.product_name,
        price: parseFloat(item.unit_price) || 0,
        imgSrc: item.product_image || "/images/products/placeholder.jpg",
        quantity: item.quantity,
      });
    });
    setReordering(false);
    setReorderDone(true);
    setTimeout(() => setReorderDone(false), 3000);
  };

  const handleSubmitReturn = async () => {
    setReturnLoading(true);
    setReturnError("");
    try {
      const { default: userApi } = await import("@/lib/userApi");
      const res = await userApi.post("/api/returns", {
        order_id: order.id,
        reason: returnReason,
        description: returnDescription || null,
      });
      if (res.data?.success || res.status === 201) {
        setReturnSuccess(true);
        setShowReturnForm(false);
      }
    } catch (err) {
      setReturnError(err.response?.data?.error || t("returns.submitError"));
    } finally {
      setReturnLoading(false);
    }
  };

  const handleCancelOrder = async () => {
    setCancelLoading(true);
    try {
      await ordersApi.cancel(order.id);
      setOrder((prev) => ({ ...prev, status: "cancelled" }));
      setShowCancelConfirm(false);
      toast.success(t("orders.cancelSuccess"));
    } catch (err) {
      toast.error(err.response?.data?.error || t("orders.cancelError"));
    } finally {
      setCancelLoading(false);
    }
  };

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
          <div className="d-flex gap-12 mt_16 mb_16 flex-wrap">
            <button
              className="tf-btn btn-fill radius-4"
              onClick={handleReorder}
              disabled={reordering || items.length === 0}
            >
              <span className="text">
                {reorderDone ? t("orders.reorderDone") : t("orders.reorder")}
              </span>
            </button>
            <Link
              href={`/invoice/${order.id}`}
              target="_blank"
              className="tf-btn btn-outline radius-4"
            >
              <span className="text">{t("orders.invoice")}</span>
            </Link>
            {order.status === "delivered" && !returnSuccess && (
              <button
                className="tf-btn btn-outline radius-4"
                onClick={() => setShowReturnForm(!showReturnForm)}
              >
                <span className="text">{t("orders.requestReturn")}</span>
              </button>
            )}
            {(order.status === "pending" || order.status === "processing") && (
              !showCancelConfirm ? (
                <button
                  className="tf-btn btn-outline radius-4"
                  onClick={() => setShowCancelConfirm(true)}
                  style={{ borderColor: "#dc3545", color: "#dc3545" }}
                >
                  <span className="text">{t("orders.cancelOrder")}</span>
                </button>
              ) : (
                <div className="d-flex align-items-center gap-8">
                  <span style={{ fontSize: 13, color: "#dc3545" }}>{t("orders.cancelOrderConfirm")}</span>
                  <button
                    className="tf-btn radius-4"
                    onClick={handleCancelOrder}
                    disabled={cancelLoading}
                    style={{ background: "#dc3545", borderColor: "#dc3545", color: "#fff" }}
                  >
                    <span className="text">{cancelLoading ? "..." : t("common.confirm")}</span>
                  </button>
                  <button
                    className="tf-btn btn-outline radius-4"
                    onClick={() => setShowCancelConfirm(false)}
                  >
                    <span className="text">{t("common.cancel")}</span>
                  </button>
                </div>
              )
            )}
          </div>

          {returnSuccess && (
            <div className="alert alert-success mb_16" style={{ fontSize: 13 }}>
              {t("returns.submitSuccess")}
            </div>
          )}

          {showReturnForm && (
            <div className="p_16 mb_20" style={{ background: "#f8f9fa", borderRadius: 8 }}>
              <h6 className="mb_12">{t("returns.formTitle")}</h6>
              <div className="form-group mb_12">
                <label className="form-label">{t("returns.reason")}</label>
                <select
                  className="form-control"
                  value={returnReason}
                  onChange={(e) => setReturnReason(e.target.value)}
                >
                  <option value="defective">{t("returns.reasonDefective")}</option>
                  <option value="wrong_item">{t("returns.reasonWrongItem")}</option>
                  <option value="changed_mind">{t("returns.reasonChangedMind")}</option>
                  <option value="other">{t("returns.reasonOther")}</option>
                </select>
              </div>
              <div className="form-group mb_12">
                <label className="form-label">{t("returns.description")}</label>
                <textarea
                  className="form-control"
                  rows={3}
                  placeholder={t("returns.descriptionPlaceholder")}
                  value={returnDescription}
                  onChange={(e) => setReturnDescription(e.target.value)}
                />
              </div>
              {returnError && (
                <p style={{ color: "#dc3545", fontSize: 13, marginBottom: 8 }}>{returnError}</p>
              )}
              <div className="d-flex gap-8">
                <button
                  className="tf-btn btn-fill radius-4"
                  onClick={handleSubmitReturn}
                  disabled={returnLoading}
                >
                  <span className="text">{returnLoading ? t("common.loading") : t("returns.submit")}</span>
                </button>
                <button
                  className="tf-btn btn-outline radius-4"
                  onClick={() => setShowReturnForm(false)}
                >
                  <span className="text">{t("common.cancel")}</span>
                </button>
              </div>
            </div>
          )}
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
