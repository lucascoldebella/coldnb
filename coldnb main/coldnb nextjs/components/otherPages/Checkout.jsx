"use client";

import { useContextElement } from "@/context/Context";
import { calculateShipping } from "@/lib/shopApi";
import Image from "next/image";
import Link from "next/link";
import { useState } from "react";
import { Swiper, SwiperSlide } from "swiper/react";
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
export default function Checkout() {
  const { t } = useLanguage();
  const [activeDiscountIndex, setActiveDiscountIndex] = useState(1);
  const [cep, setCep] = useState("");
  const [shippingResult, setShippingResult] = useState(null);
  const [shippingLoading, setShippingLoading] = useState(false);
  const [shippingError, setShippingError] = useState("");
  const { cartProducts, totalPrice } = useContextElement();

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
  return (
    <section>
      <div className="container">
        <div className="row">
          <div className="col-xl-6">
            <div className="flat-spacing tf-page-checkout">
              <div className="wrap">
                <div className="title-login">
                  <p>{t("checkout.alreadyHaveAccount")}</p>{" "}
                  <Link href={`/login`} className="text-button">
                    {t("checkout.loginHere")}
                  </Link>
                </div>
                <form
                  className="login-box"
                  onSubmit={(e) => e.preventDefault()}
                >
                  <div className="grid-2">
                    <input type="text" placeholder={t("checkout.yourNameEmail")} />
                    <input type="password" placeholder={t("checkout.password")} />
                  </div>
                  <button className="tf-btn" type="submit">
                    <span className="text">{t("checkout.login")}</span>
                  </button>
                </form>
              </div>
              <div className="wrap">
                <h5 className="title">{t("checkout.information")}</h5>
                <form className="info-box" onSubmit={(e) => e.preventDefault()}>
                  <div className="grid-2">
                    <input type="text" placeholder={t("checkout.firstName")} />
                    <input type="text" placeholder={t("checkout.lastName")} />
                  </div>
                  <div className="grid-2">
                    <input type="text" placeholder={t("checkout.emailAddress")} />
                    <input type="text" placeholder={t("checkout.phoneNumber")} />
                  </div>
                  <div className="grid-2">
                    <div>
                      <input
                        type="text"
                        placeholder={t("checkout.cepPlaceholder")}
                        value={cep}
                        onChange={handleCepChange}
                      />
                      {shippingLoading && (
                        <p style={{ fontSize: 12, color: "#888", marginTop: 4, marginBottom: 0 }}>
                          {t("checkout.calculatingShipping")}
                        </p>
                      )}
                      {shippingError && (
                        <p style={{ fontSize: 12, color: "#dc3545", marginTop: 4, marginBottom: 0 }}>
                          {shippingError}
                        </p>
                      )}
                      {shippingResult && (
                        <p style={{ fontSize: 12, color: "#28a745", marginTop: 4, marginBottom: 0 }}>
                          {shippingResult.city}, {shippingResult.state} - R$ {parseFloat(shippingResult.price).toFixed(2)} ({shippingResult.estimated_days_min}-{shippingResult.estimated_days_max} {t("checkout.days")})
                        </p>
                      )}
                    </div>
                    <input type="text" placeholder={t("checkout.neighborhood")} />
                  </div>
                  <div className="grid-2">
                    <input type="text" placeholder={t("checkout.townCity")} />
                    <input type="text" placeholder={t("checkout.streetNumber")} />
                  </div>
                  <div className="grid-2">
                    <div className="tf-select">
                      <select className="text-title" data-default="">
                        <option value="">{t("checkout.state")}</option>
                        <option value="AC">AC</option>
                        <option value="AL">AL</option>
                        <option value="AP">AP</option>
                        <option value="AM">AM</option>
                        <option value="BA">BA</option>
                        <option value="CE">CE</option>
                        <option value="DF">DF</option>
                        <option value="ES">ES</option>
                        <option value="GO">GO</option>
                        <option value="MA">MA</option>
                        <option value="MT">MT</option>
                        <option value="MS">MS</option>
                        <option value="MG">MG</option>
                        <option value="PA">PA</option>
                        <option value="PB">PB</option>
                        <option value="PR">PR</option>
                        <option value="PE">PE</option>
                        <option value="PI">PI</option>
                        <option value="RJ">RJ</option>
                        <option value="RN">RN</option>
                        <option value="RS">RS</option>
                        <option value="RO">RO</option>
                        <option value="RR">RR</option>
                        <option value="SC">SC</option>
                        <option value="SP">SP</option>
                        <option value="SE">SE</option>
                        <option value="TO">TO</option>
                      </select>
                    </div>
                    <input type="text" placeholder={t("checkout.complement")} />
                  </div>
                  <textarea placeholder={t("checkout.writeNote")} defaultValue={""} />
                </form>
              </div>
              <div className="wrap">
                <h5 className="title">{t("checkout.choosePayment")}</h5>
                <form
                  className="form-payment"
                  onSubmit={(e) => e.preventDefault()}
                >
                  <div className="payment-box" id="payment-box">
                    <div className="payment-item payment-choose-card active">
                      <label
                        htmlFor="credit-card-method"
                        className="payment-header"
                        data-bs-toggle="collapse"
                        data-bs-target="#credit-card-payment"
                        aria-controls="credit-card-payment"
                      >
                        <input
                          type="radio"
                          name="payment-method"
                          className="tf-check-rounded"
                          id="credit-card-method"
                          defaultChecked
                        />
                        <span className="text-title">{t("checkout.creditCard")}</span>
                      </label>
                      <div
                        id="credit-card-payment"
                        className="collapse show"
                        data-bs-parent="#payment-box"
                      >
                        <div className="payment-body">
                          <p className="text-secondary">
                            {t("checkout.creditCardDesc")}
                          </p>
                          <div className="input-payment-box">
                            <input type="text" placeholder={t("checkout.nameOnCard")} />
                            <div className="ip-card">
                              <input type="text" placeholder={t("checkout.cardNumbers")} />
                              <div className="list-card">
                                <Image
                                  width={48}
                                  height={16}
                                  alt="card"
                                  src="/images/payment/img-7.png"
                                />
                                <Image
                                  width={21}
                                  height={16}
                                  alt="card"
                                  src="/images/payment/img-8.png"
                                />
                                <Image
                                  width={22}
                                  height={16}
                                  alt="card"
                                  src="/images/payment/img-9.png"
                                />
                                <Image
                                  width={24}
                                  height={16}
                                  alt="card"
                                  src="/images/payment/img-10.png"
                                />
                              </div>
                            </div>
                            <div className="grid-2">
                              <input type="date" />
                              <input type="text" placeholder={t("checkout.cvv")} />
                            </div>
                          </div>
                          <div className="check-save">
                            <input
                              type="checkbox"
                              className="tf-check"
                              id="check-card"
                              defaultChecked
                            />
                            <label htmlFor="check-card">
                              {t("checkout.saveCardDetails")}
                            </label>
                          </div>
                        </div>
                      </div>
                    </div>
                    <div className="payment-item">
                      <label
                        htmlFor="delivery-method"
                        className="payment-header collapsed"
                        data-bs-toggle="collapse"
                        data-bs-target="#delivery-payment"
                        aria-controls="delivery-payment"
                      >
                        <input
                          type="radio"
                          name="payment-method"
                          className="tf-check-rounded"
                          id="delivery-method"
                        />
                        <span className="text-title">{t("checkout.cashOnDelivery")}</span>
                      </label>
                      <div
                        id="delivery-payment"
                        className="collapse"
                        data-bs-parent="#payment-box"
                      />
                    </div>
                    <div className="payment-item">
                      <label
                        htmlFor="apple-method"
                        className="payment-header collapsed"
                        data-bs-toggle="collapse"
                        data-bs-target="#apple-payment"
                        aria-controls="apple-payment"
                      >
                        <input
                          type="radio"
                          name="payment-method"
                          className="tf-check-rounded"
                          id="apple-method"
                        />
                        <span className="text-title apple-pay-title">
                          <Image
                            alt="apple"
                            src="/images/payment/applePay.png"
                            width={13}
                            height={18}
                          />
                          Apple Pay
                        </span>
                      </label>
                      <div
                        id="apple-payment"
                        className="collapse"
                        data-bs-parent="#payment-box"
                      />
                    </div>
                    <div className="payment-item paypal-item">
                      <label
                        htmlFor="paypal-method"
                        className="payment-header collapsed"
                        data-bs-toggle="collapse"
                        data-bs-target="#paypal-method-payment"
                        aria-controls="paypal-method-payment"
                      >
                        <input
                          type="radio"
                          name="payment-method"
                          className="tf-check-rounded"
                          id="paypal-method"
                        />
                        <span className="paypal-title">
                          <Image
                            alt="apple"
                            src="/images/payment/paypal.png"
                            width={90}
                            height={23}
                          />
                        </span>
                      </label>
                      <div
                        id="paypal-method-payment"
                        className="collapse"
                        data-bs-parent="#payment-box"
                      />
                    </div>
                  </div>
                  <button className="tf-btn btn-reset">{t("checkout.payment")}</button>
                </form>
              </div>
            </div>
          </div>
          <div className="col-xl-1">
            <div className="line-separation" />
          </div>
          <div className="col-xl-5">
            <div className="flat-spacing flat-sidebar-checkout">
              <div className="sidebar-checkout-content">
                <h5 className="title">{t("checkout.shoppingCart")}</h5>
                <div className="list-product">
                  {cartProducts.map((elm, i) => (
                    <div key={i} className="item-product">
                      <Link
                        href={`/product-detail/${elm.id}`}
                        className="img-product"
                      >
                        <Image
                          alt="img-product"
                          src={elm.imgSrc}
                          width={600}
                          height={800}
                        />
                      </Link>
                      <div className="content-box">
                        <div className="info">
                          <Link
                            href={`/product-detail/${elm.id}`}
                            className="name-product link text-title"
                          >
                            {elm.title}
                          </Link>
                          <div className="variant text-caption-1 text-secondary">
                            <span className="size">XL</span>/
                            <span className="color">Blue</span>
                          </div>
                        </div>
                        <div className="total-price text-button">
                          <span className="count">{elm.quantity}</span>X
                          <span className="price">${elm.price.toFixed(2)}</span>
                        </div>
                      </div>
                    </div>
                  ))}
                </div>
                <div className="sec-discount">
                  <Swiper
                    dir="ltr"
                    className="swiper tf-sw-categories"
                    slidesPerView={2.25} // data-preview="2.25"
                    breakpoints={{
                      1024: {
                        slidesPerView: 2.25, // data-tablet={3}
                      },
                      768: {
                        slidesPerView: 3, // data-tablet={3}
                      },
                      640: {
                        slidesPerView: 2.5, // data-mobile-sm="2.5"
                      },
                      0: {
                        slidesPerView: 1.2, // data-mobile="1.2"
                      },
                    }}
                    spaceBetween={20}
                  >
                    {discounts.map((item, index) => (
                      <SwiperSlide key={index}>
                        <div
                          className={`box-discount ${
                            activeDiscountIndex === index ? "active" : ""
                          }`}
                          onClick={() => setActiveDiscountIndex(index)}
                        >
                          <div className="discount-top">
                            <div className="discount-off">
                              <div className="text-caption-1">{t("checkout.discount")}</div>
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
                              <span className="text">{t("checkout.applyCode")}</span>
                            </button>
                          </div>
                        </div>{" "}
                      </SwiperSlide>
                    ))}
                  </Swiper>
                  <div className="ip-discount-code">
                    <input type="text" placeholder={t("checkout.addVoucherDiscount")} />
                    <button className="tf-btn">
                      <span className="text">{t("checkout.applyCode")}</span>
                    </button>
                  </div>
                  <p>
                    {t("checkout.discountNote")}
                  </p>
                </div>
                <div className="sec-total-price">
                  <div className="top">
                    <div className="item d-flex align-items-center justify-content-between text-button">
                      <span>{t("checkout.shipping")}</span>
                      <span>
                        {shippingResult
                          ? `R$ ${parseFloat(shippingResult.price).toFixed(2)}`
                          : t("checkout.enterCep")}
                      </span>
                    </div>
                    <div className="item d-flex align-items-center justify-content-between text-button">
                      <span>{t("checkout.discounts")}</span>
                      <span>-${totalPrice ? "20.00" : "0.00"}</span>
                    </div>
                  </div>
                  <div className="bottom">
                    <h5 className="d-flex justify-content-between">
                      <span>{t("checkout.total")}</span>
                      <span className="total-price-checkout">
                        ${totalPrice ? (totalPrice - 20 + shippingPrice).toFixed(2) : "0.00"}
                      </span>
                    </h5>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
