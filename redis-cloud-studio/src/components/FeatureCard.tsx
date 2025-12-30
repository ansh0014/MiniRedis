import { Zap, Shield, Gift, Activity, LucideIcon } from "lucide-react";

const iconMap: Record<string, LucideIcon> = {
  Zap,
  Shield,
  Gift,
  Activity
};

interface FeatureCardProps {
  icon: string;
  title: string;
  description: string;
}

const FeatureCard = ({ icon, title, description }: FeatureCardProps) => {
  const Icon = iconMap[icon] || Zap;

  return (
    <div className="group bg-card rounded-xl border border-border p-6 shadow-sm transition-all duration-300 hover:shadow-lg hover:border-primary/50 hover:-translate-y-1">
      <div className="mb-4 flex h-12 w-12 items-center justify-center rounded-lg gradient-bg group-hover:animate-pulse-glow transition-all">
        <Icon className="h-6 w-6 text-primary-foreground" />
      </div>
      <h3 className="text-lg font-semibold text-foreground mb-2">{title}</h3>
      <p className="text-sm text-muted-foreground leading-relaxed">{description}</p>
    </div>
  );
};

export default FeatureCard;
