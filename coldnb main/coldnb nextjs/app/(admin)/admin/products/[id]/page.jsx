"use client";
import { useState, useEffect } from "react";
import { useRouter, useParams } from "next/navigation";
import toast from "react-hot-toast";
import ProductForm from "@/components/admin/forms/ProductForm";
import { adminProducts, adminCategories } from "@/lib/api/adminProducts";

export default function EditProductPage() {
  const router = useRouter();
  const params = useParams();
  const [product, setProduct] = useState(null);
  const [categories, setCategories] = useState([]);
  const [loading, setLoading] = useState(true);
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [error, setError] = useState(null);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const [productRes, categoriesRes] = await Promise.all([
          adminProducts.get(params.id),
          adminCategories.list().catch(() => ({ data: { data: [] } })),
        ]);

        if (productRes.data?.error) {
          const errMsg = productRes.data.error;
          throw new Error(typeof errMsg === 'object' ? errMsg.message : errMsg || "Failed to load product");
        }
        if (productRes.data?.success === false) {
          throw new Error(productRes.data?.message || "Failed to load product");
        }
        const productData = productRes.data?.data || productRes.data?.product || productRes.data;
        if (!productData || !productData.id) {
          throw new Error("Invalid product data received");
        }
        setProduct(productData);

        const categoriesData = categoriesRes.data?.data || categoriesRes.data?.categories || categoriesRes.data || [];
        setCategories(Array.isArray(categoriesData) ? categoriesData : []);
      } catch (err) {
        let errorMessage = "Product not found";
        if (err.message) {
          errorMessage = err.message;
        } else if (err.response?.data?.error) {
          const errorData = err.response.data.error;
          errorMessage = typeof errorData === 'object' ? errorData.message : errorData;
        }
        setError(errorMessage);
        setProduct(null);
      } finally {
        setLoading(false);
      }
    };

    if (params.id) {
      fetchData();
    }
  }, [params.id]);

  const handleSubmit = async (data) => {
    setIsSubmitting(true);
    setError(null);
    try {
      const { images, deletedImageIds, ...productData } = data;

      // Update product data
      await adminProducts.update(params.id, productData);

      // Delete images that were removed
      if (deletedImageIds && deletedImageIds.length > 0) {
        for (const imageId of deletedImageIds) {
          try {
            await adminProducts.deleteImage(params.id, imageId);
          } catch (_imgErr) {
            // Silently continue — image may already be deleted
          }
        }
      }

      // Process images
      if (images && images.length > 0) {
        // Find the image marked as primary
        const primaryImage = images.find(img => img.is_primary);

        // Handle primary for existing DB images
        if (primaryImage && primaryImage.product_id) {
          try {
            await adminProducts.updateImage(params.id, primaryImage.id, {
              is_primary: true,
            });
          } catch (_imgErr) {
            // Non-critical — continue
          }
        }

        // Add new images (no product_id = not yet in DB)
        const newImages = images.filter(img => img.url && !img.product_id);
        for (let i = 0; i < newImages.length; i++) {
          const img = newImages[i];
          try {
            await adminProducts.addImage(params.id, {
              url: img.url,
              alt_text: productData.name || product.name,
              is_primary: img.is_primary || false,
              sort_order: i,
            });
          } catch (_imgErr) {
            // Non-critical — continue
          }
        }
      }

      toast.success("Product updated successfully");
      router.push("/admin/products");
    } catch (err) {
      const errorData = err.response?.data?.error;
      const errorMessage = typeof errorData === 'object'
        ? (errorData?.message || JSON.stringify(errorData))
        : (errorData || "Error updating product. Please try again.");
      setError(errorMessage);
      toast.error(errorMessage);
    } finally {
      setIsSubmitting(false);
    }
  };

  if (loading) {
    return (
      <div className="edit-product-page">
        <div className="admin-page-header">
          <div className="skeleton skeleton-title" style={{ width: 200 }} />
        </div>
        <div className="admin-card">
          <div className="card-body">
            <div className="skeleton skeleton-text" style={{ marginBottom: 16 }} />
            <div className="skeleton skeleton-text" style={{ marginBottom: 16 }} />
            <div className="skeleton skeleton-text" />
          </div>
        </div>
      </div>
    );
  }

  if (!product) {
    return (
      <div className="edit-product-page">
        <div className="admin-page-header">
          <h1 className="page-title">Product Not Found</h1>
        </div>
        <div className="admin-card">
          <div className="card-body" style={{ textAlign: "center", padding: 40 }}>
            <p>{error || "The product you're looking for doesn't exist."}</p>
            <button className="admin-btn btn-primary" onClick={() => router.push("/admin/products")}>
              Back to Products
            </button>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="edit-product-page">
      <div className="admin-page-header">
        <h1 className="page-title">Edit Product</h1>
      </div>

      {error && (
        <div className="alert-card alert-danger" style={{ marginBottom: 24 }}>
          <div className="alert-content">
            <div className="alert-message">{error}</div>
          </div>
        </div>
      )}

      <ProductForm
        initialData={product}
        categories={categories}
        onSubmit={handleSubmit}
        isSubmitting={isSubmitting}
        submitLabel="Update Product"
      />
    </div>
  );
}
