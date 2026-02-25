import AuthGuard from "@/components/my-account/AuthGuard";

export default function MyAccountLayout({ children }) {
  return <AuthGuard>{children}</AuthGuard>;
}
