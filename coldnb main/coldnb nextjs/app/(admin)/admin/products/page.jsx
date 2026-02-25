"use client";
import { useState, useEffect, useMemo, useCallback } from "react";
import Link from "next/link";
import { useRouter, useSearchParams } from "next/navigation";
import toast from "react-hot-toast";
import { useAdmin } from "@/context/AdminContext";
import DataTable from "@/components/admin/tables/DataTable";
import StockBadge from "@/components/admin/products/StockBadge";
import { adminProducts, adminCategories } from "@/lib/api/adminProducts";
import { formatCurrency, exportToCSV } from "@/lib/adminUtils";

const PlusIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <line x1="12" y1="5" x2="12" y2="19" />
    <line x1="5" y1="12" x2="19" y2="12" />
  </svg>
);

const EditIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" />
    <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" />
  </svg>
);

const TrashIcon = () => (
  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="3 6 5 6 21 6" />
    <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" />
  </svg>
);

const DownloadIcon = () => (
  <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" />
    <polyline points="7 10 12 15 17 10" />
    <line x1="12" y1="15" x2="12" y2="3" />
  </svg>
);

const ExternalLinkIcon = () => (
  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6" />
    <polyline points="15 3 21 3 21 9" />
    <line x1="10" y1="14" x2="21" y2="3" />
  </svg>
);

const PRODUCT_ROUTE = "/product-detail";

const fallbackCategories = [
  { id: 0, name: "All Categories", slug: "all" },
];

export default function ProductsPage() {
  const router = useRouter();
  const searchParams = useSearchParams();
  const { hasPermission } = useAdmin();

  const [loading, setLoading] = useState(true);
  const [products, setProducts] = useState([]);
  const [categories, setCategories] = useState([]);
  const [selectedIds, setSelectedIds] = useState([]);
  const [filters, setFilters] = useState({
    category: searchParams.get("category") || "all",
    stock: searchParams.get("stock") || "all",
    status: searchParams.get("status") || "all",
  });

  useEffect(() => {
    const fetchData = async () => {
      try {
        const [productsRes, categoriesRes] = await Promise.all([
          adminProducts.list({ limit: 100, show_inactive: "true" }),
          adminCategories.list().catch(() => ({ data: { data: [] } })),
        ]);

        const productsData = productsRes.data?.data?.products || productsRes.data?.products || productsRes.data?.data || [];
        setProducts(Array.isArray(productsData) ? productsData : []);

        const categoriesData = categoriesRes.data?.data || categoriesRes.data?.categories || categoriesRes.data || [];
        const catArray = Array.isArray(categoriesData) ? categoriesData : [];
        setCategories([
          { id: 0, name: "All Categories", slug: "all" },
          ...catArray.map(cat => ({
            ...cat,
            slug: cat.slug || cat.name.toLowerCase().replace(/\s+/g, "-"),
          })),
        ]);
      } catch (_error) {
        setProducts([]);
        setCategories(fallbackCategories);
      } finally {
        setLoading(false);
      }
    };

    fetchData();
  }, []);

  const filteredProducts = useMemo(() => {
    return products.filter((product) => {
      const productCategory = product.category_name || product.category || "";
      if (filters.category !== "all" && productCategory.toLowerCase() !== filters.category.toLowerCase()) {
        return false;
      }
      if (filters.stock === "in" && product.stock_quantity <= 0) return false;
      if (filters.stock === "low" && (product.stock_quantity <= 0 || product.stock_quantity > 10)) return false;
      if (filters.stock === "out" && product.stock_quantity > 0) return false;
      if (filters.status === "active" && !product.is_active) return false;
      if (filters.status === "inactive" && product.is_active) return false;
      return true;
    });
  }, [products, filters]);

  const handleFilterChange = (key, value) => {
    setFilters(prev => ({ ...prev, [key]: value }));
  };

  const handleDelete = useCallback(async (id) => {
    if (!confirm("Are you sure you want to delete this product?")) return;
    try {
      await adminProducts.delete(id);
      setProducts(prev => prev.filter(p => p.id !== id));
      toast.success("Product deleted");
    } catch (error) {
      const errData = error.response?.data?.error;
      const errMsg = typeof errData === 'object' ? errData.message : errData;
      toast.error(errMsg || "Failed to delete product");
    }
  }, []);

  const handleBulkDelete = useCallback(async () => {
    if (!confirm(`Are you sure you want to delete ${selectedIds.length} products?`)) return;
    try {
      await adminProducts.bulkDelete(selectedIds);
      setProducts(prev => prev.filter(p => !selectedIds.includes(p.id)));
      setSelectedIds([]);
      toast.success(`${selectedIds.length} products deleted`);
    } catch (error) {
      const errData = error.response?.data?.error;
      const errMsg = typeof errData === 'object' ? errData.message : errData;
      toast.error(errMsg || "Failed to delete products");
    }
  }, [selectedIds]);

  const handleExport = useCallback(() => {
    const csvColumns = [
      { header: "Name", accessor: "name" },
      { header: "SKU", accessor: "sku" },
      { header: "Category", accessor: "category_name", exportValue: (r) => r.category_name || r.category || "" },
      { header: "Price", accessor: "price" },
      { header: "Stock", accessor: "stock_quantity" },
      { header: "Status", accessor: "is_active", exportValue: (r) => r.is_active ? "Active" : "Inactive" },
    ];
    exportToCSV(filteredProducts, csvColumns, "products");
    toast.success("Products exported");
  }, [filteredProducts]);

  const getStoreBaseUrl = () => {
    if (typeof window !== "undefined") return window.location.origin;
    return "";
  };

  const columns = useMemo(() => [
    {
      header: "Product",
      accessor: "name",
      render: (row) => (
        <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
          <div
            style={{
              width: 44,
              height: 44,
              borderRadius: 8,
              backgroundColor: "#f3f4f6",
              overflow: "hidden",
              flexShrink: 0,
            }}
          >
            {(row.image_url || row.image) ? (
              <img
                src={row.image_url || row.image}
                alt={row.name}
                style={{ width: "100%", height: "100%", objectFit: "cover" }}
              />
            ) : (
              <div style={{ width: "100%", height: "100%", display: "flex", alignItems: "center", justifyContent: "center", color: "#9ca3af", fontSize: 10 }}>
                N/A
              </div>
            )}
          </div>
          <div>
            <div style={{ fontWeight: 500 }}>{row.name}</div>
            <div style={{ fontSize: 12, color: "#6b7280" }}>{row.sku}</div>
          </div>
        </div>
      ),
    },
    {
      header: "Category",
      accessor: "category_name",
      width: "120px",
      render: (row) => row.category_name || row.category || "—",
    },
    {
      header: "Price",
      accessor: "price",
      width: "120px",
      render: (row) => formatCurrency(row.price),
    },
    {
      header: "Stock",
      accessor: "stock_quantity",
      width: "120px",
      render: (row) => <StockBadge quantity={row.stock_quantity} />,
    },
    {
      header: "Status",
      accessor: "is_active",
      width: "100px",
      render: (row) => (
        <span className={`admin-badge ${row.is_active ? "badge-success" : "badge-danger"}`}>
          {row.is_active ? "Active" : "Inactive"}
        </span>
      ),
    },
    {
      header: "URL",
      accessor: "id",
      width: "180px",
      render: (row) => {
        const productUrl = `${getStoreBaseUrl()}${PRODUCT_ROUTE}/${row.id}`;
        return (
          <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
            <span style={{
              fontSize: 12,
              color: "#6b7280",
              overflow: "hidden",
              textOverflow: "ellipsis",
              whiteSpace: "nowrap",
              maxWidth: 120
            }}>
              {PRODUCT_ROUTE}/{row.id}
            </span>
            <a
              href={productUrl}
              target="_blank"
              rel="noopener noreferrer"
              className="action-btn"
              onClick={(e) => e.stopPropagation()}
              title="Open in new tab"
              style={{
                padding: 4,
                borderRadius: 4,
                color: "#6b7280",
                display: "flex",
                alignItems: "center",
                justifyContent: "center"
              }}
            >
              <ExternalLinkIcon />
            </a>
          </div>
        );
      },
    },
    {
      header: "Actions",
      accessor: "actions",
      sortable: false,
      width: "100px",
      className: "cell-actions",
      render: (row) => (
        <div className="table-row-actions">
          {hasPermission("edit_products") && (
            <Link href={`/admin/products/${row.id}`} className="action-btn action-edit">
              <EditIcon />
            </Link>
          )}
          {hasPermission("delete_products") && (
            <button className="action-btn action-delete" onClick={(e) => { e.stopPropagation(); handleDelete(row.id); }}>
              <TrashIcon />
            </button>
          )}
        </div>
      ),
    },
  ], [hasPermission, handleDelete]);

  return (
    <div className="products-page">
      <div className="admin-page-header">
        <h1 className="page-title">Products</h1>
        <div className="page-actions">
          <button className="admin-btn btn-secondary" onClick={handleExport}>
            <DownloadIcon />
            Export
          </button>
          {hasPermission("create_products") && (
            <Link href="/admin/products/new" className="admin-btn btn-primary">
              <PlusIcon />
              Add Product
            </Link>
          )}
        </div>
      </div>

      {selectedIds.length > 0 && (
        <div className="admin-card" style={{ marginBottom: 16, padding: "12px 20px" }}>
          <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
            <span>{selectedIds.length} product(s) selected</span>
            <button className="admin-btn btn-danger btn-sm" onClick={handleBulkDelete}>
              <TrashIcon />
              Delete Selected
            </button>
          </div>
        </div>
      )}

      <DataTable
        columns={columns}
        data={filteredProducts}
        loading={loading}
        selectable={hasPermission("delete_products")}
        selectedIds={selectedIds}
        onSelectionChange={setSelectedIds}
        searchPlaceholder="Search products..."
        onRowClick={(row) => router.push(`/admin/products/${row.id}`)}
        filters={
          <>
            <div className="filter-select">
              <select
                value={filters.category}
                onChange={(e) => handleFilterChange("category", e.target.value)}
              >
                {categories.map((cat) => (
                  <option key={cat.id} value={cat.slug}>
                    {cat.name}
                  </option>
                ))}
              </select>
            </div>
            <div className="filter-select">
              <select
                value={filters.stock}
                onChange={(e) => handleFilterChange("stock", e.target.value)}
              >
                <option value="all">All Stock</option>
                <option value="in">In Stock</option>
                <option value="low">Low Stock</option>
                <option value="out">Out of Stock</option>
              </select>
            </div>
            <div className="filter-select">
              <select
                value={filters.status}
                onChange={(e) => handleFilterChange("status", e.target.value)}
              >
                <option value="all">All Status</option>
                <option value="active">Active</option>
                <option value="inactive">Inactive</option>
              </select>
            </div>
          </>
        }
      />
    </div>
  );
}
