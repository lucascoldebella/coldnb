"use client";
import React, { useState, useEffect } from "react";
import { useRouter } from "next/navigation";
import { useUserAuth } from "@/context/UserAuthContext";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { addressesApi } from "@/lib/userApi";
import { lookupCep, formatCep } from "@/lib/cepLookup";
import { maskPhone, unmaskPhone } from "@/lib/phoneMask";
import SocialButtons from "./SocialButtons";

export default function AuthFlow({
  initialMode = "login",
  embedded = false,
  onComplete,
  embeddedTitle,
}) {
  const { signIn, signUp, updateProfile, isAuthenticated, isLoading: authLoading } = useUserAuth();
  const { t } = useLanguage();
  const router = useRouter();

  const [step, setStep] = useState("initial");
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState("");
  const [fadeClass, setFadeClass] = useState("auth-step-enter");

  // Login fields
  const [loginEmail, setLoginEmail] = useState("");
  const [loginPassword, setLoginPassword] = useState("");
  const [showLoginPassword, setShowLoginPassword] = useState(false);

  // Register fields
  const [fullName, setFullName] = useState("");
  const [regEmail, setRegEmail] = useState("");
  const [phone, setPhone] = useState("");
  const [regPassword, setRegPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [showRegPassword, setShowRegPassword] = useState(false);
  const [showConfirmPassword, setShowConfirmPassword] = useState(false);

  // Address fields
  const [cep, setCep] = useState("");
  const [cepLoading, setCepLoading] = useState(false);
  const [street, setStreet] = useState("");
  const [number, setNumber] = useState("");
  const [complement, setComplement] = useState("");
  const [neighborhood, setNeighborhood] = useState("");
  const [city, setCity] = useState("");
  const [state, setState] = useState("");

  // Redirect if already authenticated (only on standalone page, not embedded)
  useEffect(() => {
    if (!embedded && !authLoading && isAuthenticated && step !== "register-5") {
      router.push("/my-account");
    }
  }, [isAuthenticated, authLoading, router, step, embedded]);

  const changeStep = (newStep) => {
    setFadeClass("auth-step-exit");
    setError("");
    setTimeout(() => {
      setStep(newStep);
      setFadeClass("auth-step-enter");
    }, 200);
  };

  const handleDone = () => {
    if (embedded && onComplete) {
      onComplete();
    } else {
      router.push("/my-account");
    }
  };

  // CEP auto-fill
  const handleCepChange = async (value) => {
    const formatted = formatCep(value);
    setCep(formatted);
    const digits = value.replace(/\D/g, "");
    if (digits.length === 8) {
      setCepLoading(true);
      const result = await lookupCep(digits);
      setCepLoading(false);
      if (result) {
        setStreet(result.street);
        setNeighborhood(result.neighborhood);
        setCity(result.city);
        setState(result.state);
      }
    }
  };

  // Login handler
  const handleLogin = async (e) => {
    e.preventDefault();
    setIsLoading(true);
    setError("");

    const { error: signInError } = await signIn(loginEmail, loginPassword);
    setIsLoading(false);

    if (signInError) {
      setError(t("login.loginError"));
    } else if (embedded && onComplete) {
      onComplete();
    }
    // standalone redirect handled by useEffect
  };

  // Register handler (step 4 → create account)
  const handleCreateAccount = async (e) => {
    e.preventDefault();
    if (regPassword !== confirmPassword) {
      setError(t("register.passwordMismatch"));
      return;
    }
    if (regPassword.length < 6) {
      setError(t("auth.passwordTooShort"));
      return;
    }

    setIsLoading(true);
    setError("");

    const { error: signUpError } = await signUp(regEmail, regPassword);
    if (signUpError) {
      setIsLoading(false);
      setError(signUpError.message);
      return;
    }

    // Sign in immediately after sign up
    const { error: signInError } = await signIn(regEmail, regPassword);
    if (signInError) {
      setIsLoading(false);
      setError(signInError.message);
      return;
    }

    // Update profile with name and phone
    try {
      await updateProfile({
        full_name: fullName,
        phone: unmaskPhone(phone),
      });
    } catch {
      // Non-blocking: profile will be updated later
    }

    setIsLoading(false);
    changeStep("register-5");
  };

  // Save address handler (step 5)
  const handleSaveAddress = async (e) => {
    e.preventDefault();
    setIsLoading(true);
    setError("");

    try {
      await addressesApi.create({
        recipient_name: fullName,
        phone: unmaskPhone(phone),
        street_address: `${street}, ${number}`,
        street_address_2: complement,
        city,
        state,
        postal_code: cep.replace(/\D/g, ""),
        country: "BR",
        label: t("auth.home"),
        is_default: true,
      });
      handleDone();
    } catch (err) {
      setError(err.response?.data?.error || t("auth.addressSaveError"));
    }
    setIsLoading(false);
  };

  // Get current register step number (1-5) for progress dots
  const getRegisterStep = () => {
    const match = step.match(/register-(\d)/);
    return match ? parseInt(match[1]) : 0;
  };

  const renderProgressDots = () => {
    const current = getRegisterStep();
    if (current < 1) return null;
    return (
      <div className="auth-step-dots">
        {[1, 2, 3, 4, 5].map((n) => (
          <span
            key={n}
            className={`auth-step-dot ${n === current ? "active" : ""} ${n < current ? "completed" : ""}`}
          />
        ))}
      </div>
    );
  };

  // Navigation buttons - back (red, left) + next (green, right)
  const renderNavButtons = ({ backStep, nextLabel, nextType = "submit", onNext, disabled = false }) => (
    <div className="auth-nav-buttons">
      {backStep ? (
        <button
          type="button"
          className="tf-btn btn-auth-back"
          onClick={() => changeStep(backStep)}
        >
          <span className="text">{t("auth.back")}</span>
        </button>
      ) : (
        <span />
      )}
      {nextLabel && (
        <button
          type={nextType}
          className="tf-btn btn-auth-next"
          disabled={disabled}
          onClick={onNext}
        >
          <span className="text">{nextLabel}</span>
        </button>
      )}
    </div>
  );

  if (authLoading) {
    return (
      <section className="flat-spacing">
        <div className="container">
          <div className="login-wrap" style={{ justifyContent: "center" }}>
            <div className="tf-loading" />
          </div>
        </div>
      </section>
    );
  }

  const content = (
    <div className={`auth-flow-card ${fadeClass}`} style={{ maxWidth: 480, width: "100%" }}>
      {renderProgressDots()}

      {error && (
        <div className="alert alert-danger mb_20" style={{ fontSize: 14, padding: "10px 16px" }}>
          {error}
        </div>
      )}

      {/* INITIAL: Choose Login or Register */}
      {step === "initial" && (
        <div className="auth-step">
          <div className="heading text-center mb_20">
            <h4>{embedded && embeddedTitle ? embeddedTitle : t("auth.welcome")}</h4>
            <p className="text-secondary mt_8">{t("auth.chooseMethod")}</p>
          </div>
          <div className="d-flex flex-column gap-12">
            <button
              className="tf-btn btn-fill w-100"
              onClick={() => changeStep("login")}
            >
              <span className="text">{t("auth.loginWithEmail")}</span>
            </button>
            <button
              className="tf-btn btn-outline w-100"
              onClick={() => changeStep("register-1")}
            >
              <span className="text">{t("auth.registerWithEmail")}</span>
            </button>
          </div>
          <div className="auth-divider">
            <span>{t("auth.orContinueWith")}</span>
          </div>
          <SocialButtons />
        </div>
      )}

      {/* LOGIN */}
      {step === "login" && (
        <div className="auth-step">
          <div className="heading text-center mb_20">
            <h4>{t("login.title")}</h4>
          </div>
          <form onSubmit={handleLogin} className="form-login form-has-password">
            <div className="wrap">
              <fieldset>
                <input
                  type="email"
                  placeholder={t("login.email")}
                  value={loginEmail}
                  onChange={(e) => setLoginEmail(e.target.value)}
                  required
                  autoFocus
                />
              </fieldset>
              <fieldset className="position-relative password-item">
                <input
                  className="input-password"
                  type={showLoginPassword ? "text" : "password"}
                  placeholder={t("login.password")}
                  value={loginPassword}
                  onChange={(e) => setLoginPassword(e.target.value)}
                  required
                />
                <span
                  className={`toggle-password ${!showLoginPassword ? "unshow" : ""}`}
                  onClick={() => setShowLoginPassword(!showLoginPassword)}
                >
                  <i className={`icon-eye-${!showLoginPassword ? "hide" : "show"}-line`} />
                </span>
              </fieldset>
            </div>
            {renderNavButtons({
              backStep: "initial",
              nextLabel: isLoading ? t("login.logging") : t("login.login"),
              disabled: isLoading,
            })}
          </form>
          <div className="auth-divider">
            <span>{t("auth.orContinueWith")}</span>
          </div>
          <SocialButtons />
        </div>
      )}

      {/* REGISTER STEP 1: Full Name */}
      {step === "register-1" && (
        <div className="auth-step">
          <div className="heading text-center mb_20">
            <h4>{t("auth.whatsYourName")}</h4>
          </div>
          <form
            onSubmit={(e) => {
              e.preventDefault();
              if (fullName.trim()) changeStep("register-2");
            }}
            className="form-login"
          >
            <div className="wrap">
              <fieldset>
                <input
                  type="text"
                  placeholder={t("auth.fullName")}
                  value={fullName}
                  onChange={(e) => setFullName(e.target.value)}
                  required
                  autoFocus
                />
              </fieldset>
            </div>
            {renderNavButtons({
              backStep: "initial",
              nextLabel: t("auth.next"),
            })}
          </form>
        </div>
      )}

      {/* REGISTER STEP 2: Email */}
      {step === "register-2" && (
        <div className="auth-step">
          <div className="heading text-center mb_20">
            <h4>{t("auth.whatsYourEmail")}</h4>
          </div>
          <form
            onSubmit={(e) => {
              e.preventDefault();
              if (regEmail.trim()) changeStep("register-3");
            }}
            className="form-login"
          >
            <div className="wrap">
              <fieldset>
                <input
                  type="email"
                  placeholder={t("login.email")}
                  value={regEmail}
                  onChange={(e) => setRegEmail(e.target.value)}
                  required
                  autoFocus
                />
              </fieldset>
            </div>
            {renderNavButtons({
              backStep: "register-1",
              nextLabel: t("auth.next"),
            })}
          </form>
        </div>
      )}

      {/* REGISTER STEP 3: Phone */}
      {step === "register-3" && (
        <div className="auth-step">
          <div className="heading text-center mb_20">
            <h4>{t("auth.whatsYourPhone")}</h4>
          </div>
          <form
            onSubmit={(e) => {
              e.preventDefault();
              if (unmaskPhone(phone).length >= 10) changeStep("register-4");
            }}
            className="form-login"
          >
            <div className="wrap">
              <fieldset>
                <input
                  type="tel"
                  placeholder={t("auth.phonePlaceholder")}
                  value={phone}
                  onChange={(e) => setPhone(maskPhone(e.target.value))}
                  required
                  autoFocus
                />
              </fieldset>
            </div>
            {renderNavButtons({
              backStep: "register-2",
              nextLabel: t("auth.next"),
            })}
          </form>
        </div>
      )}

      {/* REGISTER STEP 4: Password */}
      {step === "register-4" && (
        <div className="auth-step">
          <div className="heading text-center mb_20">
            <h4>{t("auth.createPassword")}</h4>
          </div>
          <form onSubmit={handleCreateAccount} className="form-login form-has-password">
            <div className="wrap">
              <fieldset className="position-relative password-item">
                <input
                  className="input-password"
                  type={showRegPassword ? "text" : "password"}
                  placeholder={t("login.password")}
                  value={regPassword}
                  onChange={(e) => setRegPassword(e.target.value)}
                  required
                  minLength={6}
                  autoFocus
                />
                <span
                  className={`toggle-password ${!showRegPassword ? "unshow" : ""}`}
                  onClick={() => setShowRegPassword(!showRegPassword)}
                >
                  <i className={`icon-eye-${!showRegPassword ? "hide" : "show"}-line`} />
                </span>
              </fieldset>
              <fieldset className="position-relative password-item">
                <input
                  className="input-password"
                  type={showConfirmPassword ? "text" : "password"}
                  placeholder={t("register.confirmPassword")}
                  value={confirmPassword}
                  onChange={(e) => setConfirmPassword(e.target.value)}
                  required
                  minLength={6}
                />
                <span
                  className={`toggle-password ${!showConfirmPassword ? "unshow" : ""}`}
                  onClick={() => setShowConfirmPassword(!showConfirmPassword)}
                >
                  <i className={`icon-eye-${!showConfirmPassword ? "hide" : "show"}-line`} />
                </span>
              </fieldset>
            </div>
            {renderNavButtons({
              backStep: "register-3",
              nextLabel: isLoading ? t("register.registering") : t("auth.createAccount"),
              disabled: isLoading,
            })}
          </form>
        </div>
      )}

      {/* REGISTER STEP 5: Delivery Address */}
      {step === "register-5" && (
        <div className="auth-step">
          <div className="heading text-center mb_20">
            <h4>{t("auth.deliveryAddress")}</h4>
            <p className="text-secondary mt_8">{t("auth.addressOptional")}</p>
          </div>
          <form onSubmit={handleSaveAddress} className="form-login">
            <div className="wrap">
              <fieldset>
                <input
                  type="text"
                  placeholder={t("auth.cepPlaceholder")}
                  value={cep}
                  onChange={(e) => handleCepChange(e.target.value)}
                  required
                  autoFocus
                />
                {cepLoading && (
                  <small className="text-secondary">{t("auth.lookingUpCep")}</small>
                )}
              </fieldset>
              <div className="cols">
                <fieldset>
                  <input
                    type="text"
                    placeholder={t("auth.street")}
                    value={street}
                    onChange={(e) => setStreet(e.target.value)}
                    required
                  />
                </fieldset>
                <fieldset style={{ maxWidth: 120 }}>
                  <input
                    type="text"
                    placeholder={t("auth.number")}
                    value={number}
                    onChange={(e) => setNumber(e.target.value)}
                    required
                  />
                </fieldset>
              </div>
              <fieldset>
                <input
                  type="text"
                  placeholder={t("auth.complement")}
                  value={complement}
                  onChange={(e) => setComplement(e.target.value)}
                />
              </fieldset>
              <div className="cols">
                <fieldset>
                  <input
                    type="text"
                    placeholder={t("auth.neighborhood")}
                    value={neighborhood}
                    onChange={(e) => setNeighborhood(e.target.value)}
                    required
                  />
                </fieldset>
                <fieldset>
                  <input
                    type="text"
                    placeholder={t("auth.city")}
                    value={city}
                    onChange={(e) => setCity(e.target.value)}
                    required
                  />
                </fieldset>
              </div>
              <fieldset>
                <input
                  type="text"
                  placeholder={t("auth.state")}
                  value={state}
                  onChange={(e) => setState(e.target.value)}
                  required
                  maxLength={2}
                />
              </fieldset>
            </div>
            <div className="auth-nav-buttons">
              <button
                type="button"
                className="tf-btn btn-auth-back"
                onClick={handleDone}
              >
                <span className="text">{t("auth.skipForNow")}</span>
              </button>
              <button className="tf-btn btn-auth-next" type="submit" disabled={isLoading}>
                <span className="text">{t("auth.saveAddress")}</span>
              </button>
            </div>
          </form>
        </div>
      )}
    </div>
  );

  // When embedded, render without section/container wrapper
  if (embedded) {
    return content;
  }

  return (
    <section className="flat-spacing">
      <div className="container">
        <div className="login-wrap" style={{ justifyContent: "center" }}>
          {content}
        </div>
      </div>
    </section>
  );
}
