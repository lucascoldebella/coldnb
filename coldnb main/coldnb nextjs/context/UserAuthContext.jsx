"use client";
import React, { createContext, useContext, useState, useEffect, useCallback, useRef } from "react";
import { useRouter, usePathname } from "next/navigation";
import supabase from "@/lib/supabase";
import { profileApi, cartApi } from "@/lib/userApi";
import { sanitizeNextPath } from "@/lib/authRedirect";
import { useContextElement } from "./Context";

const UserAuthContext = createContext();

export const useUserAuth = () => {
  const context = useContext(UserAuthContext);
  if (!context) {
    throw new Error("useUserAuth must be used within UserAuthProvider");
  }
  return context;
};

export function UserAuthProvider({ children }) {
  const [user, setUser] = useState(null);
  const [session, setSession] = useState(null);
  const [profile, setProfile] = useState(null);
  const [isLoading, setIsLoading] = useState(true);
  const isSigningOut = useRef(false);
  const { setCartProducts } = useContextElement();
  const router = useRouter();
  const pathname = usePathname();

  const isAuthenticated = !!session;
  const isProfileComplete = !!(profile?.full_name && profile?.phone);

  const getAuthRedirectUrl = useCallback((nextPath = "/my-account", flow = "") => {
    if (typeof window === "undefined") return undefined;
    const redirectUrl = new URL("/auth/callback", window.location.origin);
    redirectUrl.searchParams.set("next", sanitizeNextPath(nextPath));
    if (flow) {
      redirectUrl.searchParams.set("flow", flow);
    }
    return redirectUrl.toString();
  }, []);

  // Fetch user profile from backend (auto-creates if not found)
  const fetchProfile = useCallback(async () => {
    try {
      const response = await profileApi.get();
      const data = response.data?.data || response.data;
      setProfile(data);
      return data;
    } catch (error) {
      console.error("Failed to fetch profile:", error);
      return null;
    }
  }, []);

  // Merge localStorage cart into server cart, then load server cart into Context
  const mergeCartOnLogin = useCallback(async () => {
    try {
      // Push localStorage items to server
      const localCart = JSON.parse(localStorage.getItem("cartList") || "[]");
      if (localCart.length > 0) {
        await Promise.all(
          localCart.map((item) =>
            cartApi.add(item.id, item.quantity || 1).catch(() => null)
          )
        );
      }

      // Fetch merged server cart
      const res = await cartApi.get();
      const serverItems = res.data?.data?.items || [];

      // Transform server items to frontend shape
      const merged = serverItems.map((item) => ({
        id: item.product_id,
        title: item.product_name,
        price: parseFloat(item.price) || 0,
        imgSrc: item.image_url || "/images/products/placeholder.jpg",
        quantity: item.quantity || 1,
        slug: item.product_slug,
        compareAtPrice: item.compare_at_price ? parseFloat(item.compare_at_price) : null,
        color: item.color_name || null,
        size: item.size_name || null,
      }));

      setCartProducts(merged);
    } catch (error) {
      console.error("Cart merge failed:", error);
    }
  }, [setCartProducts]);

  // Update user profile
  const updateProfile = useCallback(async (data) => {
    const response = await profileApi.update(data);
    const updated = response.data?.data || response.data;
    setProfile(updated);
    return updated;
  }, []);

  // Sign up with email
  const signUp = useCallback(async (email, password, options = {}) => {
    if (!supabase) return { error: { message: "Auth not configured" } };
    const { data, error } = await supabase.auth.signUp({
      email,
      password,
      options,
    });
    return { data, error };
  }, []);

  // Sign in with email
  const signIn = useCallback(async (email, password) => {
    if (!supabase) return { error: { message: "Auth not configured" } };
    const { data, error } = await supabase.auth.signInWithPassword({ email, password });
    return { data, error };
  }, []);

  // Sign in with OAuth
  const signInWithOAuth = useCallback(async (provider, nextPath = "/complete-profile") => {
    if (!supabase) return { error: { message: "Auth not configured" } };
    const { data, error } = await supabase.auth.signInWithOAuth({
      provider,
      options: {
        redirectTo: getAuthRedirectUrl(nextPath, "oauth"),
        queryParams: provider === "google" ? { prompt: "select_account" } : undefined,
      },
    });
    return { data, error };
  }, [getAuthRedirectUrl]);

  // Sign out
  const signOut = useCallback(async () => {
    if (isSigningOut.current) return;
    isSigningOut.current = true;

    try {
      if (supabase) {
        await supabase.auth.signOut();
      }
    } finally {
      setUser(null);
      setSession(null);
      setProfile(null);

      if (pathname !== "/login") {
        router.push("/login");
      }

      setTimeout(() => {
        isSigningOut.current = false;
      }, 1000);
    }
  }, [router, pathname]);

  // Reset password
  const resetPassword = useCallback(async (email) => {
    if (!supabase) return { error: { message: "Auth not configured" } };
    const { data, error } = await supabase.auth.resetPasswordForEmail(email, {
      redirectTo: getAuthRedirectUrl("/my-account", "recovery"),
    });
    return { data, error };
  }, [getAuthRedirectUrl]);

  const deleteAccount = useCallback(async () => {
    await profileApi.deleteAccount();

    try {
      if (supabase) {
        await supabase.auth.signOut();
      }
    } finally {
      setUser(null);
      setSession(null);
      setProfile(null);
      router.push("/login");
    }
  }, [router]);

  // Listen to auth state changes
  useEffect(() => {
    if (!supabase) {
      setIsLoading(false);
      return;
    }

    // Get initial session
    supabase.auth.getSession().then(({ data: { session: initialSession } }) => {
      setSession(initialSession);
      setUser(initialSession?.user ?? null);
      if (initialSession) {
        fetchProfile();
      }
      setIsLoading(false);
    });

    // Listen for changes
    const { data: { subscription } } = supabase.auth.onAuthStateChange(
      async (event, newSession) => {
        setSession(newSession);
        setUser(newSession?.user ?? null);

        if (event === "SIGNED_IN" && newSession) {
          await fetchProfile();
          await mergeCartOnLogin();
        }

        if (event === "SIGNED_OUT") {
          setProfile(null);
        }
      }
    );

    return () => subscription.unsubscribe();
  }, [fetchProfile, mergeCartOnLogin]);

  const value = {
    user,
    session,
    profile,
    isLoading,
    isAuthenticated,
    isProfileComplete,
    signUp,
    signIn,
    signInWithOAuth,
    signOut,
    updateProfile,
    fetchProfile,
    resetPassword,
    deleteAccount,
    getAuthRedirectUrl,
  };

  return (
    <UserAuthContext.Provider value={value}>
      {children}
    </UserAuthContext.Provider>
  );
}

export default UserAuthContext;
