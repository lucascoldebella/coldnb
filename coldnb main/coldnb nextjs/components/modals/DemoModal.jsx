"use client";
import React from "react";
import Image from "next/image";
import Link from "next/link";
import useNavigationData from "@/lib/hooks/useNavigationData";
import { usePathname } from "next/navigation";

export default function DemoModal() {
  const pathname = usePathname();
  const { menusBySlug } = useNavigationData();

  const inicio = menusBySlug.inicio;
  const items = [];
  if (inicio && inicio.groups) {
    for (const group of inicio.groups) {
      for (const item of group.items || []) {
        items.push(item);
      }
    }
  }

  return (
    <div className="modal fade modalDemo" id="modalDemo">
      <div className="modal-dialog modal-dialog-centered">
        <div className="modal-content">
          <div className="mega-menu">
            <div className="row-demo">
              {items.map((item) => (
                <div
                  className={`demo-item ${pathname === item.href ? "active" : ""}`}
                  key={item.id || item.href}
                >
                  <Link href={item.href}>
                    <div className="demo-image position-relative">
                      {item.image_url && (
                        <Image
                          className="lazyload"
                          data-src={item.image_url}
                          alt={item.image_alt || item.label}
                          src={item.image_url}
                          width={273}
                          height={300}
                        />
                      )}
                      {item.badge && (
                        <div className="demo-label">
                          <span className={`demo-${item.badge.toLowerCase()}`}>{item.badge}</span>
                        </div>
                      )}
                    </div>
                    <span className="demo-name">{item.label}</span>
                  </Link>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
