import { cn } from "@/lib/utils";
import { LucideIcon } from "lucide-react";

interface MetricCardProps {
  title: string;
  value: string;
  change: string;
  trend: "up" | "down";
}

const MetricCard = ({ title, value, change, trend }: MetricCardProps) => {
  const getTrendColor = (trend: "up" | "down") => {
    return trend === "up" ? "text-green-500" : "text-red-500";
  };

  return (
    <div className="bg-card rounded-lg border border-border p-4 shadow-sm hover:shadow-md transition-shadow duration-200">
      <div className="flex items-start justify-between">
        <div className="space-y-1">
          <p className="text-sm font-medium text-muted-foreground">{title}</p>
          <p className="text-2xl font-semibold text-foreground">{value}</p>
          <p className={`text-xs ${getTrendColor(trend)}`}>
            {change} {trend === "up" ? "↑" : "↓"}
          </p>
        </div>
      </div>
    </div>
  );
};

export default MetricCard;
