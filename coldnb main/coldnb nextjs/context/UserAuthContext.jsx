"use client";
import React, { createContext, useContext, useState, useEffect, useCallback, useRef } from "react";
import { useRouter, usePathname } from "next/navigation";
import supabase from "@/lib/supabase";
import { profileApi } from "@/lib/userApi";

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
  const router = useRouter();
  const pathname = usePathname();

  const isAuthenticated = !!session;
  const isProfileComplete = !!(profile?.full_name && profile?.phone);

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

  // Update user profile
  const updateProfile = useCallback(async (data) => {
    const response = await profileApi.update(data);
    const updated = response.data?.data || response.data;
    setProfile(updated);
    return updated;
  }, []);

  // Sign up with email
  const signUp = useCallback(async (email, password) => {
    if (!supabase) return { error: { message: "Auth not configured" } };
    const { data, error } = await supabase.auth.signUp({ email, password });
    return { data, error };
  }, []);

  // Sign in with email
  const signIn = useCallback(async (email, password) => {
    if (!supabase) return { error: { message: "Auth not configured" } };
    const { data, error } = await supabase.auth.signInWithPassword({ email, password });
    return { data, error };
  }, []);

  // Sign in with OAuth
  const signInWithOAuth = useCallback(async (provider) => {
    if (!supabase) return { error: { message: "Auth not configured" } };
    const { data, error } = await supabase.auth.signInWithOAuth({
      provider,
      options: {
        redirectTo: `${window.location.origin}/my-account`,
      },
    });
    return { data, error };
  }, []);

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
      redirectTo: `${window.location.origin}/my-account`,
    });
    return { data, error };
  }, []);

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
        }

        if (event === "SIGNED_OUT") {
          setProfile(null);
        }
      }
    );

    return () => subscription.unsubscribe();
  }, [fetchProfile]);

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
  };

  return (
    <UserAuthContext.Provider value={value}>
      {children}
    </UserAuthContext.Provider>
  );
}

export default UserAuthContext;
