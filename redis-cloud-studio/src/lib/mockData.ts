export const mockUser = {
  name: "John Doe",
  email: "john@example.com",
  avatar: "JD"
};


export const mockMetrics = {
  memoryUsed: 32,
  memoryTotal: 40,
  totalKeys: 1247,
  commandsToday: 5432,
  uptime: "7 days 3 hours"
};

export const mockCommands = [
  { id: 1, timestamp: "2024-12-26 10:30:15", command: "SET user:1 Alice", time: "2ms", status: "success" as const },
  { id: 2, timestamp: "2024-12-26 10:30:20", command: "GET user:1", time: "1ms", status: "success" as const },
  { id: 3, timestamp: "2024-12-26 10:31:05", command: "HSET user:1 name Alice age 30", time: "3ms", status: "success" as const },
  { id: 4, timestamp: "2024-12-26 10:31:10", command: "HGETALL user:1", time: "1ms", status: "success" as const },
  { id: 5, timestamp: "2024-12-26 10:32:00", command: "INVALID_CMD", time: "0ms", status: "error" as const },
  { id: 6, timestamp: "2024-12-26 10:32:30", command: "LPUSH mylist item1", time: "2ms", status: "success" as const },
  { id: 7, timestamp: "2024-12-26 10:33:00", command: "LRANGE mylist 0 -1", time: "1ms", status: "success" as const },
  { id: 8, timestamp: "2024-12-26 10:33:30", command: "DEL mylist", time: "1ms", status: "success" as const },
];

export const pricingPlans = [
  {
    name: "Free",
    price: "$0",
    period: "forever",
    description: "Perfect for learning and small projects",
    features: [
      "40 MB memory",
      "1 Redis instance",
      "Community support",
      "Basic monitoring",
      "1,000 commands/day"
    ],
    cta: "Start Free",
    popular: false
  },
  {
    name: "Pro",
    price: "$19",
    period: "per month",
    description: "For growing applications and teams",
    features: [
      "1 GB memory",
      "5 Redis instances",
      "Priority support",
      "Advanced monitoring",
      "Unlimited commands",
      "Daily backups",
      "Custom alerts"
    ],
    cta: "Start Pro",
    popular: true
  },
  {
    name: "Enterprise",
    price: "$99",
    period: "per month",
    description: "For large-scale production workloads",
    features: [
      "10 GB memory",
      "Unlimited instances",
      "24/7 dedicated support",
      "Real-time monitoring",
      "Unlimited commands",
      "Hourly backups",
      "Custom SLA",
      "VPC peering"
    ],
    cta: "Contact Sales",
    popular: false
  }
];

export const features = [
  {
    icon: "Zap",
    title: "Instant Setup",
    description: "Deploy your Redis instance in under 30 seconds. No configuration needed."
  },
  {
    icon: "Shield",
    title: "Secure & Isolated",
    description: "Each instance runs in its own isolated environment with encryption at rest."
  },
  {
    icon: "Gift",
    title: "Free 40MB Tier",
    description: "Start for free with 40MB of memory. No credit card required."
  },
  {
    icon: "Activity",
    title: "Real-time Monitoring",
    description: "Track memory usage, commands, and performance with live dashboards."
  }
];
