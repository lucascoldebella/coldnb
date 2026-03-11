"use client";

import { useContextElement } from "@/context/Context";
import { useUserAuth } from "@/context/UserAuthContext";
import { calculateShipping } from "@/lib/shopApi";
import { addressesApi, cartApi } from "@/lib/userApi";
import userApi from "@/lib/userApi";
import { buildApiUrl } from "@/lib/apiBase";
import Image from "next/image";
import Link from "next/link";
import { useRouter } from "next/navigation";
import { useState, useEffect } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { loadStripe } from "@stripe/stripe-js";
import { Elements, PaymentElement, useStripe, useElements } from "@stripe/react-stripe-js";

const stripePromise = process.env.NEXT_PUBLIC_STRIPE_PUBLISHABLE_KEY
  ? loadStripe(process.env.NEXT_PUBLIC_STRIPE_PUBLISHABLE_KEY)
  : null;

function PaymentForm({ orderId, orderNumber, onSuccess, onError }) {
  const { t } = useLanguage();
  const stripe = useStripe();
  const elements = useElements();
  const [processing, setProcessing] = useState(false);
  const [errorMsg, setErrorMsg] = useState("");

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!stripe || !elements || processing) return;

    setProcessing(true);
    setErrorMsg("");

    const { error } = await stripe.confirmPayment({
      elements,
      confirmParams: {
        return_url: `${window.location.origin}/my-account-orders-details?order=${orderId}`,
      },
    });

    if (error) {
      setErrorMsg(error.message || t("checkout.paymentFailed"));
      setProcessing(false);
    }
  };

  return (
    <form onSubmit={handleSubmit}>
      <PaymentElement />
      {errorMsg && (
        <p style={{ color: "#dc3545", marginTop: 12 }}>{errorMsg}</p>
      )}
      <button
        className="tf-btn btn-fill w-100 justify-content-center mt_20"
        type="submit"
        disabled={!stripe || processing}
      >
        <span className="text text-button">
          {processing ? t("checkout.processing") : t("checkout.confirmPayment")}
        </span>
      </button>
    </form>
  );
}

export default function Checkout() {
  const { t } = useLanguage();
  const router = useRouter();
  const { isAuthenticated, isLoading: authLoading, session } = useUserAuth();
  const { cartProducts, totalPrice, setCartProducts } = useContextElement();

  const [step, setStep] = useState("shipping"); // "shipping" | "payment"
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");

  // Address fields
  const [firstName, setFirstName] = useState("");
  const [lastName, setLastName] = useState("");
  const [email, setEmail] = useState("");
  const [phone, setPhone] = useState("");
  const [cep, setCep] = useState("");
  const [neighborhood, setNeighborhood] = useState("");
  const [city, setCity] = useState("");
  const [street, setStreet] = useState("");
  const [state, setState] = useState("");
  const [complement, setComplement] = useState("");
  const [notes, setNotes] = useState("");

  // Shipping
  const [shippingResult, setShippingResult] = useState(null);
  const [shippingLoading, setShippingLoading] = useState(false);
  const [shippingError, setShippingError] = useState("");

  // Payment
  const [clientSecret, setClientSecret] = useState("");
  const [orderId, setOrderId] = useState("");
  const [orderNumber, setOrderNumber] = useState("");

  // Discount
  const [discountCode, setDiscountCode] = useState("");

  const shippingPrice = shippingResult ? parseFloat(shippingResult.price) : 0;
  const grandTotal = totalPrice + shippingPrice;

  // Redirect to login if not authenticated
  useEffect(() => {
    if (!authLoading && !isAuthenticated) {
      router.push("/login?redirect=/checkout");
    }
  }, [authLoading, isAuthenticated, router]);

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
        if (result.city && !city) setCity(result.city);
        if (result.state && !state) setState(result.state);
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

  const handleContinueToPayment = async () => {
    setError("");

    // Validate required fields
    if (!firstName || !lastName || !email || !phone || !cep || !city || !street || !state) {
      setError(t("checkout.fillRequired"));
      return;
    }

    if (cartProducts.length === 0) {
      setError(t("checkout.cartEmpty"));
      return;
    }

    if (!stripePromise) {
      setError(t("checkout.paymentNotConfigured"));
      return;
    }

    setLoading(true);

    try {
      // 1. Save address
      const addressRes = await addressesApi.create({
        recipient_name: `${firstName} ${lastName}`,
        phone,
        street_address: street,
        street_address_2: complement || null,
        city,
        state,
        postal_code: cep.replace(/\D/g, ""),
        country: "BR",
      });
      const addressId = addressRes.data?.data?.id || addressRes.data?.id;

      if (!addressId) {
        throw new Error("Failed to save address");
      }

      // 2. Sync cart to server before order creation
      if (cartProducts.length > 0) {
        await Promise.all(
          cartProducts.map((item) =>
            cartApi.add(item.id, item.quantity || 1).catch(() => null)
          )
        );
      }

      // 3. Create order
      const orderRes = await userApi.post("/api/orders", {
        shipping_address_id: addressId,
        payment_method: "card",
        discount_code: discountCode || null,
        notes: notes || null,
      });

      const orderData = orderRes.data?.data || orderRes.data;
      const newOrderId = orderData.id;
      const newOrderNumber = orderData.order_number;

      if (!newOrderId) {
        throw new Error("Failed to create order");
      }

      setOrderId(newOrderId);
      setOrderNumber(newOrderNumber);

      // Clear frontend cart (backend already cleared server cart)
      setCartProducts([]);

      // 4. Create payment intent
      const paymentRes = await userApi.post("/api/payments/create-intent", {
        order_id: newOrderId,
      });

      const paymentData = paymentRes.data?.data || paymentRes.data;
      const secret = paymentData.client_secret;

      if (!secret) {
        throw new Error("Failed to create payment");
      }

      setClientSecret(secret);
      setStep("payment");
    } catch (err) {
      const msg =
        err.response?.data?.error ||
        err.message ||
        t("checkout.orderFailed");
      setError(typeof msg === "string" ? msg : t("checkout.orderFailed"));
    } finally {
      setLoading(false);
    }
  };

  if (authLoading) {
    return (
      <section className="flat-spacing">
        <div className="container text-center py-5">
          <div className="spinner-border" role="status">
            <span className="visually-hidden">{t("common.loading")}</span>
          </div>
        </div>
      </section>
    );
  }

  if (!isAuthenticated) {
    return null;
  }

  if (step === "payment" && clientSecret && stripePromise) {
    return (
      <section>
        <div className="container">
          <div className="row">
            <div className="col-xl-6">
              <div className="flat-spacing tf-page-checkout">
                <div className="wrap">
                  <h5 className="title">{t("checkout.confirmPayment")}</h5>
                  <p className="text-secondary mb_20">
                    {t("checkout.orderCreated")} <strong>{orderNumber}</strong>
                  </p>
                  <Elements
                    stripe={stripePromise}
                    options={{
                      clientSecret,
                      appearance: { theme: "stripe" },
                    }}
                  >
                    <PaymentForm
                      orderId={orderId}
                      orderNumber={orderNumber}
                    />
                  </Elements>
                </div>
              </div>
            </div>
            <div className="col-xl-1">
              <div className="line-separation" />
            </div>
            <div className="col-xl-5">
              <div className="flat-spacing flat-sidebar-checkout">
                <div className="sidebar-checkout-content">
                  <h5 className="title">{t("checkout.orderSummary")}</h5>
                  <div className="sec-total-price">
                    <div className="bottom">
                      <h5 className="d-flex justify-content-between">
                        <span>{t("checkout.total")}</span>
                        <span className="total-price-checkout">
                          R$ {grandTotal.toFixed(2)}
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

  return (
    <section>
      <div className="container">
        <div className="row">
          <div className="col-xl-6">
            <div className="flat-spacing tf-page-checkout">
              {!isAuthenticated && (
                <div className="wrap">
                  <div className="title-login">
                    <p>{t("checkout.alreadyHaveAccount")}</p>{" "}
                    <Link href="/login" className="text-button">
                      {t("checkout.loginHere")}
                    </Link>
                  </div>
                </div>
              )}
              <div className="wrap">
                <h5 className="title">{t("checkout.information")}</h5>
                <div className="info-box">
                  <div className="grid-2">
                    <input
                      type="text"
                      placeholder={t("checkout.firstName")}
                      value={firstName}
                      onChange={(e) => setFirstName(e.target.value)}
                      required
                    />
                    <input
                      type="text"
                      placeholder={t("checkout.lastName")}
                      value={lastName}
                      onChange={(e) => setLastName(e.target.value)}
                      required
                    />
                  </div>
                  <div className="grid-2">
                    <input
                      type="email"
                      placeholder={t("checkout.emailAddress")}
                      value={email}
                      onChange={(e) => setEmail(e.target.value)}
                      required
                    />
                    <input
                      type="text"
                      placeholder={t("checkout.phoneNumber")}
                      value={phone}
                      onChange={(e) => setPhone(e.target.value)}
                      required
                    />
                  </div>
                  <div className="grid-2">
                    <div>
                      <input
                        type="text"
                        placeholder={t("checkout.cepPlaceholder")}
                        value={cep}
                        onChange={handleCepChange}
                        required
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
                          {shippingResult.city}, {shippingResult.state} - R${" "}
                          {parseFloat(shippingResult.price).toFixed(2)} (
                          {shippingResult.estimated_days_min}-{shippingResult.estimated_days_max}{" "}
                          {t("checkout.days")})
                        </p>
                      )}
                    </div>
                    <input
                      type="text"
                      placeholder={t("checkout.neighborhood")}
                      value={neighborhood}
                      onChange={(e) => setNeighborhood(e.target.value)}
                    />
                  </div>
                  <div className="grid-2">
                    <input
                      type="text"
                      placeholder={t("checkout.townCity")}
                      value={city}
                      onChange={(e) => setCity(e.target.value)}
                      required
                    />
                    <input
                      type="text"
                      placeholder={t("checkout.streetNumber")}
                      value={street}
                      onChange={(e) => setStreet(e.target.value)}
                      required
                    />
                  </div>
                  <div className="grid-2">
                    <div className="tf-select">
                      <select
                        className="text-title"
                        value={state}
                        onChange={(e) => setState(e.target.value)}
                        required
                      >
                        <option value="">{t("checkout.state")}</option>
                        {["AC","AL","AP","AM","BA","CE","DF","ES","GO","MA","MT","MS","MG",
                          "PA","PB","PR","PE","PI","RJ","RN","RS","RO","RR","SC","SP","SE","TO",
                        ].map((uf) => (
                          <option key={uf} value={uf}>{uf}</option>
                        ))}
                      </select>
                    </div>
                    <input
                      type="text"
                      placeholder={t("checkout.complement")}
                      value={complement}
                      onChange={(e) => setComplement(e.target.value)}
                    />
                  </div>
                  <textarea
                    placeholder={t("checkout.writeNote")}
                    value={notes}
                    onChange={(e) => setNotes(e.target.value)}
                  />
                </div>
              </div>

              <div className="wrap">
                <div className="ip-discount-code mb_20">
                  <input
                    type="text"
                    placeholder={t("checkout.addVoucherDiscount")}
                    value={discountCode}
                    onChange={(e) => setDiscountCode(e.target.value)}
                  />
                </div>

                {error && (
                  <p style={{ color: "#dc3545", marginBottom: 12 }}>{error}</p>
                )}

                <button
                  className="tf-btn btn-fill w-100 justify-content-center"
                  onClick={handleContinueToPayment}
                  disabled={loading || cartProducts.length === 0}
                >
                  <span className="text text-button">
                    {loading
                      ? t("checkout.creatingOrder")
                      : t("checkout.continueToPayment")}
                  </span>
                </button>
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
                  {cartProducts.length === 0 ? (
                    <p className="text-secondary">{t("checkout.cartEmpty")}</p>
                  ) : (
                    cartProducts.map((elm, i) => (
                      <div key={i} className="item-product">
                        <Link
                          href={`/product-detail/${elm.id}`}
                          className="img-product"
                        >
                          <Image
                            alt={elm.title || "product"}
                            src={elm.imgSrc || "/images/products/placeholder.jpg"}
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
                            {(elm.color || elm.size) && (
                              <div className="variant text-caption-1 text-secondary">
                                {elm.size && <span className="size">{elm.size}</span>}
                                {elm.size && elm.color && "/"}
                                {elm.color && <span className="color">{elm.color}</span>}
                              </div>
                            )}
                          </div>
                          <div className="total-price text-button">
                            <span className="count">{elm.quantity}</span>X
                            <span className="price">R$ {elm.price.toFixed(2)}</span>
                          </div>
                        </div>
                      </div>
                    ))
                  )}
                </div>
                <div className="sec-total-price">
                  <div className="top">
                    <div className="item d-flex align-items-center justify-content-between text-button">
                      <span>{t("checkout.subtotal")}</span>
                      <span>R$ {totalPrice.toFixed(2)}</span>
                    </div>
                    <div className="item d-flex align-items-center justify-content-between text-button">
                      <span>{t("checkout.shipping")}</span>
                      <span>
                        {shippingResult
                          ? `R$ ${shippingPrice.toFixed(2)}`
                          : t("checkout.enterCep")}
                      </span>
                    </div>
                  </div>
                  <div className="bottom">
                    <h5 className="d-flex justify-content-between">
                      <span>{t("checkout.total")}</span>
                      <span className="total-price-checkout">
                        R$ {grandTotal.toFixed(2)}
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
