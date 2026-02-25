"use client";
import React, { useState, useEffect, useCallback } from "react";
import { useLanguage } from "@/lib/i18n/LanguageContext";
import { addressesApi } from "@/lib/userApi";
import { lookupCep, formatCep } from "@/lib/cepLookup";
import { maskPhone, unmaskPhone } from "@/lib/phoneMask";
import { useUserAuth } from "@/context/UserAuthContext";
import toast from "react-hot-toast";

const emptyForm = {
  recipient_name: "",
  phone: "",
  postal_code: "",
  street_address: "",
  number: "",
  street_address_2: "",
  neighborhood: "",
  city: "",
  state: "",
  label: "",
  is_default: false,
};

export default function Address() {
  const { t } = useLanguage();
  const { profile } = useUserAuth();
  const [addresses, setAddresses] = useState([]);
  const [isLoading, setIsLoading] = useState(true);
  const [showCreateForm, setShowCreateForm] = useState(false);
  const [editingId, setEditingId] = useState(null);
  const [form, setForm] = useState({ ...emptyForm });
  const [cepLoading, setCepLoading] = useState(false);

  const fetchAddresses = useCallback(async () => {
    try {
      const response = await addressesApi.list();
      const data = response.data?.data || response.data?.addresses || response.data || [];
      setAddresses(Array.isArray(data) ? data : []);
    } catch {
      setAddresses([]);
    }
    setIsLoading(false);
  }, []);

  useEffect(() => {
    fetchAddresses();
  }, [fetchAddresses]);

  const resetForm = () => {
    setForm({
      ...emptyForm,
      recipient_name: profile?.full_name || "",
      phone: profile?.phone ? maskPhone(profile.phone) : "",
    });
  };

  const handleShowCreate = () => {
    resetForm();
    setEditingId(null);
    setShowCreateForm(true);
  };

  const handleEdit = (address) => {
    setShowCreateForm(false);
    setEditingId(address.id);
    // Parse street_address into street + number if possible
    const parts = (address.street_address || "").split(", ");
    setForm({
      recipient_name: address.recipient_name || "",
      phone: address.phone ? maskPhone(address.phone) : "",
      postal_code: address.postal_code ? formatCep(address.postal_code) : "",
      street_address: parts[0] || "",
      number: parts[1] || "",
      street_address_2: address.street_address_2 || "",
      neighborhood: address.neighborhood || "",
      city: address.city || "",
      state: address.state || "",
      label: address.label || "",
      is_default: address.is_default || false,
    });
  };

  const handleCepChange = async (value) => {
    const formatted = formatCep(value);
    setForm((prev) => ({ ...prev, postal_code: formatted }));
    const digits = value.replace(/\D/g, "");
    if (digits.length === 8) {
      setCepLoading(true);
      const result = await lookupCep(digits);
      setCepLoading(false);
      if (result) {
        setForm((prev) => ({
          ...prev,
          street_address: result.street,
          neighborhood: result.neighborhood,
          city: result.city,
          state: result.state,
        }));
      }
    }
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    const payload = {
      recipient_name: form.recipient_name,
      phone: unmaskPhone(form.phone),
      street_address: form.number ? `${form.street_address}, ${form.number}` : form.street_address,
      street_address_2: form.street_address_2,
      city: form.city,
      state: form.state,
      postal_code: form.postal_code.replace(/\D/g, ""),
      country: "BR",
      label: form.label || t("auth.home"),
      is_default: form.is_default,
    };

    try {
      if (editingId) {
        await addressesApi.update(editingId, payload);
        toast.success(t("myAccount.editAddress"));
      } else {
        await addressesApi.create(payload);
        toast.success(t("myAccount.addAddress"));
      }
      setShowCreateForm(false);
      setEditingId(null);
      fetchAddresses();
    } catch (err) {
      toast.error(err.response?.data?.error || "Error");
    }
  };

  const handleDelete = async (id) => {
    try {
      await addressesApi.delete(id);
      fetchAddresses();
    } catch (err) {
      toast.error(err.response?.data?.error || "Error");
    }
  };

  const handleSetDefault = async (id) => {
    try {
      await addressesApi.setDefault(id);
      fetchAddresses();
    } catch (err) {
      toast.error(err.response?.data?.error || "Error");
    }
  };

  const renderForm = () => (
    <form
      className="show-form-address wd-form-address d-block"
      onSubmit={handleSubmit}
    >
      <div className="title">{editingId ? t("myAccount.editAddress") : t("myAccount.addNewAddress")}</div>
      <div className="cols mb_20">
        <fieldset>
          <input
            type="text"
            placeholder={t("auth.fullName")}
            value={form.recipient_name}
            onChange={(e) => setForm({ ...form, recipient_name: e.target.value })}
            required
          />
        </fieldset>
        <fieldset>
          <input
            type="text"
            placeholder={t("myAccount.phone")}
            value={form.phone}
            onChange={(e) => setForm({ ...form, phone: maskPhone(e.target.value) })}
          />
        </fieldset>
      </div>
      <fieldset className="mb_20">
        <input
          type="text"
          placeholder={t("auth.cepPlaceholder")}
          value={form.postal_code}
          onChange={(e) => handleCepChange(e.target.value)}
          required
        />
        {cepLoading && <small className="text-secondary">{t("auth.lookingUpCep")}</small>}
      </fieldset>
      <div className="cols mb_20">
        <fieldset>
          <input
            type="text"
            placeholder={t("auth.street")}
            value={form.street_address}
            onChange={(e) => setForm({ ...form, street_address: e.target.value })}
            required
          />
        </fieldset>
        <fieldset style={{ maxWidth: 120 }}>
          <input
            type="text"
            placeholder={t("auth.number")}
            value={form.number}
            onChange={(e) => setForm({ ...form, number: e.target.value })}
            required
          />
        </fieldset>
      </div>
      <fieldset className="mb_20">
        <input
          type="text"
          placeholder={t("auth.complement")}
          value={form.street_address_2}
          onChange={(e) => setForm({ ...form, street_address_2: e.target.value })}
        />
      </fieldset>
      <div className="cols mb_20">
        <fieldset>
          <input
            type="text"
            placeholder={t("auth.neighborhood")}
            value={form.neighborhood}
            onChange={(e) => setForm({ ...form, neighborhood: e.target.value })}
          />
        </fieldset>
        <fieldset>
          <input
            type="text"
            placeholder={t("auth.city")}
            value={form.city}
            onChange={(e) => setForm({ ...form, city: e.target.value })}
            required
          />
        </fieldset>
      </div>
      <div className="cols mb_20">
        <fieldset>
          <input
            type="text"
            placeholder={t("auth.state")}
            value={form.state}
            onChange={(e) => setForm({ ...form, state: e.target.value })}
            required
            maxLength={2}
          />
        </fieldset>
        <fieldset>
          <input
            type="text"
            placeholder="Label (ex: Casa, Trabalho)"
            value={form.label}
            onChange={(e) => setForm({ ...form, label: e.target.value })}
          />
        </fieldset>
      </div>
      <div className="tf-cart-checkbox mb_20">
        <div className="tf-checkbox-wrapp">
          <input
            type="checkbox"
            id="address-default"
            checked={form.is_default}
            onChange={(e) => setForm({ ...form, is_default: e.target.checked })}
          />
          <div>
            <i className="icon-check" />
          </div>
        </div>
        <label htmlFor="address-default">
          {t("myAccount.setDefault")}
        </label>
      </div>
      <div className="d-flex align-items-center justify-content-center gap-20">
        <button type="submit" className="tf-btn btn-fill radius-4">
          <span className="text">{editingId ? t("myAccount.saveChanges") : t("myAccount.addAddress")}</span>
        </button>
        <span
          className="tf-btn btn-fill radius-4"
          style={{ cursor: "pointer" }}
          onClick={() => {
            setShowCreateForm(false);
            setEditingId(null);
          }}
        >
          <span className="text">{t("myAccount.cancel")}</span>
        </span>
      </div>
    </form>
  );

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
      <div className="account-address">
        <div className="text-center widget-inner-address">
          <button
            className="tf-btn btn-fill radius-4 mb_20 btn-address"
            onClick={handleShowCreate}
          >
            <span className="text text-caption-1">{t("myAccount.addNewAddress")}</span>
          </button>

          {showCreateForm && renderForm()}

          <div className="list-account-address">
            {addresses.length === 0 && !showCreateForm && (
              <p className="text-secondary" style={{ padding: 20 }}>
                {t("myAccount.addNewAddress")}
              </p>
            )}
            {addresses.map((address) => (
              <div className="account-address-item" key={address.id}>
                <h6 className="mb_20">
                  {address.label || t("myAccount.address")}
                  {address.is_default && (
                    <span className="badge ms-2" style={{ fontSize: 11, background: "var(--primary)", color: "#fff", padding: "2px 8px", borderRadius: 4 }}>
                      {t("myAccount.defaultAddress")}
                    </span>
                  )}
                </h6>
                <p>{address.recipient_name}</p>
                <p>{address.street_address}</p>
                {address.street_address_2 && <p>{address.street_address_2}</p>}
                <p>{address.city}, {address.state} - {address.postal_code ? formatCep(address.postal_code) : ""}</p>
                {address.phone && <p className="mb_10">{maskPhone(address.phone)}</p>}
                <div className="d-flex gap-10 justify-content-center flex-wrap">
                  <button
                    className="tf-btn radius-4 btn-fill justify-content-center btn-edit-address"
                    onClick={() => handleEdit(address)}
                  >
                    <span className="text">{t("myAccount.edit")}</span>
                  </button>
                  <button
                    className="tf-btn radius-4 btn-outline justify-content-center btn-delete-address"
                    onClick={() => handleDelete(address.id)}
                  >
                    <span className="text">{t("myAccount.delete")}</span>
                  </button>
                  {!address.is_default && (
                    <button
                      className="tf-btn radius-4 btn-outline justify-content-center"
                      onClick={() => handleSetDefault(address.id)}
                    >
                      <span className="text">{t("myAccount.setDefault")}</span>
                    </button>
                  )}
                </div>
                {editingId === address.id && renderForm()}
              </div>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}
