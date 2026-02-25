"use client";
import { useRef } from "react";
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  BarElement,
  Title,
  Tooltip,
  Legend,
} from "chart.js";
import { Bar } from "react-chartjs-2";

ChartJS.register(
  CategoryScale,
  LinearScale,
  BarElement,
  Title,
  Tooltip,
  Legend
);

export default function BarChart({
  labels = [],
  datasets = [],
  title,
  showLegend = true,
  showGrid = true,
  horizontal = false,
  stacked = false,
  borderRadius = 6,
  yAxisPrefix = "",
  yAxisSuffix = "",
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
  ];

  const data = {
    labels,
    datasets: datasets.map((dataset, index) => ({
      label: dataset.label,
      data: dataset.data,
      backgroundColor: dataset.backgroundColor || defaultColors[index % defaultColors.length],
      borderRadius: borderRadius,
      borderSkipped: false,
      maxBarThickness: 50,
    })),
  };

  const options = {
    responsive: true,
    maintainAspectRatio: false,
    indexAxis: horizontal ? "y" : "x",
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
          pointStyle: "rectRounded",
          boxWidth: 10,
          boxHeight: 10,
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
        boxWidth: 10,
        boxHeight: 10,
        boxPadding: 4,
        usePointStyle: true,
        callbacks: {
          label: function (context) {
            let label = context.dataset.label || "";
            if (label) {
              label += ": ";
            }
            if (context.parsed.y !== null) {
              const value = horizontal ? context.parsed.x : context.parsed.y;
              label += yAxisPrefix + value.toLocaleString() + yAxisSuffix;
            }
            return label;
          },
        },
      },
    },
    scales: {
      x: {
        stacked: stacked,
        grid: {
          display: horizontal ? showGrid : false,
          color: "rgba(0, 0, 0, 0.05)",
          drawBorder: false,
        },
        ticks: {
          font: {
            size: 11,
          },
          color: "#9ca3af",
          callback: horizontal
            ? function (value) {
                return yAxisPrefix + value.toLocaleString() + yAxisSuffix;
              }
            : undefined,
        },
        border: {
          display: false,
        },
      },
      y: {
        stacked: stacked,
        grid: {
          display: horizontal ? false : showGrid,
          color: "rgba(0, 0, 0, 0.05)",
          drawBorder: false,
        },
        ticks: {
          font: {
            size: 11,
          },
          color: "#9ca3af",
          callback: horizontal
            ? undefined
            : function (value) {
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
      <Bar ref={chartRef} data={data} options={options} />
    </div>
  );
}
