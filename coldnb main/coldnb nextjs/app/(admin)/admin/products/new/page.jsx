"use client";
import { useState, useEffect } from "react";
import { useRouter } from "next/navigation";
import toast from "react-hot-toast";
import ProductForm from "@/components/admin/forms/ProductForm";
import { adminProducts, adminCategories } from "@/lib/api/adminProducts";

export default function NewProductPage() {
  const router = useRouter();
  const [categories, setCategories] = useState([]);
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [error, setError] = useState(null);

  useEffect(() => {
    // Fetch categories from API
    const fetchCategories = async () => {
      try {
        const response = await adminCategories.list();
        // API returns {success: true, data: [...]}
        const data = response.data?.data || response.data?.categories || response.data || [];
        setCategories(Array.isArray(data) ? data : []);
      } catch (_err) {
        setCategories([]);
      }
    };
    fetchCategories();
  }, []);

  const handleSubmit = async (data) => {
    setIsSubmitting(true);
    setError(null);
    try {
      // Extract images from data
      const { images, ...productData } = data;

      // Call real API to create product
      const response = await adminProducts.create(productData);

      // Check if the creation was successful
      if (response.data?.success) {
        const productId = response.data.data?.id;

        // Save images if product was created and we have images
        if (productId && images && images.length > 0) {
          for (let i = 0; i < images.length; i++) {
            const img = images[i];
            try {
              await adminProducts.addImage(productId, {
                url: img.url,
                alt_text: productData.name,
                is_primary: img.is_primary || i === 0,
                sort_order: i,
              });
            } catch (_imgErr) {
              // Continue with other images even if one fails
            }
          }
        }

        toast.success("Product created successfully");
        router.push("/admin/products");
      } else {
        // Handle error response - backend returns {error: {status, message}}
        const errData = response.data?.error;
        const errMsg = typeof errData === 'object' ? errData.message : errData;
        setError(errMsg || "Unexpected response from server");
      }
    } catch (err) {
      let errorMessage = "Error creating product. Please try again.";
      if (err.message && !err.response) {
        errorMessage = err.message;
      } else if (err.response?.data?.error) {
        const errorData = err.response.data.error;
        errorMessage = typeof errorData === 'object' ? errorData.message : errorData;
      }
      setError(errorMessage);
      toast.error(errorMessage);
    } finally {
      setIsSubmitting(false);
    }
  };

  return (
    <div className="new-product-page">
      <div className="admin-page-header">
        <h1 className="page-title">New Product</h1>
      </div>

      {error && (
        <div className="alert-card alert-danger" style={{ marginBottom: 24 }}>
          <div className="alert-content">
            <div className="alert-message">{error}</div>
          </div>
        </div>
      )}

      <ProductForm
        categories={categories}
        onSubmit={handleSubmit}
        isSubmitting={isSubmitting}
        submitLabel="Create Product"
      />
    </div>
  );
}
