# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Next.js 15 + React 19 e-commerce frontend for a jewelry store. Part of the Coldnb full-stack platform (see parent `coldnb/CLAUDE.md` for backend and full architecture).

## Commands

```bash
npm run dev       # Development server on :3000
npm run build     # Production build
npm run start     # Start production server
npm run lint      # ESLint
```

## Architecture

### State Management

Two separate context systems:

1. **Customer Context** (`context/Context.jsx`) - Shopping experience
   - `useContextElement()` hook for cart, wishlist, compare, quick view
   - Persists cart/wishlist to localStorage
   - Functions: `addProductToCart()`, `addToWishlist()`, `isAddedToCartProducts()`

2. **Admin Context** (`context/AdminContext.jsx`) - Admin dashboard
   - `useAdmin()` hook for auth, permissions, dashboard config
   - JWT stored in localStorage as `adminToken`
   - `hasPermission(key)` for granular permission checks
   - `super_admin` role bypasses all permission checks

### API Integration

- **Admin API** (`lib/adminApi.js`): Axios instance with JWT interceptor, base URL from `NEXT_PUBLIC_API_URL` env var (defaults to `localhost:8080`)
- Backend expects different auth headers for customers (Supabase) vs admins (custom JWT)

### Route Groups

| Group | Purpose |
|-------|---------|
| `(homes)/` | 18 jewelry homepage themes |
| `(products)/` | Shop pages with various layouts |
| `(productDetails)/` | 25+ product detail variants |
| `(my-account)/` | Customer account pages |
| `(admin)/` | Admin dashboard (separate layout) |
| `(other-pages)/` | Contact, FAQ, login, etc. |

### Root Layout (`app/layout.js`)

All pages wrapped with:
- Bootstrap JS initialization
- Context provider with all modals (Cart, QuickView, Compare, etc.)
- Header scroll behavior (sticky, hide/show on scroll direction)
- WOW.js animations
- Modals auto-close on route change

## Key Patterns

### Product Data

Static product data in `data/products.js` (133KB). Access via `allProducts` array. Each product has: id, title, price, imgSrc, colors[], sizes[], category, etc.

### Modal System

Modals are pre-rendered in layout, triggered via Bootstrap data attributes or utility functions:
- `openCartModal()` from `utlis/openCartModal.js`
- `openWistlistModal()` from `utlis/openWishlist.js`

### Bilingual i18n System

The frontend supports **Portuguese (PT-BR)** and **English (EN)**:

**Architecture:**
- `lib/i18n/LanguageContext.jsx` - React context providing `useLanguage()` hook
  - `lang` - Current language ("pt-BR" or "en")
  - `setLang(newLang)` - Change language, updates localStorage & `<html lang>`
  - `t(keyPath)` - Translate using dot-notation: `t("nav.home")`, `t("cart.subtotal")`
- `lib/i18n/translations/pt.json` - Portuguese strings (namespaces: nav, cart, checkout, etc.)
- `lib/i18n/translations/en.json` - English strings (same keys)
- `components/common/LanguageSelect.jsx` - Language toggle (rewired to use context)

**Usage Pattern:**
```jsx
"use client";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function MyComponent() {
  const { t } = useLanguage();
  return <h1>{t("nav.home")}</h1>;
}
```

**Key Rules (CRITICAL):**
1. Always add `"use client"` at top of component if using `useLanguage()` — required for client-side context access
2. Admin components (in `app/(admin)`) do NOT use translations — remain English only
3. String interpolation: use `.replace("{key}", value)` on translated strings (e.g., `t("cart.freeShippingSpend").replace("{amount}", "R$ 250")`)
4. Update **both** `pt.json` AND `en.json` when adding new strings — they must have identical key structure
5. Never re-add Themesflat, themeforest, Vietnamese, or demo name references
6. Language persists to localStorage as `coldnb-lang` key — users' choice is remembered across sessions
7. `<html lang>` attribute updates automatically when language changes — for SEO and accessibility

**Translation Namespaces (500+ keys):**
| Namespace | Keys | Example |
|-----------|------|---------|
| nav | 32 | `t("nav.home")` → "Início" |
| cart | 45 | `t("cart.addToCart")` → "Adicionar ao Carrinho" |
| checkout | 42 | `t("checkout.firstName")` → "Primeiro Nome*" |
| shopCart | 28 | `t("shopCart.processCheckout")` → "Ir para Checkout" |
| product | 35 | `t("product.quickView")` → "Visualização Rápida" |
| shop | 24 | `t("shop.filters")` → "Filtros" |
| myAccount | 22 | `t("myAccount.addNewAddress")` → "Adicionar Novo Endereço" |
| orders | 18 | `t("orders.orderReceived")` → "Obrigado! Seu pedido foi recebido" |
| blog | 22 | `t("blog.readMore")` → "Ler Mais" |
| homepage | 35 | `t("homepage.shopNow")` → "Comprar Agora" |
| contact | 18 | `t("contact.getInTouch")` → "Entre em Contato" |
| features | 8 | `t("features.shippingTitle")` → "Frete Grátis" |
| *+ 13 more* | 150+ | features, footer, newsletter, login, register, etc. |

**✅ FULLY WIRED (100+ files - All customer-facing strings translated):**
- **otherPages** (16): Login, Register, ForgotPass, Wishlist, About, Faqs, Contact1-3, OrderTrac, CommingSoon, ProductCompare, Terms, Team, Testimonials, RecentProducts
- **Cart/Checkout** (2): ShopCart, Checkout
- **Modals** (6): QuickView, QuickAdd, Wishlist modal, Compare, SizeGuide, AccountSidebar
- **My Account** (5): AccountSidebar, Information, Address, OrderDetails, Orders
- **ProductCards** (14): All 13 variants (ProductCard1,3-5,7-13) + ProductsCards6, BannerTabProduct
- **Shop/ProductDetails** (23): Filters (Sidebar/Modal/Dropdown), Sorting, Products1, SearchProducts, Breadcrumb, SizeSelect, Details1, Description tabs (Descriptions1, Description, Shipping, ReturnPolicies, Reviews, ReviewSorting)
- **Common/Homepage** (41): Blogs, BannerTab, BannerTab2, Categories, Countdown, LookbookProduct, MarqueeSection, MarqueeSection2, Products (all variants), ShopGram, Testimonials, Tiktok + all homes/ subdirectories (home-1, eleganceNest, jewelry-01, jewelry-02)
- **Blogs/StoreLocations** (11): BlogDetail1-2, Sidebar, Sidebar2, CommentForm, Comments, BlogList, RelatedBlogs, StoreLocations1-3

### Styling

SCSS in `public/scss/` with Bootstrap 5.3.2. Custom icon font at `public/fonts/icomoon.*`.

## Gotchas

- **Directory name has space**: Always quote path `"coldnb main/coldnb nextjs"` in shell commands
- **Typo in folder**: `utlis/` not `utils/`
- **localStorage sync**: Cart/wishlist don't sync across tabs; language setting DOES persist via `coldnb-lang`
- **Admin vs customer routes**: Don't mix - admin has separate layout and context; admin components MUST NOT use useLanguage()
- **API base URL**: Hardcoded to localhost:8080 in dev; set `NEXT_PUBLIC_API_URL` for production
- **useLanguage() hook**: Client-side only — cannot call from server components. Always wrap with `"use client"` or pass translation to props
- **Build errors on "useLanguage called from server"**: Add `"use client"` directive at the very top of the file (before imports)
