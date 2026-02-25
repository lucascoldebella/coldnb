"use client";
import { useContextElement } from "@/context/Context";
import Image from "next/image";
import Link from "next/link";
import { useState } from "react";
import { useRouter } from "next/navigation";
import ColorSelect from "../productDetails/ColorSelect";
import SizeSelect from "../productDetails/SizeSelect";
import QuantitySelect from "../productDetails/QuantitySelect";
import { useLanguage } from "@/lib/i18n/LanguageContext";
export default function QuickAdd() {
  const { t } = useLanguage();
  const [quantity, setQuantity] = useState(1);
  const {
    quickAddItem,
    addProductToCart,
    isAddedToCartProducts,
    addToCompareItem,
    addToWishlist,
    isAddedtoWishlist,
    isAddedtoCompareItem,
    cartProducts,
    updateQuantity,
  } = useContextElement();
  const router = useRouter();

  // quickAddItem is now a full product object (set by ProductCard)
  const item = quickAddItem || { id: 0, title: "", price: 0, imgSrc: "/images/products/placeholder.jpg" };

  if (!quickAddItem) return null;

  return (
    <div className="modal fade modal-quick-add" id="quickAdd">
      <div className="modal-dialog modal-dialog-centered">
        <div className="modal-content">
          <div className="header">
            <span
              className="icon-close icon-close-popup"
              data-bs-dismiss="modal"
            />
          </div>
          <div>
            <div className="tf-product-info-list">
              <div className="tf-product-info-item">
                <div className="image">
                  <Image alt="" src={item.imgSrc} width={600} height={800} />
                </div>
                <div className="content">
                  <Link href={`/product-detail/${item.id}`}>{item.title}</Link>
                  <div className="tf-product-info-price">
                    <h5 className="price-on-sale font-2">
                      R${item.price.toFixed(2)}
                    </h5>
                    {item.oldPrice && item.oldPrice > item.price ? (
                      <>
                        <div className="compare-at-price font-2">
                          R${item.oldPrice.toFixed(2)}
                        </div>
                        <div className="badges-on-sale text-btn-uppercase">
                          -{Math.round(((item.oldPrice - item.price) / item.oldPrice) * 100)}%
                        </div>
                      </>
                    ) : (
                      ""
                    )}
                  </div>
                </div>
              </div>
              <div className="tf-product-info-choose-option">
                <ColorSelect />
                <SizeSelect />
                <div className="tf-product-info-quantity">
                  <div className="title mb_12">{t("product.quantity")}</div>
                  <QuantitySelect
                    quantity={
                      isAddedToCartProducts(item.id)
                        ? cartProducts.filter((elm) => elm.id == item.id)[0]
                            .quantity
                        : quantity
                    }
                    setQuantity={(qty) => {
                      if (isAddedToCartProducts(item.id)) {
                        updateQuantity(item.id, qty);
                      } else {
                        setQuantity(qty);
                      }
                    }}
                  />
                </div>
                <div>
                  <div className="tf-product-info-by-btn mb_10">
                    <a
                      className="btn-style-2 flex-grow-1 text-btn-uppercase fw-6 show-shopping-cart"
                      onClick={() => addProductToCart(item, quantity)}
                    >
                      <span>
                        {isAddedToCartProducts(item.id)
                          ? t("product.alreadyAdded")
                          : t("product.addToCartDash")}
                        &nbsp;
                      </span>
                      <span className="tf-qty-price total-price">
                        R$
                        {isAddedToCartProducts(item.id)
                          ? (
                              item.price *
                              cartProducts.filter((elm) => elm.id == item.id)[0]
                                .quantity
                            ).toFixed(2)
                          : (item.price * quantity).toFixed(2)}
                      </span>
                    </a>
                    <a
                      href="#compare"
                      onClick={() => addToCompareItem(item.id)}
                      data-bs-toggle="offcanvas"
                      aria-controls="compare"
                      className="box-icon hover-tooltip compare btn-icon-action show-compare"
                    >
                      <span className="icon icon-gitDiff" />
                      <span className="tooltip text-caption-2">
                        {" "}
                        {isAddedtoCompareItem(item.id)
                          ? t("product.alreadyCompared")
                          : t("product.compare")}
                      </span>
                    </a>
                    <a
                      onClick={() => addToWishlist(item.id)}
                      className="box-icon hover-tooltip text-caption-2 wishlist btn-icon-action"
                    >
                      <span className="icon icon-heart" />
                      <span className="tooltip text-caption-2">
                        {isAddedtoWishlist(item.id)
                          ? t("product.alreadyWishlisted")
                          : t("product.wishlist")}
                      </span>
                    </a>
                  </div>
                  <a
                    onClick={() => {
                      addProductToCart(item, quantity);
                      router.push("/checkout");
                    }}
                    className="btn-style-3 text-btn-uppercase"
                    style={{ cursor: "pointer" }}
                  >
                    {t("product.buyItNow")}
                  </a>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
