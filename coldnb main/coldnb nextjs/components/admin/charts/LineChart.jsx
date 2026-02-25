"use client";
import { useEffect, useRef } from "react";
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler,
} from "chart.js";
import { Line } from "react-chartjs-2";

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler
);

export default function LineChart({
  labels = [],
  datasets = [],
  title,
  showLegend = true,
  showGrid = true,
  fill = false,
  tension = 0.4,
  aspectRatio,
  yAxisPrefix = "",
  yAxisSuffix = "",
}) {
  const chartRef = useRef(null);

  const defaultColors = [
    { border: "#4f46e5", background: "rgba(79, 70, 229, 0.1)" },
    { border: "#10b981", background: "rgba(16, 185, 129, 0.1)" },
    { border: "#f59e0b", background: "rgba(245, 158, 11, 0.1)" },
    { border: "#ef4444", background: "rgba(239, 68, 68, 0.1)" },
  ];

  const data = {
    labels,
    datasets: datasets.map((dataset, index) => ({
      label: dataset.label,
      data: dataset.data,
      borderColor: dataset.borderColor || defaultColors[index % defaultColors.length].border,
      backgroundColor: fill
        ? dataset.backgroundColor || defaultColors[index % defaultColors.length].background
        : "transparent",
      borderWidth: 2,
      fill: fill,
      tension: tension,
      pointRadius: 0,
      pointHoverRadius: 6,
      pointHoverBackgroundColor: dataset.borderColor || defaultColors[index % defaultColors.length].border,
      pointHoverBorderColor: "#fff",
      pointHoverBorderWidth: 2,
    })),
  };

  const options = {
    responsive: true,
    maintainAspectRatio: aspectRatio !== undefined,
    aspectRatio: aspectRatio,
    interaction: {
      mode: "index",
      intersect: false,
    },
    plugins: {
      legend: {
        display: showLegend && datasets.length > 1,
        position: "top",
        align: "end",
        labels: {
          usePointStyle: true,
          pointStyle: "circle",
          boxWidth: 6,
          boxHeight: 6,
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
            let label = context.dataset.label || "";
            if (label) {
              label += ": ";
            }
            if (context.parsed.y !== null) {
              label += yAxisPrefix + context.parsed.y.toLocaleString() + yAxisSuffix;
            }
            return label;
          },
        },
      },
    },
    scales: {
      x: {
        grid: {
          display: showGrid,
          color: "rgba(0, 0, 0, 0.05)",
          drawBorder: false,
        },
        ticks: {
          font: {
            size: 11,
          },
          color: "#9ca3af",
        },
        border: {
          display: false,
        },
      },
      y: {
        grid: {
          display: showGrid,
          color: "rgba(0, 0, 0, 0.05)",
          drawBorder: false,
        },
        ticks: {
          font: {
            size: 11,
          },
          color: "#9ca3af",
          callback: function (value) {
            return yAxisPrefix + value.toLocaleString() + yAxisSuffix;
          },
        },
        border: {
          display: false,
        },
        beginAtZero: true,
      },
    },
  };

  return (
    <div className="chart-container">
      <Line ref={chartRef} data={data} options={options} />
    </div>
  );
}
