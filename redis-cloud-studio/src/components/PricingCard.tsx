import { Check } from "lucide-react";
import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";
import { Link } from "react-router-dom";

interface PricingCardProps {
  name: string;
  price: string;
  period: string;
  description: string;
  features: string[];
  cta: string;
  popular?: boolean;
}

const PricingCard = ({ name, price, period, description, features, cta, popular }: PricingCardProps) => {
  return (
    <div className={cn(
      "relative bg-card rounded-xl border p-6 shadow-sm transition-all duration-300 hover:shadow-lg flex flex-col",
      popular ? "border-primary shadow-glow scale-105" : "border-border hover:border-primary/50"
    )}>
      {popular && (
        <div className="absolute -top-3 left-1/2 -translate-x-1/2">
          <span className="gradient-bg text-primary-foreground text-xs font-medium px-3 py-1 rounded-full">
            Most Popular
          </span>
        </div>
      )}

      <div className="text-center mb-6">
        <h3 className="text-xl font-semibold text-foreground mb-2">{name}</h3>
        <div className="flex items-baseline justify-center gap-1">
          <span className="text-4xl font-bold text-foreground">{price}</span>
          <span className="text-muted-foreground">/{period}</span>
        </div>
        <p className="text-sm text-muted-foreground mt-2">{description}</p>
      </div>

      <ul className="space-y-3 mb-8 flex-grow">
        {features.map((feature, index) => (
          <li key={index} className="flex items-start gap-3">
            <Check className="h-5 w-5 text-success flex-shrink-0 mt-0.5" />
            <span className="text-sm text-muted-foreground">{feature}</span>
          </li>
        ))}
      </ul>

      <Link to="/login" className="mt-auto">
        <Button
          variant={popular ? "gradient" : "outline"}
          className="w-full"
          size="lg"
        >
          {cta}
        </Button>
      </Link>
    </div>
  );
};

export default PricingCard;
