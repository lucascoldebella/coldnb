import Footer1 from "@/components/footers/Footer1";
import Header1 from "@/components/headers/Header1";
import Topbar6 from "@/components/headers/Topbar6";
import AccountSidebar from "@/components/my-account/AccountSidebar";
import LoyaltyDashboard from "@/components/my-account/LoyaltyDashboard";
import Link from "next/link";
import React from "react";

export const metadata = {
  title: "Loyalty Rewards || Coldnb",
  description: "Coldnb loyalty rewards program",
};

export default function MyAccountLoyaltyPage() {
  return (
    <>
      <Topbar6 bgColor="bg-main" />
      <Header1 />
      <div
        className="page-title"
        style={{ backgroundImage: "url(/images/section/page-title.jpg)" }}
      >
        <div className="container-full">
          <div className="row">
            <div className="col-12">
              <h3 className="heading text-center">Loyalty Rewards</h3>
              <ul className="breadcrumbs d-flex align-items-center justify-content-center">
                <li>
                  <Link className="link" href="/">
                    Homepage
                  </Link>
                </li>
                <li>
                  <i className="icon-arrRight" />
                </li>
                <li>Loyalty Rewards</li>
              </ul>
            </div>
          </div>
        </div>
      </div>
      <section className="flat-spacing">
        <div className="container">
          <div className="my-account-wrap">
            <AccountSidebar />
            <LoyaltyDashboard />
          </div>
        </div>
      </section>
      <Footer1 />
    </>
  );
}
