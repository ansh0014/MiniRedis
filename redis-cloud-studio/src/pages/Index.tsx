import { useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { Database, Zap, Shield, Globe, ArrowRight } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Card } from "@/components/ui/card";
import ThemeToggle from "@/components/ThemeToggle";
import { useTheme } from "@/components/ThemeProvider";
import { useLenis } from "@/hooks/useLenis";

export default function Index() {
  useLenis();
  const { theme } = useTheme();
  const [isLoaded, setIsLoaded] = useState(false);

  useEffect(() => {
    setIsLoaded(true);
  }, []);

  const features = [
    {
      icon: <Database className="w-8 h-8" />,
      title: "Cloud Redis Instances",
      description: "Deploy Redis instances in seconds",
    },
    {
      icon: <Zap className="w-8 h-8" />,
      title: "Lightning Fast",
      description: "Sub-millisecond latency performance",
    },
    {
      icon: <Shield className="w-8 h-8" />,
      title: "Enterprise Security",
      description: "Bank-level encryption and compliance",
    },
    {
      icon: <Globe className="w-8 h-8" />,
      title: "Global Distribution",
      description: "Deploy across multiple regions",
    },
  ];

  return (
    <div className={`min-h-screen ${theme === 'dark' ? 'hero-gradient-dark' : 'hero-gradient-light'}`}>
      {/* Navbar */}
      <nav className="glass-card border-0 border-b sticky top-0 z-50">
        <div className="container mx-auto px-4 sm:px-6 py-4">
          <div className="flex items-center justify-between">
            <Link to="/" className="flex items-center gap-2">
              <div className="p-2 rounded-xl bg-primary/20">
                <Database className="w-6 h-6 text-primary" />
              </div>
              <span className="text-xl sm:text-2xl font-bold bg-gradient-to-r from-primary to-accent bg-clip-text text-transparent">
                MiniRedis
              </span>
            </Link>

            <div className="flex items-center gap-3">
              <ThemeToggle />
              <Link to="/login">
                <Button className="glass-button font-semibold">
                  <span className="hidden sm:inline">Get Started</span>
                  <ArrowRight className="w-4 h-4 sm:ml-2" />
                </Button>
              </Link>
            </div>
          </div>
        </div>
      </nav>

      {/* Hero */}
      <section className="container mx-auto px-4 sm:px-6 py-16 sm:py-32">
        <div className={`max-w-7xl mx-auto grid lg:grid-cols-2 gap-16 items-center transition-all duration-1000 ${isLoaded ? 'opacity-100 translate-y-0' : 'opacity-0 translate-y-10'}`}>
          <div className="space-y-8 text-center lg:text-left">
            <h1 className="text-5xl sm:text-6xl lg:text-7xl font-bold leading-tight">
              <span className={theme === 'dark' ? 'text-white' : 'text-gray-900'}>
                Beyond The
              </span>
              <br />
              <span className="bg-gradient-to-r from-primary via-accent to-primary bg-clip-text text-transparent">
                Naked Eye
              </span>
            </h1>

            <p className="text-xl sm:text-2xl text-foreground/80 max-w-xl mx-auto lg:mx-0">
              {theme === 'dark' 
                ? 'Experience extraordinary cloud-native Redis instances'
                : 'Explore the galaxy of high-performance caching'
              }
            </p>

            <div className="flex flex-col sm:flex-row gap-4 justify-center lg:justify-start">
              <Link to="/login">
                <Button size="lg" className="w-full sm:w-auto px-8 py-6 text-lg bg-primary hover:bg-primary/90 shadow-2xl shadow-primary/50">
                  Launch Console
                  <ArrowRight className="ml-2 w-5 h-5" />
                </Button>
              </Link>
              <Button size="lg" variant="outline" className="w-full sm:w-auto px-8 py-6 text-lg glass-button">
                Learn More
              </Button>
            </div>
          </div>

          {/* Static Terminal */}
          <div className="terminal-window rounded-xl overflow-hidden max-w-xl mx-auto lg:mx-0">
            <div className="terminal-header">
              <div className="terminal-dot bg-red-500"></div>
              <div className="terminal-dot bg-yellow-500"></div>
              <div className="terminal-dot bg-green-500"></div>
              <span className="ml-2 text-sm text-white/60 font-mono">redis-cli</span>
            </div>
            <div className="terminal-content">
              <pre className="whitespace-pre-wrap">
{`$ redis-cli PING
PONG

$ redis-cli SET user:1000 "John Doe"
OK

$ redis-cli GET user:1000
"John Doe"

$ redis-cli INCR page_views
(integer) 1337

$ redis-cli HSET user:profile name "Alice" age 28
(integer) 2

$ redis-cli LPUSH notifications "New message"
(integer) 1

$ redis-cli EXPIRE session:xyz 3600
(integer) 1

$ redis-cli TTL session:xyz
(integer) 3599`}
              </pre>
            </div>
          </div>
        </div>
      </section>

      {/* Features */}
      <section className="container mx-auto px-4 sm:px-6 py-16 sm:py-24">
        <div className="text-center mb-16">
          <h2 className="text-4xl sm:text-5xl font-bold mb-4">
            {theme === 'dark' ? 'Experience The Extraordinary' : 'Featured Adventures'}
          </h2>
          <p className="text-xl text-foreground/70">
            Production-ready Redis deployments
          </p>
        </div>

        <div className="grid sm:grid-cols-2 lg:grid-cols-4 gap-6 max-w-7xl mx-auto">
          {features.map((feature, index) => (
            <Card
              key={index}
              className="glass-card feature-card p-6 space-y-4"
            >
              <div className="p-3 rounded-xl bg-primary/20 w-fit">
                {feature.icon}
              </div>
              <h3 className="text-xl font-bold">{feature.title}</h3>
              <p className="text-foreground/70">{feature.description}</p>
            </Card>
          ))}
        </div>
      </section>

      {/* CTA */}
      <section className="container mx-auto px-4 sm:px-6 py-16 sm:py-24">
        <Card className="glass-card p-12 sm:p-16 text-center space-y-6 max-w-4xl mx-auto">
          <h2 className="text-4xl sm:text-5xl font-bold">
            Ready to Start Your Journey?
          </h2>
          <p className="text-xl text-foreground/70 max-w-2xl mx-auto">
            Join thousands of developers building with MiniRedis
          </p>
          <Link to="/login">
            <Button size="lg" className="px-12 py-6 text-lg bg-primary hover:bg-primary/90 shadow-2xl shadow-primary/50">
              Get Started Free
              <ArrowRight className="ml-2 w-5 h-5" />
            </Button>
          </Link>
        </Card>
      </section>

      {/* Footer */}
      <footer className="glass-card border-0 border-t mt-24">
        <div className="container mx-auto px-4 sm:px-6 py-12">
          <div className="grid grid-cols-2 md:grid-cols-4 gap-8 max-w-7xl mx-auto">
            <div className="col-span-2 md:col-span-1 space-y-4">
              <div className="flex items-center gap-2">
                <Database className="w-6 h-6 text-primary" />
                <span className="text-xl font-bold">MiniRedis</span>
              </div>
              <p className="text-sm text-foreground/70">
                Production-ready Redis in the cloud
              </p>
            </div>
            
            <div>
              <h4 className="font-semibold mb-4">Product</h4>
              <ul className="space-y-2 text-sm text-foreground/70">
                <li><a href="#" className="hover:text-primary transition-colors">Pricing</a></li>
                <li><a href="#" className="hover:text-primary transition-colors">Features</a></li>
                <li><a href="#" className="hover:text-primary transition-colors">Docs</a></li>
              </ul>
            </div>

            <div>
              <h4 className="font-semibold mb-4">Company</h4>
              <ul className="space-y-2 text-sm text-foreground/70">
                <li><a href="#" className="hover:text-primary transition-colors">About</a></li>
                <li><a href="#" className="hover:text-primary transition-colors">Blog</a></li>
                <li><a href="#" className="hover:text-primary transition-colors">Careers</a></li>
              </ul>
            </div>

            <div>
              <h4 className="font-semibold mb-4">Legal</h4>
              <ul className="space-y-2 text-sm text-foreground/70">
                <li><a href="#" className="hover:text-primary transition-colors">Privacy</a></li>
                <li><a href="#" className="hover:text-primary transition-colors">Terms</a></li>
                <li><a href="#" className="hover:text-primary transition-colors">Security</a></li>
              </ul>
            </div>
          </div>

          <div className="border-t border-white/10 mt-12 pt-8 text-center text-sm text-foreground/60">
            <p>© 2025 MiniRedis. Built for developers.</p>
          </div>
        </div>
      </footer>
    </div>
  );
}
