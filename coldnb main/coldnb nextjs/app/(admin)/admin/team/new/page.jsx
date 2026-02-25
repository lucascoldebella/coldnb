"use client";
import { useState, useEffect } from "react";
import { useRouter } from "next/navigation";
import toast from "react-hot-toast";
import { useAdmin } from "@/context/AdminContext";
import { adminEmployees } from "@/lib/api/adminEmployees";
import EmployeeForm from "@/components/admin/forms/EmployeeForm";

export default function NewEmployeePage() {
  const router = useRouter();
  const { admin } = useAdmin();
  const [isSubmitting, setIsSubmitting] = useState(false);

  const isSuperAdmin = admin?.role === "super_admin";

  useEffect(() => {
    // Redirect if not super_admin
    if (admin && !isSuperAdmin) {
      router.push("/admin/team");
    }
  }, [admin, isSuperAdmin, router]);

  const handleSubmit = async (data) => {
    setIsSubmitting(true);
    try {
      await adminEmployees.create(data);
      toast.success("Employee created successfully");
      router.push("/admin/team");
    } catch (error) {
      const errData = error.response?.data?.error;
      const errMsg = typeof errData === 'object' ? errData.message : errData;
      toast.error(errMsg || "Error creating employee. Please try again.");
    } finally {
      setIsSubmitting(false);
    }
  };

  if (!isSuperAdmin) {
    return null;
  }

  return (
    <div className="new-employee-page">
      <div className="admin-page-header">
        <h1 className="page-title">New Employee</h1>
      </div>

      <EmployeeForm
        onSubmit={handleSubmit}
        isSubmitting={isSubmitting}
        submitLabel="Create Employee"
      />
    </div>
  );
}
