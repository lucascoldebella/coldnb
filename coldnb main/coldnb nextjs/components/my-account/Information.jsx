"use client";
import React, { useState, useEffect } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { useUserAuth } from "@/context/UserAuthContext";
import { maskPhone, unmaskPhone } from "@/lib/phoneMask";
import toast from "react-hot-toast";

export default function Information() {
  const { t } = useLanguage();
  const { profile, updateProfile, resetPassword, user } = useUserAuth();

  const [fullName, setFullName] = useState("");
  const [email, setEmail] = useState("");
  const [phone, setPhone] = useState("");
  const [isLoading, setIsLoading] = useState(false);

  // Populate form when profile loads
  useEffect(() => {
    if (profile) {
      setFullName(profile.full_name || "");
      setEmail(profile.email || "");
      setPhone(profile.phone ? maskPhone(profile.phone) : "");
    }
  }, [profile]);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setIsLoading(true);

    try {
      await updateProfile({
        full_name: fullName,
        phone: unmaskPhone(phone),
      });
      toast.success(t("auth.profileUpdated"));
    } catch {
      toast.error(t("auth.profileUpdateError"));
    }
    setIsLoading(false);
  };

  const handleResetPassword = async () => {
    if (!user?.email) return;
    const { error } = await resetPassword(user.email);
    if (error) {
      toast.error(error.message);
    } else {
      toast.success(t("auth.resetPasswordSent"));
    }
  };

  return (
    <div className="account-details">
      <form
        onSubmit={handleSubmit}
        className="form-account-details form-has-password"
      >
        <div className="account-info">
          <h5 className="title">{t("myAccount.information")}</h5>
          <div className="cols mb_20">
            <fieldset>
              <input
                type="text"
                placeholder={t("auth.fullName")}
                value={fullName}
                onChange={(e) => setFullName(e.target.value)}
                required
              />
            </fieldset>
          </div>
          <div className="cols mb_20">
            <fieldset>
              <input
                type="email"
                placeholder={t("myAccount.email")}
                value={email}
                disabled
                style={{ opacity: 0.6, cursor: "not-allowed" }}
              />
            </fieldset>
            <fieldset>
              <input
                type="text"
                placeholder={t("myAccount.phone")}
                value={phone}
                onChange={(e) => setPhone(maskPhone(e.target.value))}
              />
            </fieldset>
          </div>
        </div>
        <div className="account-password">
          <h5 className="title">{t("myAccount.changePassword")}</h5>
          <p className="text-secondary mb_20">
            {t("auth.resetPasswordSent").replace("!", ".")}
          </p>
          <button
            type="button"
            className="tf-btn btn-outline radius-4"
            onClick={handleResetPassword}
          >
            <span className="text">{t("auth.resetPassword")}</span>
          </button>
        </div>
        <div className="button-submit">
          <button className="tf-btn btn-fill" type="submit" disabled={isLoading}>
            <span className="text text-button">
              {isLoading ? "..." : t("myAccount.updateAccount")}
            </span>
          </button>
        </div>
      </form>
    </div>
  );
}
