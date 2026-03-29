import { useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { Database, Zap, Shield, Globe, ArrowRight, Terminal, Cpu } from "lucide-react";
import { useLenis } from "@/hooks/useLenis";

const TYPED_COMMANDS = [
  { cmd: "redis-cli PING", out: "PONG" },
  { cmd: 'SET user:1000 "John Doe"', out: "OK" },
  { cmd: "GET user:1000", out: '"John Doe"' },
  { cmd: "INCR page_views", out: "(integer) 1337" },
  { cmd: 'HSET user:profile name "Alice" age 28', out: "(integer) 2" },
  { cmd: "EXPIRE session:xyz 3600", out: "(integer) 1" },
  { cmd: "TTL session:xyz", out: "(integer) 3599" },
];

export default function Index() {
  useLenis();
  const [visibleLines, setVisibleLines] = useState(0);
  const [isLoaded, setIsLoaded] = useState(false);

  useEffect(() => {
    const t = setTimeout(() => setIsLoaded(true), 100);
    return () => clearTimeout(t);
  }, []);

  useEffect(() => {
    if (!isLoaded) return;
    const timer = setInterval(() => {
      setVisibleLines((prev) => {
        if (prev >= TYPED_COMMANDS.length) { clearInterval(timer); return prev; }
        return prev + 1;
      });
    }, 600);
    return () => clearInterval(timer);
  }, [isLoaded]);

  const features = [
    { icon: <Database className="w-6 h-6" />, title: "Cloud Instances", desc: "Deploy Redis nodes in seconds. Isolated, persistent, production-ready.", tag: "INFRA" },
    { icon: <Zap className="w-6 h-6" />, title: "Sub-ms Latency", desc: "Optimized for speed at every layer. Zero cold starts, always hot.", tag: "PERF" },
    { icon: <Shield className="w-6 h-6" />, title: "Tenant Isolation", desc: "Hard boundaries per tenant. Memory limits enforced at kernel level.", tag: "SEC" },
    { icon: <Globe className="w-6 h-6" />, title: "Live Monitoring", desc: "Real-time metrics. Memory, keys, clients — all in one pane.", tag: "OPS" },
  ];

  return (
    <div className="min-h-screen hero-bg overflow-hidden">
      {/* NAV */}
      <nav className="glass sticky top-0 z-50 border-b border-white/5">
        <div className="container mx-auto px-6 py-4">
          <div className="flex items-center justify-between">
            <Link to="/" className="flex items-center gap-3 group">
              <div className="relative">
                <div className="w-8 h-8 rounded-lg bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center transition-all group-hover:border-emerald-500/60">
                  <Database className="w-4 h-4 text-emerald-400" />
                </div>
                <div className="absolute -top-0.5 -right-0.5 w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
              </div>
              <span className="text-sm font-bold tracking-widest text-white mono uppercase">
                Mini<span className="text-emerald-400">Redis</span>
              </span>
            </Link>

            <Link to="/login">
              <button className="btn-solid-neon flex items-center gap-2 px-5 py-2 rounded-md">
                Launch Console
                <ArrowRight className="w-3.5 h-3.5" />
              </button>
            </Link>
          </div>
        </div>
      </nav>

      {/* HERO */}
      <section className="container mx-auto px-6 py-24 lg:py-36">
        <div className="max-w-7xl mx-auto grid lg:grid-cols-2 gap-20 items-center">

          {/* Left copy */}
          <div className={`space-y-8 transition-all duration-1000 ${isLoaded ? 'opacity-100 translate-y-0' : 'opacity-0 translate-y-8'}`}>
            <div className="inline-flex items-center gap-2 px-3 py-1.5 rounded-full bg-emerald-500/8 border border-emerald-500/20 animate-fade-in">
              <Cpu className="w-3 h-3 text-emerald-400" />
              <span className="label-tag">Cloud-native Redis / v2.0</span>
            </div>

            <h1 className="text-5xl sm:text-6xl lg:text-7xl font-extrabold leading-[0.95] tracking-tight">
              <span className="text-white/90">Redis.</span>
              <br />
              <span className="gradient-text">Redefined.</span>
              <br />
              <span className="text-white/40 text-3xl sm:text-4xl lg:text-5xl font-semibold">In the cloud.</span>
            </h1>

            <p className="text-base text-white/50 max-w-md leading-relaxed font-medium">
              Multi-tenant Redis infrastructure with real-time monitoring, 
              per-instance isolation, and a built-in CLI. Ship faster.
            </p>

            <div className="flex flex-col sm:flex-row gap-3 animate-fade-up animate-delay-300">
              <Link to="/login">
                <button className="btn-solid-neon flex items-center gap-2 px-7 py-3.5 rounded-md text-sm w-full sm:w-auto justify-center">
                  <Terminal className="w-4 h-4" />
                  Start Building
                </button>
              </Link>
              <button className="btn-neon px-7 py-3.5 rounded-md text-sm">
                Read the Docs
              </button>
            </div>

            {/* Stats */}
            <div className="grid grid-cols-3 gap-4 pt-4 animate-fade-up animate-delay-500">
              {[
                { val: "<1ms", label: "Latency" },
                { val: "99.9%", label: "Uptime" },
                { val: "∞", label: "Instances" },
              ].map((s) => (
                <div key={s.label} className="space-y-1">
                  <div className="text-2xl font-bold gradient-text">{s.val}</div>
                  <div className="label-tag">{s.label}</div>
                </div>
              ))}
            </div>
          </div>

          {/* Right terminal */}
          <div className={`transition-all duration-1000 delay-300 ${isLoaded ? 'opacity-100 translate-y-0' : 'opacity-0 translate-y-8'}`}>
            <div className="terminal-window rounded-xl overflow-hidden neon-glow">
              <div className="terminal-header">
                <div className="terminal-dot bg-red-500/90" />
                <div className="terminal-dot bg-amber-500/90" />
                <div className="terminal-dot bg-emerald-500/90" />
                <div className="flex-1 flex items-center justify-between mx-3">
                  <span className="text-xs text-white/30 mono">redis-cli — miniredis</span>
                  <div className="flex items-center gap-1.5">
                    <div className="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse" />
                    <span className="text-xs mono text-emerald-400/60">CONNECTED</span>
                  </div>
                </div>
              </div>
              <div className="terminal-content min-h-[340px]">
                {TYPED_COMMANDS.slice(0, visibleLines).map((item, i) => (
                  <div key={i} className="mb-1">
                    <div className="text-emerald-400/60">
                      <span className="text-emerald-400/40">$ </span>
                      <span className="text-emerald-300">{item.cmd}</span>
                    </div>
                    <div className="text-emerald-400/80 pl-3">{item.out}</div>
                  </div>
                ))}
                {visibleLines < TYPED_COMMANDS.length && (
                  <span className="text-emerald-400/40 animate-pulse">$ <span className="inline-block w-2 h-4 bg-emerald-400/40 -mb-1" /></span>
                )}
              </div>
            </div>
          </div>
        </div>
      </section>

      {/* DIVIDER */}
      <div className="container mx-auto px-6">
        <div className="h-px bg-gradient-to-r from-transparent via-emerald-500/20 to-transparent" />
      </div>

      {/* FEATURES */}
      <section className="container mx-auto px-6 py-24">
        <div className="max-w-7xl mx-auto">
          <div className="mb-16 space-y-3">
            <span className="label-tag">Platform Features</span>
            <h2 className="text-4xl sm:text-5xl font-extrabold text-white/90 tracking-tight">
              Everything you need.<br />
              <span className="text-white/35">Nothing you don't.</span>
            </h2>
          </div>

          <div className="grid sm:grid-cols-2 lg:grid-cols-4 gap-5">
            {features.map((f, i) => (
              <div key={i} className={`feature-card glass-card p-6 rounded-xl space-y-4 animate-fade-up`} style={{ animationDelay: `${i * 0.1}s` }}>
                <div className="flex items-start justify-between">
                  <div className="p-2.5 rounded-lg bg-emerald-500/10 border border-emerald-500/20 text-emerald-400">
                    {f.icon}
                  </div>
                  <span className="label-tag">{f.tag}</span>
                </div>
                <div>
                  <h3 className="text-base font-bold text-white/90 mb-2">{f.title}</h3>
                  <p className="text-sm text-white/40 leading-relaxed">{f.desc}</p>
                </div>
              </div>
            ))}
          </div>
        </div>
      </section>

      {/* CTA */}
      <section className="container mx-auto px-6 py-24">
        <div className="max-w-3xl mx-auto">
          <div className="glass-card rounded-2xl p-12 sm:p-16 text-center space-y-6 relative overflow-hidden neon-border">
            <div className="absolute inset-0 bg-gradient-to-br from-emerald-500/5 via-transparent to-cyan-500/5" />
            <div className="relative">
              <span className="label-tag mb-4 block">Get Started Today</span>
              <h2 className="text-4xl sm:text-5xl font-extrabold text-white/90 mb-4 tracking-tight">
                Your Redis.<br />
                <span className="gradient-text">Your control.</span>
              </h2>
              <p className="text-white/40 mb-8 text-sm max-w-md mx-auto">
                Join developers deploying production Redis with full observability in minutes.
              </p>
              <Link to="/login">
                <button className="btn-solid-neon inline-flex items-center gap-2 px-10 py-4 rounded-md text-sm">
                  Launch Console
                  <ArrowRight className="w-4 h-4" />
                </button>
              </Link>
            </div>
          </div>
        </div>
      </section>

      {/* FOOTER */}
      <footer className="glass border-t border-white/5 mt-8">
        <div className="container mx-auto px-6 py-12">
          <div className="grid grid-cols-2 md:grid-cols-4 gap-8 max-w-7xl mx-auto">
            <div className="col-span-2 md:col-span-1 space-y-4">
              <div className="flex items-center gap-2">
                <Database className="w-5 h-5 text-emerald-400" />
                <span className="text-sm font-bold mono tracking-widest text-white/80 uppercase">MiniRedis</span>
              </div>
              <p className="text-xs text-white/30 leading-relaxed">
                Production-grade cloud Redis.<br /> Built for developers.
              </p>
            </div>
            {[
              { title: "Product", links: ["Pricing", "Features", "Docs"] },
              { title: "Company", links: ["About", "Blog", "Careers"] },
              { title: "Legal", links: ["Privacy", "Terms", "Security"] },
            ].map((col) => (
              <div key={col.title}>
                <h4 className="label-tag mb-4">{col.title}</h4>
                <ul className="space-y-2">
                  {col.links.map((l) => (
                    <li key={l}>
                      <a href="#" className="text-xs text-white/30 hover:text-emerald-400 transition-colors mono">
                        {l}
                      </a>
                    </li>
                  ))}
                </ul>
              </div>
            ))}
          </div>
          <div className="border-t border-white/5 mt-12 pt-8 flex items-center justify-between">
            <p className="text-xs text-white/20 mono">© 2025 MiniRedis. MIT License.</p>
            <p className="text-xs text-white/20 mono">v2.0.0</p>
          </div>
        </div>
      </footer>
    </div>
  );
}
