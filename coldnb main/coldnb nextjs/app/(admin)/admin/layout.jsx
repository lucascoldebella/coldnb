"use client";
import "@/public/scss/admin/_index.scss";
import { Toaster } from "react-hot-toast";
import { AdminProvider } from "@/context/AdminContext";
import AdminLayout from "@/components/admin/layout/AdminLayout";

export default function AdminRootLayout({ children }) {
  return (
    <AdminProvider>
      <AdminLayout>{children}</AdminLayout>
      <Toaster position="top-right" />
    </AdminProvider>
  );
}
