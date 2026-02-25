"use client";
import { useRef } from "react";
import {
  Chart as ChartJS,
  ArcElement,
  Tooltip,
  Legend,
} from "chart.js";
import { Pie, Doughnut } from "react-chartjs-2";

ChartJS.register(ArcElement, Tooltip, Legend);

export default function PieChart({
  labels = [],
  data = [],
  title,
  showLegend = true,
  legendPosition = "right",
  donut = false,
  cutout = "65%",
  centerText,
  centerValue,
}) {
  const chartRef = useRef(null);

  const defaultColors = [
    "#4f46e5",
    "#10b981",
    "#f59e0b",
    "#ef4444",
    "#8b5cf6",
    "#ec4899",
    "#06b6d4",
    "#84cc16",
    "#f97316",
    "#6366f1",
  ];

  const chartData = {
    labels,
    datasets: [
      {
        data,
        backgroundColor: defaultColors.slice(0, data.length),
        borderWidth: 0,
        hoverOffset: 4,
      },
    ],
  };

  const options = {
    responsive: true,
    maintainAspectRatio: false,
    cutout: donut ? cutout : 0,
    plugins: {
      legend: {
        display: showLegend,
        position: legendPosition,
        labels: {
          usePointStyle: true,
          pointStyle: "circle",
          boxWidth: 8,
          boxHeight: 8,
          padding: 16,
          font: {
            size: 12,
          },
          color: "#6b7280",
        },
      },
      title: {
        display: !!title,
        text: title,
        font: {
          size: 14,
          weight: "600",
        },
        color: "#111827",
        padding: {
          bottom: 16,
        },
      },
      tooltip: {
        backgroundColor: "#1f2937",
        titleColor: "#9ca3af",
        bodyColor: "#fff",
        titleFont: {
          size: 12,
          weight: "normal",
        },
        bodyFont: {
          size: 14,
          weight: "600",
        },
        padding: 12,
        cornerRadius: 8,
        displayColors: true,
        boxWidth: 8,
        boxHeight: 8,
        boxPadding: 4,
        usePointStyle: true,
        callbacks: {
          label: function (context) {
            const label = context.label || "";
            const value = context.parsed;
            const total = context.dataset.data.reduce((a, b) => a + b, 0);
            const percentage = ((value / total) * 100).toFixed(1);
            return `${label}: ${value.toLocaleString()} (${percentage}%)`;
          },
        },
      },
    },
  };

  const ChartComponent = donut ? Doughnut : Pie;

  return (
    <div className="chart-container" style={{ position: "relative" }}>
      <ChartComponent ref={chartRef} data={chartData} options={options} />
      {donut && centerText && (
        <div className="donut-center">
          {centerValue && <div className="center-value">{centerValue}</div>}
          <div className="center-label">{centerText}</div>
        </div>
      )}
    </div>
  );
}
