import { cn } from "@/lib/utils";
import { LucideIcon } from "lucide-react";

interface MetricCardProps {
  icon: LucideIcon;
  label: string;
  value: string | number;
  subValue?: string;
  progress?: number;
  className?: string;
}

const MetricCard = ({ icon: Icon, label, value, subValue, progress, className }: MetricCardProps) => {
  const getProgressColor = (value: number) => {
    if (value < 60) return "bg-success";
    if (value < 80) return "bg-warning";
    return "bg-destructive";
  };

  return (
    <div className={cn(
      "bg-card rounded-lg border border-border p-4 shadow-sm hover:shadow-md transition-shadow duration-200",
      className
    )}>
      <div className="flex items-start justify-between">
        <div className="space-y-1">
          <p className="text-sm font-medium text-muted-foreground">{label}</p>
          <p className="text-2xl font-semibold text-foreground">{value}</p>
          {subValue && (
            <p className="text-xs text-muted-foreground">{subValue}</p>
          )}
        </div>
        <div className="flex h-10 w-10 items-center justify-center rounded-lg bg-primary/10">
          <Icon className="h-5 w-5 text-primary" />
        </div>
      </div>
      
      {progress !== undefined && (
        <div className="mt-4 space-y-2">
          <div className="h-2 w-full rounded-full bg-muted">
            <div
              className={cn("h-2 rounded-full transition-all duration-500", getProgressColor(progress))}
              style={{ width: `${progress}%` }}
            />
          </div>
          <p className="text-xs text-muted-foreground">{progress}% used</p>
        </div>
      )}
    </div>
  );
};

export default MetricCard;
