---
applies_to: ["coldnb main/coldnb nextjs/components/**", "coldnb main/coldnb nextjs/app/**"]
---

# UI Component Rules

## Admin Modal CSS Pattern (CRITICAL)

The admin CSS design system uses a specific nested class hierarchy.
Inner elements inside modals/cards do NOT use the `admin-` prefix:

```jsx
// CORRECT ✅
<div className="admin-card">
  <div className="card-header">       {/* NOT "admin-card-header" */}
    <h5 className="card-title">Title</h5>
  </div>
  <div className="card-body">        {/* NOT "admin-card-body" */}
    <div className="form-group">     {/* Bootstrap classes only */}
      <label className="form-label">Field</label>
      <input className="form-control" />
    </div>
  </div>
  <div className="card-footer">      {/* NOT "admin-card-footer" */}
    <button className="btn btn-primary">Save</button>
  </div>
</div>
```

Getting this wrong causes modal content to be invisible or render incorrectly.

## i18n in Components

### Customer-facing components
```jsx
"use client";  // REQUIRED — always add this directive first
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function MyComponent() {
  const { t } = useLanguage();
  return <h1>{t("nav.home")}</h1>;
}
```

### Admin components
- NEVER use `useLanguage()` — admin dashboard is English-only
- No `"use client"` needed just for i18n (may still need it for other React hooks)

### String interpolation
```jsx
// For keys with {placeholder} syntax:
t("cart.freeShippingSpend").replace("{amount}", "R$ 250")
```

### Adding new i18n keys
1. Add key to `lib/i18n/translations/pt.json`
2. Add same key to `lib/i18n/translations/en.json`
3. Both files MUST have identical key structures

## Component Structure Conventions
- **Route groups** (`(homes)/`, `(products)/`, etc.) — parentheses prevent URL segment
- **Admin pages** live in `app/(admin)/admin/[page]/page.jsx`
- **Shared admin components** live in `components/admin/`
- **Modals** live in `components/modals/` (pre-rendered in layout, triggered via Bootstrap)

## Modal Trigger Pattern
```jsx
// Triggering pre-rendered modals
import { openCartModal } from "@/utlis/openCartModal";
import { openWistlistModal } from "@/utlis/openWishlist";  // note: typo in filename is intentional

// Via Bootstrap data attributes
<button data-bs-toggle="modal" data-bs-target="#cartModal">Cart</button>
```

## Context Hooks
```jsx
// Customer context
import { useContextElement } from "@/context/Context";
const { cartProducts, addProductToCart, wishList, addToWishlist } = useContextElement();

// Admin context
import { useAdmin } from "@/context/AdminContext";
const { admin, token, hasPermission, logout } = useAdmin();

// Language context (customer-facing only)
import { useLanguage } from "@/lib/i18n/LanguageContext";
const { t, lang, setLang } = useLanguage();
```

## Admin API Calls
```jsx
// Always use adminApi instance (has JWT interceptor)
import adminApi from "@/lib/adminApi";
const response = await adminApi.get("/api/admin/products");
// Handles 401 auto-logout, base URL from NEXT_PUBLIC_API_URL
```

## Customer API Calls
```jsx
// Use userApi named exports (has Supabase JWT interceptor)
import { ordersApi, returnsApi, addressesApi, profileApi, cartApi } from "@/lib/userApi";
const orders = await ordersApi.list();
const returns = await returnsApi.list();
await ordersApi.cancel(orderId);

// For product data (public, no auth)
import { getProducts, getProduct, searchProducts, transformProduct } from "@/lib/shopApi";
```

## Image Upload (Admin)
```jsx
// Existing upload endpoint
POST /api/admin/upload
// Accepts: JPEG, PNG, GIF, WebP (max 5MB)
// Returns: { success: true, data: { urls: ["/uploads/products/filename.ext"] } }
// Saves to: public/uploads/products/
```

## Styling Notes
- Bootstrap 5.3.2 classes for layout and utilities
- Custom SCSS in `public/scss/` for brand-specific styles
- Icon font: `public/fonts/icomoon.*` — use `.icon-cart`, `.icon-heart`, `.icon-user`, etc.
- CSS custom properties: `--primary-color`, `--secondary-color`, `--text-color`, `--heading-color`, `--bg-main`
- Dark/light theme: `ThemeContext.jsx` + `ThemeToggle.jsx` (toggle in header)
