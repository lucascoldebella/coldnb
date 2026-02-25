import Footer1 from "@/components/footers/Footer1";
import Header1 from "@/components/headers/Header1";
import Topbar6 from "@/components/headers/Topbar6";
import AuthFlow from "@/components/auth/AuthFlow";
import React from "react";

export const metadata = {
  title: "Login || ColdnbMain - eCommerce",
  description: "ColdnbMain - eCommerce",
};

export default function LoginPage() {
  return (
    <>
      <Topbar6 bgColor="bg-main" />
      <Header1 />
      <AuthFlow initialMode="login" />
      <Footer1 />
    </>
  );
}
