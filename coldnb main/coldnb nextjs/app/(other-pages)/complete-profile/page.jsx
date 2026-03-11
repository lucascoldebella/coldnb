import Footer1 from "@/components/footers/Footer1";
import Header1 from "@/components/headers/Header1";
import Topbar6 from "@/components/headers/Topbar6";
import AuthFlow from "@/components/auth/AuthFlow";
import React, { Suspense } from "react";

export const metadata = {
  title: "Complete Account || ColdnbMain - eCommerce",
  description: "ColdnbMain - eCommerce",
};

export default function CompleteProfilePage() {
  return (
    <>
      <Topbar6 bgColor="bg-main" />
      <Header1 />
      <Suspense fallback={null}>
        <AuthFlow initialMode="social-complete" />
      </Suspense>
      <Footer1 />
    </>
  );
}
