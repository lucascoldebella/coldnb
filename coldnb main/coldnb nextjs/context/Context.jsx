"use client";
import { openCartModal } from "@/utlis/openCartModal";
import { openWistlistModal } from "@/utlis/openWishlist";
import supabase from "@/lib/supabase";
import { cartApi } from "@/lib/userApi";

import React, { useEffect, useRef, useCallback } from "react";
import { useContext, useState } from "react";
const dataContext = React.createContext();
export const useContextElement = () => {
  return useContext(dataContext);
};

export default function Context({ children }) {
  const [cartProducts, setCartProducts] = useState([]);
  const [wishList, setWishList] = useState([]);
  const [compareItem, setCompareItem] = useState([]);
  const [quickViewItem, setQuickViewItem] = useState({
    id: 0, title: "", price: 0, imgSrc: "/images/products/placeholder.jpg",
  });
  const [quickAddItem, setQuickAddItem] = useState(null);
  const [totalPrice, setTotalPrice] = useState(0);

  /* Track auth state for server cart sync */
  const sessionRef = useRef(null);
  useEffect(() => {
    if (!supabase) return;
    supabase.auth.getSession().then(({ data: { session } }) => {
      sessionRef.current = session;
    });
    const { data: { subscription } } = supabase.auth.onAuthStateChange((_, session) => {
      sessionRef.current = session;
    });
    return () => subscription.unsubscribe();
  }, []);

  const isAuthenticated = () => !!sessionRef.current;

  useEffect(() => {
    const subtotal = cartProducts.reduce((accumulator, product) => {
      return accumulator + product.quantity * product.price;
    }, 0);
    setTotalPrice(subtotal);
  }, [cartProducts]);

  const isAddedToCartProducts = (id) => {
    if (cartProducts.filter((elm) => elm.id == id)[0]) {
      return true;
    }
    return false;
  };
  // Accepts a product object with id/title/price/imgSrc.
  // All callers must pass the full product object, not just an ID.
  const addProductToCart = (product, qty, isModal = true) => {
    if (!product || typeof product !== "object" || !product.id) return;

    if (!isAddedToCartProducts(product.id)) {
      const quantity = qty ? qty : 1;
      const item = {
        ...product,
        quantity,
      };
      setCartProducts((pre) => [...pre, item]);
      if (isModal) {
        openCartModal();
      }

      /* Sync to server if authenticated */
      if (isAuthenticated()) {
        cartApi.add(product.id, quantity).then((res) => {
          const cartItemId = res.data?.data?.id;
          if (cartItemId) {
            setCartProducts((prev) =>
              prev.map((p) => p.id === product.id && !p.cartItemId
                ? { ...p, cartItemId }
                : p
              )
            );
          }
        }).catch(() => {});
      }
    }
  };

  const updateQuantity = (id, qty) => {
    if (isAddedToCartProducts(id)) {
      let item = cartProducts.filter((elm) => elm.id == id)[0];
      let items = [...cartProducts];
      const itemIndex = items.indexOf(item);

      item.quantity = qty / 1;
      items[itemIndex] = item;
      setCartProducts(items);

      /* Sync to server if authenticated */
      if (isAuthenticated() && item.cartItemId) {
        cartApi.update(item.cartItemId, qty / 1).catch(() => {});
      }
    }
  };

  const removeFromCart = useCallback((id) => {
    const item = cartProducts.find((elm) => elm.id == id);
    setCartProducts((pre) => pre.filter((elm) => elm.id != id));

    /* Sync to server if authenticated */
    if (isAuthenticated() && item?.cartItemId) {
      cartApi.remove(item.cartItemId).catch(() => {});
    }
  }, [cartProducts]);

  const clearCart = useCallback(() => {
    setCartProducts([]);
    if (isAuthenticated()) {
      cartApi.clear().catch(() => {});
    }
  }, []);

  const addToWishlist = (id) => {
    if (!wishList.includes(id)) {
      setWishList((pre) => [...pre, id]);
      openWistlistModal();
    }
  };

  const removeFromWishlist = (id) => {
    if (wishList.includes(id)) {
      setWishList((pre) => [...pre.filter((elm) => elm != id)]);
    }
  };
  const addToCompareItem = (id) => {
    if (!compareItem.includes(id)) {
      setCompareItem((pre) => [...pre, id]);
    }
  };
  const removeFromCompareItem = (id) => {
    if (compareItem.includes(id)) {
      setCompareItem((pre) => [...pre.filter((elm) => elm != id)]);
    }
  };
  const isAddedtoWishlist = (id) => {
    if (wishList.includes(id)) {
      return true;
    }
    return false;
  };
  const isAddedtoCompareItem = (id) => {
    if (compareItem.includes(id)) {
      return true;
    }
    return false;
  };
  useEffect(() => {
    const items = JSON.parse(localStorage.getItem("cartList"));
    if (items?.length) {
      setCartProducts(items);
    }
  }, []);

  useEffect(() => {
    localStorage.setItem("cartList", JSON.stringify(cartProducts));
  }, [cartProducts]);
  useEffect(() => {
    const items = JSON.parse(localStorage.getItem("wishlist"));
    if (items?.length) {
      setWishList(items);
    }
  }, []);

  useEffect(() => {
    localStorage.setItem("wishlist", JSON.stringify(wishList));
  }, [wishList]);

  const contextElement = {
    cartProducts,
    setCartProducts,
    totalPrice,
    addProductToCart,
    isAddedToCartProducts,
    removeFromCart,
    clearCart,
    removeFromWishlist,
    addToWishlist,
    isAddedtoWishlist,
    quickViewItem,
    wishList,
    setQuickViewItem,
    quickAddItem,
    setQuickAddItem,
    addToCompareItem,
    isAddedtoCompareItem,
    removeFromCompareItem,
    compareItem,
    setCompareItem,
    updateQuantity,
  };
  return (
    <dataContext.Provider value={contextElement}>
      {children}
    </dataContext.Provider>
  );
}
