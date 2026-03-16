# Project Brief — Coldnb

## Identity
**Name:** Coldnb
**Purpose:** Full-stack jewelry e-commerce platform for the Brazilian market (PT-BR primary, EN secondary)
**Business Objective:** Production-ready online store enabling customers to browse, purchase, and track jewelry orders, with an admin dashboard for full operational management

## Core Functional Requirements
- Customer registration/login via Supabase Auth
- Product catalog browsing with filtering, search, and variant selection (color, size)
- Server-side cart and wishlist (authenticated users) + localStorage cart (guests)
- Checkout → order creation → Stripe payment flow
- Transactional emails (order confirmation, status updates, contact notifications) via Brevo
- Admin dashboard: product CRUD, order management, customer management, analytics, email operations
- Bilingual UI (PT-BR / EN) with localStorage persistence

## Target Users
- **Customers:** Brazilian jewelry shoppers; primary language PT-BR; mobile-first browsing
- **Admins:** Store operators managing products, orders, customers, and promotions

## Problems Solved
- Customers: discover → buy → track jewelry with a polished, bilingual UX
- Admins: manage all store operations without direct database access

## Scope
**In scope:**
- Jewelry product catalog with variants (colors, sizes, images), stock badges, sold-out states
- Cart, checkout, orders, order cancellation, returns, guest checkout
- Loyalty program (auto-award on delivery, rewards catalog, redemption → discount codes)
- Admin panel (products, orders, returns, customers, inventory, discounts, newsletter subscribers, contact submissions, marketing, analytics, email, team)
- Newsletter via Brevo; contact form with email notifications + admin review
- Free shipping threshold (R$ 75+), dynamic sitemap
- Bilingual i18n (PT-BR / EN)

**Out of scope:**
- Multi-vendor marketplace
- Physical POS integration
- Multi-currency (BRL only)

## Non-Negotiable Constraints
- Backend must bind to `127.0.0.1:8080` in production — never exposed publicly
- No secrets in source code or documentation; all credentials via secret files
- Admin and customer authentication are completely separate systems
- Both PT-BR and EN translation files must be updated simultaneously
