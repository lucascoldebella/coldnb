"use client";

const PERMISSION_GROUPS = [
  {
    title: "Dashboard",
    permissions: [
      { key: "view_dashboard", label: "View Dashboard" },
      { key: "customize_dashboard", label: "Customize Dashboard" },
    ],
  },
  {
    title: "Financial",
    permissions: [
      { key: "view_financial", label: "View Financial Reports" },
      { key: "view_revenue", label: "View Revenue Details" },
      { key: "export_financial", label: "Export Financial Data" },
    ],
  },
  {
    title: "Products",
    permissions: [
      { key: "view_products", label: "View Products" },
      { key: "create_products", label: "Create Products" },
      { key: "edit_products", label: "Edit Products" },
      { key: "delete_products", label: "Delete Products" },
      { key: "manage_categories", label: "Manage Categories" },
      { key: "manage_inventory", label: "Manage Inventory" },
    ],
  },
  {
    title: "Orders",
    permissions: [
      { key: "view_orders", label: "View Orders" },
      { key: "view_order_details", label: "View Order Details" },
      { key: "update_order_status", label: "Update Order Status" },
      { key: "cancel_orders", label: "Cancel Orders" },
    ],
  },
  {
    title: "Customers",
    permissions: [
      { key: "view_customers", label: "View Customers" },
      { key: "view_customer_details", label: "View Customer Details" },
      { key: "edit_customers", label: "Edit Customers" },
    ],
  },
  {
    title: "Marketing",
    permissions: [
      { key: "view_marketing", label: "View Marketing" },
      { key: "view_analytics", label: "View Analytics" },
      { key: "manage_discounts", label: "Manage Discounts" },
    ],
  },
  {
    title: "Team",
    permissions: [
      { key: "manage_team", label: "View Team" },
      { key: "create_employees", label: "Create Employees" },
      { key: "edit_employees", label: "Edit Employees" },
      { key: "assign_permissions", label: "Assign Permissions" },
    ],
  },
];

export default function PermissionCheckboxes({ permissions = {}, onChange, disabled = false }) {
  const handleToggle = (key) => {
    if (disabled) return;
    onChange({
      ...permissions,
      [key]: !permissions[key],
    });
  };

  const handleToggleGroup = (group, checked) => {
    if (disabled) return;
    const updates = {};
    group.permissions.forEach(p => {
      updates[p.key] = checked;
    });
    onChange({
      ...permissions,
      ...updates,
    });
  };

  const isGroupChecked = (group) => {
    return group.permissions.every(p => permissions[p.key] === true);
  };

  const isGroupPartial = (group) => {
    const checked = group.permissions.filter(p => permissions[p.key] === true).length;
    return checked > 0 && checked < group.permissions.length;
  };

  return (
    <div className="permission-checkboxes">
      {PERMISSION_GROUPS.map((group) => (
        <div key={group.title} className="permission-group">
          <div className="group-header">
            <label className="admin-checkbox">
              <input
                type="checkbox"
                checked={isGroupChecked(group)}
                ref={(el) => {
                  if (el) el.indeterminate = isGroupPartial(group);
                }}
                onChange={(e) => handleToggleGroup(group, e.target.checked)}
                disabled={disabled}
              />
              <span className="checkbox-label group-title">{group.title}</span>
            </label>
          </div>
          <div className="group-permissions">
            {group.permissions.map((perm) => (
              <label key={perm.key} className="admin-checkbox">
                <input
                  type="checkbox"
                  checked={permissions[perm.key] === true}
                  onChange={() => handleToggle(perm.key)}
                  disabled={disabled}
                />
                <span className="checkbox-label">{perm.label}</span>
              </label>
            ))}
          </div>
        </div>
      ))}

      <style jsx>{`
        .permission-checkboxes {
          display: grid;
          grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
          gap: 24px;
        }

        .permission-group {
          background-color: var(--admin-bg);
          border-radius: var(--admin-radius);
          padding: 16px;
        }

        .group-header {
          padding-bottom: 12px;
          margin-bottom: 12px;
          border-bottom: 1px solid var(--admin-border);
        }

        .group-title {
          font-weight: 600 !important;
          color: var(--admin-text-primary) !important;
        }

        .group-permissions {
          display: flex;
          flex-direction: column;
          gap: 10px;
        }
      `}</style>
    </div>
  );
}
