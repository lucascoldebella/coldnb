"use client";

const CheckIcon = () => (
  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="3" strokeLinecap="round" strokeLinejoin="round">
    <polyline points="20 6 9 17 4 12" />
  </svg>
);

const ClockIcon = () => (
  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <circle cx="12" cy="12" r="10" />
    <polyline points="12 6 12 12 16 14" />
  </svg>
);

const formatDate = (dateString) => {
  return new Date(dateString).toLocaleString("pt-BR", {
    day: "2-digit",
    month: "short",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
  });
};

export default function OrderTimeline({ events = [] }) {
  return (
    <div className="order-timeline">
      {events.map((event, index) => (
        <div
          key={event.id || index}
          className={`timeline-item ${event.completed ? "completed" : ""}`}
        >
          <div className="timeline-marker">
            <div className="marker-dot">
              {event.completed ? <CheckIcon /> : <ClockIcon />}
            </div>
            {index < events.length - 1 && <div className="marker-line" />}
          </div>
          <div className="timeline-content">
            <div className="timeline-title">{event.title}</div>
            {event.description && (
              <div className="timeline-description">{event.description}</div>
            )}
            <div className="timeline-date">{formatDate(event.date)}</div>
          </div>
        </div>
      ))}

      <style jsx>{`
        .order-timeline {
          padding: 0;
        }

        .timeline-item {
          display: flex;
          gap: 16px;
          position: relative;
        }

        .timeline-marker {
          display: flex;
          flex-direction: column;
          align-items: center;
          flex-shrink: 0;
        }

        .marker-dot {
          width: 28px;
          height: 28px;
          border-radius: 50%;
          background-color: var(--admin-bg);
          border: 2px solid var(--admin-border);
          display: flex;
          align-items: center;
          justify-content: center;
          color: var(--admin-text-muted);
          z-index: 1;
        }

        .timeline-item.completed .marker-dot {
          background-color: var(--admin-success);
          border-color: var(--admin-success);
          color: white;
        }

        .marker-line {
          width: 2px;
          flex: 1;
          background-color: var(--admin-border);
          margin: 4px 0;
          min-height: 24px;
        }

        .timeline-item.completed .marker-line {
          background-color: var(--admin-success);
        }

        .timeline-content {
          padding-bottom: 24px;
          flex: 1;
        }

        .timeline-title {
          font-size: 14px;
          font-weight: 600;
          color: var(--admin-text-primary);
        }

        .timeline-description {
          font-size: 13px;
          color: var(--admin-text-secondary);
          margin-top: 4px;
        }

        .timeline-date {
          font-size: 12px;
          color: var(--admin-text-muted);
          margin-top: 4px;
        }
      `}</style>
    </div>
  );
}
