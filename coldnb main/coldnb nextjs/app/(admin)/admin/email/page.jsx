"use client";

const channels = [
  {
    title: "Transactional API",
    description: "Store emails sent directly by the C backend through Brevo's HTTP API.",
    items: [
      ["Provider", "Brevo API v3"],
      ["Sender", "noreply@coldnb.com"],
      ["Reply-to", "support@coldnb.com"],
      ["Internal alerts", "email@coldnb.com"],
    ],
  },
  {
    title: "Auth SMTP",
    description: "Supabase authentication emails sent through Brevo SMTP.",
    items: [
      ["Provider", "Brevo SMTP relay"],
      ["Host", "smtp-relay.brevo.com"],
      ["Port", "587"],
      ["Managed in", "Supabase dashboard"],
    ],
  },
];

const events = [
  "Contact form notifications",
  "Customer order confirmations",
  "Internal new-order alerts",
  "Customer order status updates",
];

export default function EmailAdminPage() {
  return (
    <div className="marketing-page">
      <div className="admin-page-header">
        <div>
          <h1 className="page-title">E-mail</h1>
          <p className="admin-page-subtitle">
            Operational view of Coldnb's email stack and current delivery paths.
          </p>
        </div>
      </div>

      <div className="dashboard-grid grid-stats" style={{ marginBottom: 24 }}>
        {channels.map((channel) => (
          <section key={channel.title} className="admin-card" style={{ padding: 24 }}>
            <div style={{ marginBottom: 16 }}>
              <h3 style={{ margin: 0, fontSize: 20 }}>{channel.title}</h3>
              <p style={{ margin: "8px 0 0", color: "var(--admin-text-secondary)" }}>
                {channel.description}
              </p>
            </div>

            <div style={{ display: "grid", gap: 12 }}>
              {channel.items.map(([label, value]) => (
                <div
                  key={label}
                  style={{
                    display: "flex",
                    justifyContent: "space-between",
                    gap: 16,
                    paddingBottom: 12,
                    borderBottom: "1px solid var(--admin-border)",
                  }}
                >
                  <span style={{ color: "var(--admin-text-secondary)" }}>{label}</span>
                  <strong style={{ textAlign: "right" }}>{value}</strong>
                </div>
              ))}
            </div>
          </section>
        ))}
      </div>

      <section className="admin-card" style={{ padding: 24, marginBottom: 24 }}>
        <h3 style={{ marginTop: 0, marginBottom: 16 }}>Currently wired events</h3>
        <div style={{ display: "grid", gap: 12 }}>
          {events.map((event) => (
            <div
              key={event}
              style={{
                display: "flex",
                alignItems: "center",
                gap: 12,
                padding: "12px 14px",
                borderRadius: 12,
                background: "var(--admin-bg-secondary)",
              }}
            >
              <span
                style={{
                  width: 10,
                  height: 10,
                  borderRadius: "999px",
                  background: "var(--admin-success)",
                  flexShrink: 0,
                }}
              />
              <span>{event}</span>
            </div>
          ))}
        </div>
      </section>

      <section className="admin-card" style={{ padding: 24 }}>
        <h3 style={{ marginTop: 0, marginBottom: 12 }}>Next expansion points</h3>
        <p style={{ margin: 0, color: "var(--admin-text-secondary)", lineHeight: 1.7 }}>
          This area is ready to grow into template previews, delivery logs, sender rotation,
          support inbox routing, and manual resend tools without changing the current backend
          delivery model.
        </p>
      </section>
    </div>
  );
}
