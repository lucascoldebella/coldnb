# ColdnbMain Ecommerce Template - Technical Documentation

> Complete technical blueprint for managing this Next.js ecommerce frontend

---

## Quick Reference

| Aspect | Details |
|--------|---------|
| Framework | Next.js 15.1.6 (App Router) |
| React | 19.0.0 |
| Styling | SCSS/Bootstrap 5.3.2 |
| State | React Context API |
| Icons | Custom icomoon font + Bootstrap Icons |
| Animations | WOW.js (custom), Swiper 11.1.15 |

---

## Project Structure Overview

```
coldnb nextjs/
├── app/                    # Next.js App Router pages
│   ├── (blogs)/           # Blog pages
│   ├── (homes)/           # Homepage variants (~30 themes)
│   ├── (my-account)/      # User account pages
│   ├── (other-pages)/     # Utility pages (contact, FAQ, etc.)
│   ├── (productDetails)/  # Product detail page variants
│   ├── (products)/        # Product listing pages
│   ├── layout.js          # Root layout with all modals
│   └── page.jsx           # Main homepage
├── components/            # Reusable UI components
│   ├── blogs/            # Blog components
│   ├── common/           # Shared components
│   ├── footers/          # Footer variants
│   ├── headers/          # Header/topbar variants
│   ├── homes/            # Homepage section components
│   ├── modals/           # Modal dialogs (cart, quickview, etc.)
│   ├── my-account/       # Account section components
│   ├── otherPages/       # Utility page components
│   ├── productCards/     # Product card variants
│   ├── productDetails/   # Product detail components
│   └── products/         # Product listing components
├── context/              # React Context (state management)
├── data/                 # Static data files (products, menu, etc.)
├── public/               # Static assets (images, fonts, CSS)
├── reducer/              # (Empty - future state reducers)
└── utlis/                # Utility functions (note: typo in folder name)
```

---

## App Routes (Pages)

### Homepage Variants (`app/(homes)/`)
18 jewelry-focused homepage themes:

| Route | Category |
|-------|----------|
| `/home-jewelry-01` | Jewelry Main Theme 1 |
| `/home-jewelry-02` | Jewelry Main Theme 2 |
| `/home-jewelry-main` | Main Jewelry Homepage |
| `/home-jewelry-earrings` | Earrings Collection |
| `/home-jewelry-necklaces` | Necklaces & Chains |
| `/home-jewelry-bracelets` | Bracelets & Bangles |
| `/home-jewelry-rings` | Rings Collection |
| `/home-jewelry-pendants` | Pendants & Charms |
| `/home-jewelry-anklets` | Anklets & Foot Jewelry |
| `/home-jewelry-watches` | Watches & Timepieces |
| `/home-jewelry-wedding` | Wedding & Engagement |
| `/home-jewelry-luxury` | Luxury Collection |
| `/home-jewelry-mens` | Men's Jewelry |
| `/home-jewelry-kids` | Kids' Jewelry |
| `/home-jewelry-baby` | Baby Jewelry |
| `/home-jewelry-birthstone` | Birthstone Jewelry |
| `/home-jewelry-personalized` | Personalized & Engraved |
| `/home-jewelry-sets` | Jewelry Sets |

### Product Pages (`app/(products)/`)

| Route | Description |
|-------|-------------|
| `/shop-default` | Standard grid layout |
| `/shop-collection` | Collection view |
| `/shop-fullwidth` | Full-width grid |
| `/shop-left-sidebar`, `/shop-right-sidebar` | Sidebar filters |
| `/shop-list` | List view |
| `/shop-load-more`, `/shop-pagination` | Loading patterns |
| `/view-all-collection` | All collections |

### Product Detail Pages (`app/(productDetails)/`)
25+ product detail variants:
- Default, grid layouts, stacked views
- Swatch variants (color, image, dropdown)
- Feature variants (pre-order, out-of-stock, variable, discount)
- Up-sell, frequently bought together bundles

### Account Pages (`app/(my-account)/`)

| Route | Description |
|-------|-------------|
| `/my-account` | Account dashboard |
| `/my-account-address` | Address management |
| `/my-account-orders` | Order history |
| `/my-account-orders-details` | Order details |

### Utility Pages (`app/(other-pages)/`)

| Route | Description |
|-------|-------------|
| `/about-us` | About page |
| `/contact`, `/contact-02` | Contact forms |
| `/FAQs` | FAQ page |
| `/coming-soon` | Coming soon page |
| `/compare-products` | Product comparison |
| `/store-list-*` | Store locations |
| `/order-tracking` | Order tracking |
| `/wishlist`, `/shopping-cart`, `/checkout` | Cart flow |
| `/login`, `/register`, `/forget-password` | Auth pages |
| `/term-of-use`, `/privacy-policy` | Legal pages |

### Blog Pages (`app/(blogs)/`)

| Route | Description |
|-------|-------------|
| `/blog-default` | Default blog layout |
| `/blog-grid` | Grid layout |
| `/blog-list` | List layout |
| `/blog-detail/[id]` | Blog post detail |

---

## Key Components

### Headers (`components/headers/`)
- **Header1-Header11**: Different header layouts
- **Topbar1-Topbar11**: Top announcement bars
- **ToolbarBottom**: Mobile bottom navigation
- **Nav, NavSmall, NavFashion**: Navigation variants
- **MegaMenu**: Dropdown mega menus

### Footers (`components/footers/`)
- **Footer1**: Main footer with newsletter, links, social icons

### Modals (`components/modals/`)
| Component | Function |
|-----------|----------|
| `CartModal` | Shopping cart drawer |
| `QuickView` | Product quick preview |
| `QuickAdd` | Quick add to cart |
| `Compare` | Product comparison modal |
| `Wishlist` | Wishlist modal |
| `NewsLetterModal` | Newsletter popup |
| `SearchModal` | Search overlay |
| `SizeGuide` | Size guide modal |
| `MobileMenu` | Mobile navigation drawer |
| `DemoModal` | Theme demo selector |

### Product Components
- **productCards/**: 14 card style variants
- **productDetails/**: Gallery, info, tabs, related products
- **products/**: Grid, list, filter components

---

## Data Files (`data/`)

| File | Description |
|------|-------------|
| `products.js` | **133KB** - All product data with images, prices, variants |
| `collections.js` | **33KB** - Collection/category data |
| `heroSlides.js` | **18KB** - Homepage banner slides |
| `blogs.js` | **12KB** - Blog posts data |
| `menu.js` | **11KB** - Navigation menu structure |
| `testimonials.js` | **15KB** - Customer reviews |
| `footerLinks.js` | Footer navigation links |
| `brands.js` | Brand logos |
| `features.js` | Feature icons |
| `team.js` | Team member data |
| `productFilterOptions.js` | Filter configuration |
| `singleProductSliders.js` | Product image slides |

---

## State Management

### Context API (`context/Context.jsx`)

**Managed State:**
```javascript
cartProducts      // Shopping cart items
wishList          // Wishlist product IDs
compareItem       // Compare list product IDs
quickViewItem     // Current quick view product
quickAddItem      // Current quick add product
totalPrice        // Calculated cart total
```

**Key Functions:**
| Function | Description |
|----------|-------------|
| `addProductToCart(id, qty)` | Add product with quantity |
| `updateQuantity(id, qty)` | Update cart item quantity |
| `addToWishlist(id)` | Add to wishlist |
| `removeFromWishlist(id)` | Remove from wishlist |
| `addToCompareItem(id)` | Add to compare |
| `isAddedToCartProducts(id)` | Check if in cart |
| `isAddedtoWishlist(id)` | Check if in wishlist |

**Persistence:** Cart and wishlist saved to `localStorage`

---

## External Integrations

### 1. Newsletter (DISABLED - Configure Your Own)

**Location:** `components/modals/NewsLetterModal.jsx`, `components/footers/Footer1.jsx`

```javascript
// TODO: Replace with your own newsletter service API
console.log("Newsletter signup (configure your service):", email);
```

**Original service was:** Brevo (Sendinblue) via `express-brevomail.vercel.app`  
**Action needed:** Connect to your email marketing service (Mailchimp, SendGrid, etc.)

### 2. Contact Forms (EmailJS - Needs Configuration)

**Location:** `components/otherPages/Contact1.jsx`, `Contact2.jsx`, `Contact3.jsx`

```javascript
import emailjs from "@emailjs/browser";
emailjs.sendForm(serviceId, templateId, form, publicKey);
```

**Action needed:** Create EmailJS account and add your service/template IDs

### 3. Google Maps (Store Locations)

**Location:** `components/otherPages/StoreLocations1-3.jsx`, `app/(other-pages)/contact/page.jsx`

Uses embedded Google Maps iframe. Replace coordinates for your store locations.

---

## Styling Architecture

### File Structure (`public/scss/`)
```
scss/
├── main.scss           # Main entry point
├── app.scss           # Base styles
├── component/         # Component styles
│   ├── _animation.scss
│   ├── _banner.scss
│   ├── _box-icon.scss
│   ├── _button.scss
│   ├── _card.scss
│   ├── _footer.scss
│   ├── _header.scss
│   ├── _pop-up.scss
│   ├── _shop.scss
│   └── ... (31 files)
└── libs/             # Third-party styles
```

### CSS Custom Properties (Key Variables)

```css
--primary-color        /* Main brand color */
--secondary-color      /* Accent color */
--text-color          /* Body text */
--heading-color       /* Headings */
--bg-main             /* Background */
--facebook-cl         /* Social colors */
```

### Icon Font (`public/fonts/`)
- **icomoon.svg/woff/ttf** - Custom icon font
- **font-icons.css** - Icon class definitions

Preview icons: `.icon-cart`, `.icon-heart`, `.icon-user`, `.icon-search`, etc.

---

## Utility Functions (`utlis/`)

| File | Functions |
|------|-----------|
| `wow.js` | WOW animation library (scroll reveal) |
| `openCartModal.js` | `openCartModal()` - trigger cart drawer |
| `openWishlist.js` | `openWistlistModal()` - trigger wishlist modal |

---

## Root Layout (`app/layout.js`)

The main layout wraps all pages with:
- Bootstrap JS initialization
- Header scroll behavior
- Modal instances (Cart, QuickView, Compare, etc.)
- WOW.js animations
- RTL toggle support

**Key effects:**
- Scroll > 100px → Header gets `header-bg` class
- Scroll direction tracking for sticky header
- Auto-close modals on route change

---

## Security Notes & Configuration Required

### ⚠️ Must Configure

1. **Newsletter Service**
   - Location: `Footer1.jsx`, `NewsLetterModal.jsx`
   - Currently: Logs to console only
   - Action: Connect your email service

2. **EmailJS for Contact Forms**
   - Location: `Contact1/2/3.jsx`
   - Action: Add service ID, template ID, public key

3. **Placeholder Contact Info**
   - Footer shows: `themesflat@gmail.com`, `315-666-6688`
   - Action: Replace with real contact info

4. **Google Maps Embed**
   - Shows: New York coordinates
   - Action: Update with your store location(s)

### ✅ Safe & Ready

- No external analytics
- No tracking pixels
- LocalStorage for cart (client-only)
- All images local
- Standard npm dependencies

---

## Build & Run Commands

```bash
# Development
npm run dev

# Production build
npm run build

# Start production
npm start

# Lint
npm run lint
```

---

## Quick Start Customization

1. **Change Logo**: Replace `/images/logo/logo.svg` and `logo-white.svg`
2. **Update Products**: Edit `data/products.js`
3. **Modify Menu**: Edit `data/menu.js`
4. **Change Theme Colors**: Edit SCSS variables in `public/scss/`
5. **Update Footer**: Edit `components/footers/Footer1.jsx`
6. **Add Real Contact**: Edit `Contact1/2/3.jsx` with EmailJS credentials
