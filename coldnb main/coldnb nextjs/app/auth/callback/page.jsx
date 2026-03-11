import AuthCallbackContent from "@/components/auth/AuthCallbackContent";
import { Suspense } from "react";

export const dynamic = "force-dynamic";

export default function AuthCallbackPage() {
  return (
    <Suspense fallback={null}>
      <AuthCallbackContent />
    </Suspense>
  );
}
