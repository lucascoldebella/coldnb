"use client";
import React, { useEffect, useState } from "react";
import Image from "next/image";
import Link from "next/link";
import CountdownTimer from "../common/Countdown";
import { useContextElement } from "@/context/Context";
import { calculateShipping } from "@/lib/shopApi";
import { useLanguage } from "@/lib/i18n/LanguageContext";
const discounts = [
  {
    discount: "10% OFF",
    details: "For all orders from 200$",
    code: "Mo234231",
  },
  {
    discount: "10% OFF",
    details: "For all orders from 200$",
    code: "Mo234231",
  },
  {
    discount: "10% OFF",
    details: "For all orders from 200$",
    code: "Mo234231",
  },
];

export default function ShopCart() {
  const { t } = useLanguage();
  const [activeDiscountIndex, setActiveDiscountIndex] = useState(1);
  const [cep, setCep] = useState("");
  const [shippingResult, setShippingResult] = useState(null);
  const [shippingLoading, setShippingLoading] = useState(false);
  const [shippingError, setShippingError] = useState("");
  const { cartProducts, setCartProducts, totalPrice, removeFromCart, updateQuantity } = useContextElement();

  const shippingPrice = shippingResult ? parseFloat(shippingResult.price) : 0;

  const formatCep = (value) => {
    const digits = value.replace(/\D/g, "").slice(0, 8);
    if (digits.length > 5) {
      return digits.slice(0, 5) + "-" + digits.slice(5);
    }
    return digits;
  };

  const handleCepChange = async (e) => {
    const formatted = formatCep(e.target.value);
    setCep(formatted);
    setShippingError("");

    const digits = formatted.replace(/\D/g, "");
    if (digits.length === 8) {
      setShippingLoading(true);
      setShippingResult(null);
      try {
        const result = await calculateShipping(digits);
        setShippingResult(result);
      } catch (err) {
        const errMsg = err.response?.data?.error || err.message || t("checkout.shippingError");
        setShippingError(typeof errMsg === "string" ? errMsg : t("checkout.shippingError"));
        setShippingResult(null);
      } finally {
        setShippingLoading(false);
      }
    } else {
      setShippingResult(null);
    }
  };

  const setQuantity = (id, quantity) => {
    if (quantity >= 1) {
      updateQuantity(id, quantity);
    }
  };
  const removeItem = (id) => {
    removeFromCart(id);
  };

  useEffect(() => {
    document.querySelector(".progress-cart .value").style.width = "70%";
  }, []);

  return (
    <>
      <section className="flat-spacing">
        <div className="container">
          <div className="row">
            <div className="col-xl-8">
              <div className="tf-cart-sold">
                <div className="notification-sold bg-surface">
                  <Image
                    className="icon"
                    alt="img"
                    src="/images/logo/icon-fire.png"
                    width={48}
                    height={49}
                  />
                  <div className="count-text">
                    {t("shopCart.cartExpireIn")}
                    <div
                      className="js-countdown time-count"
                      data-timer={600}
                      data-labels=":,:,:,"
                    >
                      <CountdownTimer
                        style={4}
                        targetDate={new Date(new Date().getTime() - 30 * 60000)}
                      />
                    </div>
                    {t("shopCart.minutesCheckout")}
                  </div>
                </div>
                <div className="notification-progress">
                  <div className="text">
                    {t("shopCart.buy")}
                    <span className="fw-semibold text-primary">
                      $70.00
                    </span>{" "}
                    {t("shopCart.moreToGet")} <span className="fw-semibold">{t("shopCart.freeship")}</span>
                  </div>
                  <div className="progress-cart">
                    <div
                      className="value"
                      style={{ width: "0%" }}
                      data-progress={50}
                    >
                      <span className="round" />
                    </div>
                  </div>
                </div>
              </div>
              {cartProducts.length ? (
                <form onSubmit={(e) => e.preventDefault()}>
                  <table className="tf-table-page-cart">
                    <thead>
                      <tr>
                        <th>{t("shopCart.products")}</th>
                        <th>{t("shopCart.price")}</th>
                        <th>{t("shopCart.quantity")}</th>
                        <th>{t("shopCart.totalPrice")}</th>
                        <th />
                      </tr>
                    </thead>
                    <tbody>
                      {cartProducts.map((elm, i) => (
                        <tr key={i} className="tf-cart-item file-delete">
                          <td className="tf-cart-item_product">
                            <Link
                              href={`/product-detail/${elm.id}`}
                              className="img-box"
                            >
                              <Image
                                alt="product"
                                src={elm.imgSrc}
                                width={600}
                                height={800}
                              />
                            </Link>
                            <div className="cart-info">
                              <Link
                                href={`/product-detail/${elm.id}`}
                                className="cart-title link"
                              >
                                {elm.title}
                              </Link>
                              <div className="variant-box">
                                <div className="tf-select">
                                  <select>
                                    <option>Blue</option>
                                    <option>Black</option>
                                    <option>White</option>
                                    <option>Red</option>
                                    <option>Beige</option>
                                    <option>Pink</option>
                                  </select>
                                </div>
                                <div className="tf-select">
                                  <select>
                                    <option>XL</option>
                                    <option>XS</option>
                                    <option>S</option>
                                    <option>M</option>
                                    <option>L</option>
                                    <option>XL</option>
                                    <option>2XL</option>
                                  </select>
                                </div>
                              </div>
                            </div>
                          </td>
                          <td
                            data-cart-title="Price"
                            className="tf-cart-item_price text-center"
                          >
                            <div className="cart-price text-button price-on-sale">
                              ${elm.price.toFixed(2)}
                            </div>
                          </td>
                          <td
                            data-cart-title="Quantity"
                            className="tf-cart-item_quantity"
                          >
                            <div className="wg-quantity mx-md-auto">
                              <span
                                className="btn-quantity btn-decrease"
                                onClick={() =>
                                  setQuantity(elm.id, elm.quantity - 1)
                                }
                              >
                                -
                              </span>
                              <input
                                type="text"
                                className="quantity-product"
                                name="number"
                                value={elm.quantity}
                                readOnly
                              />
                              <span
                                className="btn-quantity btn-increase"
                                onClick={() =>
                                  setQuantity(elm.id, elm.quantity + 1)
                                }
                              >
                                +
                              </span>
                            </div>
                          </td>
                          <td
                            data-cart-title="Total"
                            className="tf-cart-item_total text-center"
                          >
                            <div className="cart-total text-button total-price">
                              ${(elm.price * elm.quantity).toFixed(2)}
                            </div>
                          </td>
                          <td
                            data-cart-title="Remove"
                            className="remove-cart"
                            onClick={() => removeItem(elm.id)}
                          >
                            <span className="remove icon icon-close" />
                          </td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                  <div className="ip-discount-code">
                    <input type="text" placeholder={t("shopCart.addVoucherDiscount")} />
                    <button className="tf-btn">
                      <span className="text">{t("shopCart.applyCode")}</span>
                    </button>
                  </div>
                  <div className="group-discount">
                    {discounts.map((item, index) => (
                      <div
                        key={index}
                        className={`box-discount ${
                          activeDiscountIndex === index ? "active" : ""
                        }`}
                        onClick={() => setActiveDiscountIndex(index)}
                      >
                        <div className="discount-top">
                          <div className="discount-off">
                            <div className="text-caption-1">{t("shopCart.discount")}</div>
                            <span className="sale-off text-btn-uppercase">
                              {item.discount}
                            </span>
                          </div>
                          <div className="discount-from">
                            <p className="text-caption-1">{item.details}</p>
                          </div>
                        </div>
                        <div className="discount-bot">
                          <span className="text-btn-uppercase">
                            {item.code}
                          </span>
                          <button className="tf-btn">
                            <span className="text">{t("shopCart.applyCode")}</span>
                          </button>
                        </div>
                      </div>
                    ))}
                  </div>
                </form>
              ) : (
                <div>
                  {t("shopCart.emptyWishlist")}{" "}
                  <Link className="btn-line" href="/shop-default-grid">
                    {t("shopCart.exploreProducts")}
                  </Link>
                </div>
              )}
            </div>
            <div className="col-xl-4">
              <div className="fl-sidebar-cart">
                <div className="box-order bg-surface">
                  <h5 className="title">{t("shopCart.orderSummary")}</h5>
                  <div className="subtotal text-button d-flex justify-content-between align-items-center">
                    <span>{t("shopCart.subtotal")}</span>
                    <span className="total">${totalPrice.toFixed(2)}</span>
                  </div>
                  <div className="discount text-button d-flex justify-content-between align-items-center">
                    <span>{t("shopCart.discounts")}</span>
                    <span className="total">-${totalPrice ? "20.00" : "0.00"}</span>
                  </div>
                  <div className="ship">
                    <span className="text-button">{t("shopCart.shipping")}</span>
                    <div className="flex-grow-1">
                      <div style={{ marginTop: 8 }}>
                        <div style={{ display: "flex", gap: 8, alignItems: "center" }}>
                          <input
                            type="text"
                            placeholder={t("shopCart.enterCep")}
                            value={cep}
                            onChange={handleCepChange}
                            style={{
                              flex: 1,
                              padding: "8px 12px",
                              border: "1px solid #e5e5e5",
                              borderRadius: 4,
                              fontSize: 14,
                            }}
                          />
                          {shippingLoading && (
                            <span style={{ fontSize: 13, color: "#888" }}>...</span>
                          )}
                        </div>
                        {shippingError && (
                          <p style={{ color: "#dc3545", fontSize: 12, marginTop: 4, marginBottom: 0 }}>
                            {shippingError}
                          </p>
                        )}
                        {shippingResult && (
                          <div style={{ marginTop: 8, fontSize: 13 }}>
                            <div className="d-flex justify-content-between">
                              <span>
                                {shippingResult.city}, {shippingResult.state} ({shippingResult.zone})
                              </span>
                              <span className="fw-semibold">
                                R$ {parseFloat(shippingResult.price).toFixed(2)}
                              </span>
                            </div>
                            <p style={{ color: "#888", fontSize: 12, marginTop: 2, marginBottom: 0 }}>
                              {t("shopCart.delivery")} {shippingResult.estimated_days_min}-{shippingResult.estimated_days_max} {t("shopCart.businessDays")}
                            </p>
                          </div>
                        )}
                        {!shippingResult && !shippingLoading && !shippingError && (
                          <p style={{ color: "#888", fontSize: 12, marginTop: 4, marginBottom: 0 }}>
                            {t("shopCart.enterCepCalculate")}
                          </p>
                        )}
                      </div>
                    </div>
                  </div>
                  <h5 className="total-order d-flex justify-content-between align-items-center">
                    <span>{t("shopCart.total")}</span>
                    <span className="total">
                      $
                      {totalPrice
                        ? (totalPrice - 20 + shippingPrice).toFixed(2)
                        : "0.00"}
                    </span>
                  </h5>
                  <div className="box-progress-checkout">
                    <fieldset className="check-agree">
                      <input
                        type="checkbox"
                        id="check-agree"
                        className="tf-check-rounded"
                      />
                      <label htmlFor="check-agree">
                        {t("shopCart.agreeTerms")}
                        <Link href={`/term-of-use`}>{t("shopCart.termsConditions")}</Link>
                      </label>
                    </fieldset>
                    <Link href={`/checkout`} className="tf-btn btn-reset">
                      {t("shopCart.processCheckout")}
                    </Link>
                    <p className="text-button text-center">
                      {t("shopCart.continueShopping")}
                    </p>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </section>
    </>
  );
}
