"use client";
import React from "react";

// Error boundary component to catch runtime errors in admin dashboard
// Prevents entire app crash and shows friendly error message
export default class AdminErrorBoundary extends React.Component {
  constructor(props) {
    super(props);
    this.state = { hasError: false, error: null, errorInfo: null };
  }

  static getDerivedStateFromError(error) {
    return { hasError: true, error };
  }

  componentDidCatch(error, errorInfo) {
    // Log error for debugging
    console.error("Admin Dashboard Error:", error);
    console.error("Component Stack:", errorInfo.componentStack);

    this.setState({ errorInfo });

    // In production, you might want to send this to an error reporting service
    if (process.env.NODE_ENV === "production") {
      // Example: sendToErrorReporting(error, errorInfo);
    }
  }

  handleRetry = () => {
    this.setState({ hasError: false, error: null, errorInfo: null });
  };

  handleReload = () => {
    window.location.reload();
  };

  render() {
    if (this.state.hasError) {
      return (
        <div className="admin-error-boundary">
          <div className="error-content">
            <div className="error-icon">
              <svg
                width="64"
                height="64"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="1.5"
                strokeLinecap="round"
                strokeLinejoin="round"
              >
                <circle cx="12" cy="12" r="10" />
                <line x1="12" y1="8" x2="12" y2="12" />
                <line x1="12" y1="16" x2="12.01" y2="16" />
              </svg>
            </div>
            <h2 className="error-title">Something went wrong</h2>
            <p className="error-message">
              An unexpected error occurred. Please try again or reload the page.
            </p>
            {process.env.NODE_ENV !== "production" && this.state.error && (
              <div className="error-details">
                <details>
                  <summary>Error Details (Development Only)</summary>
                  <pre>{this.state.error.toString()}</pre>
                  {this.state.errorInfo && (
                    <pre>{this.state.errorInfo.componentStack}</pre>
                  )}
                </details>
              </div>
            )}
            <div className="error-actions">
              <button onClick={this.handleRetry} className="admin-btn btn-primary">
                Try Again
              </button>
              <button onClick={this.handleReload} className="admin-btn btn-secondary">
                Reload Page
              </button>
            </div>
          </div>

          <style jsx>{`
            .admin-error-boundary {
              display: flex;
              align-items: center;
              justify-content: center;
              min-height: 400px;
              padding: 40px;
            }

            .error-content {
              text-align: center;
              max-width: 480px;
            }

            .error-icon {
              display: inline-flex;
              align-items: center;
              justify-content: center;
              width: 80px;
              height: 80px;
              border-radius: 50%;
              background-color: var(--admin-danger-light, #fef2f2);
              color: var(--admin-danger, #ef4444);
              margin-bottom: 24px;
            }

            .error-title {
              font-size: 24px;
              font-weight: 700;
              color: var(--admin-text-primary, #111827);
              margin: 0 0 12px;
            }

            .error-message {
              font-size: 14px;
              color: var(--admin-text-secondary, #6b7280);
              margin: 0 0 24px;
              line-height: 1.5;
            }

            .error-details {
              margin-bottom: 24px;
              text-align: left;
            }

            .error-details details {
              background-color: var(--admin-bg, #f9fafb);
              border-radius: 8px;
              padding: 12px 16px;
            }

            .error-details summary {
              cursor: pointer;
              font-size: 12px;
              font-weight: 500;
              color: var(--admin-text-secondary, #6b7280);
            }

            .error-details pre {
              margin: 12px 0 0;
              font-size: 11px;
              color: var(--admin-danger, #ef4444);
              white-space: pre-wrap;
              word-break: break-word;
              max-height: 200px;
              overflow-y: auto;
            }

            .error-actions {
              display: flex;
              gap: 12px;
              justify-content: center;
            }
          `}</style>
        </div>
      );
    }

    return this.props.children;
  }
}
