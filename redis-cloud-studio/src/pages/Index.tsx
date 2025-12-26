import { useState, useEffect } from "react";
import { Link } from "react-router-dom";
import { Button } from "@/components/ui/button";
import { Database, Zap, Shield, Code2, ArrowRight, Sparkles } from "lucide-react";

const Index = () => {
  return (
    <div className="min-h-screen relative overflow-hidden bg-[#0a0e27]">
      {/* 3D Grid Background */}
      <div className="absolute inset-0 grid-3d opacity-30" style={{ 
        transform: 'perspective(500px) rotateX(60deg)',
        transformOrigin: 'center bottom'
      }} />

      {/* Vertical Neon Lines */}
      <div className="absolute inset-0 overflow-hidden pointer-events-none">
        {[10, 20, 30, 70, 80, 90].map((position, i) => (
          <div
            key={i}
            className="absolute top-0 h-full w-[2px] bg-gradient-to-b from-transparent via-cyan-400 to-transparent"
            style={{
              left: `${position}%`,
              opacity: 0.3 + Math.random() * 0.3,
              animation: `pulse ${2 + Math.random() * 2}s ease-in-out infinite`,
              animationDelay: `${Math.random() * 2}s`
            }}
          />
        ))}
      </div>

      {/* Bottom Glow */}
      <div className="absolute bottom-0 left-0 right-0 h-64 bg-gradient-to-t from-cyan-500/30 via-cyan-400/10 to-transparent pointer-events-none" />
      <div className="absolute bottom-0 left-0 right-0 h-[2px] bg-gradient-to-r from-transparent via-cyan-400 to-transparent shadow-[0_0_50px_rgba(34,211,238,1)]" />

      {/* Scan Effect */}
      <div className="absolute inset-0 scan-effect pointer-events-none" />

      {/* Floating Particles */}
      {[...Array(20)].map((_, i) => (
        <div
          key={i}
          className="absolute w-1 h-1 bg-cyan-400 rounded-full"
          style={{
            top: `${Math.random() * 100}%`,
            left: `${Math.random() * 100}%`,
            animation: `float-3d ${5 + Math.random() * 5}s ease-in-out infinite`,
            animationDelay: `${Math.random() * 3}s`,
            opacity: 0.3 + Math.random() * 0.5
          }}
        />
      ))}

      {/* Navbar */}
      <nav className="relative z-50 flex items-center justify-between p-6 backdrop-blur-sm">
        <div className="flex items-center gap-3">
          <div className="relative">
            <div className="absolute inset-0 bg-cyan-400 blur-xl opacity-60 animate-pulse" />
            <Database className="relative h-8 w-8 text-cyan-400" />
          </div>
          <span className="text-3xl font-[Orbitron] font-bold text-white tracking-wider drop-shadow-[0_0_20px_rgba(34,211,238,0.8)]">
            MiniRedis
          </span>
        </div>
        
        <div className="flex items-center gap-6">
          <a href="#features" className="text-cyan-100 hover:text-cyan-400 transition-colors font-medium">
            Features
          </a>
          <Link to="/login">
            <Button variant="outline" className="border-cyan-400/50 text-cyan-400 hover:bg-cyan-400/10 hover:border-cyan-400">
              Sign In
            </Button>
          </Link>
        </div>
      </nav>

      {/* Hero Section */}
      <div className="relative z-10 container mx-auto px-4 pt-20 pb-32">
        <div className="text-center max-w-5xl mx-auto">
          {/* Floating MiniRedis Logo */}
          <div className="inline-block mb-8 animate-float-3d">
            <div className="relative p-6 bg-cyan-500/5 rounded-3xl border border-cyan-400/30 backdrop-blur-xl">
              <Database className="h-24 w-24 text-cyan-400 drop-shadow-[0_0_30px_rgba(34,211,238,0.8)]" />
              <div className="absolute inset-0 bg-cyan-400/20 rounded-3xl blur-2xl" />
            </div>
          </div>

          <h1 className="text-7xl md:text-8xl font-[Orbitron] font-black text-white mb-6 leading-tight">
            <span className="inline-block animate-pulse">Mini</span>
            <span className="text-transparent bg-clip-text bg-gradient-to-r from-cyan-400 via-blue-400 to-cyan-400 drop-shadow-[0_0_40px_rgba(34,211,238,1)]">
              Redis
            </span>
          </h1>

          <p className="text-2xl md:text-3xl text-cyan-100/80 mb-4 font-light">
            Redis Hosting Made Simple
          </p>

          <p className="text-lg text-cyan-100/60 mb-12 max-w-2xl mx-auto">
            Get your Redis instance in 30 seconds. No configuration, no hassle.
            <br />Just fast, reliable in-memory storage.
          </p>

          <div className="flex gap-6 justify-center">
            <Link to="/register">
              <Button 
                size="lg"
                className="h-16 px-10 bg-gradient-to-r from-cyan-500 to-blue-500 hover:from-cyan-400 hover:to-blue-400 text-white font-bold text-lg rounded-2xl shadow-[0_0_40px_rgba(34,211,238,0.6)] hover:shadow-[0_0_60px_rgba(34,211,238,0.8)] transition-all duration-300 transform hover:scale-105 border-0"
              >
                <Sparkles className="mr-2 h-6 w-6" />
                Start Free
                <ArrowRight className="ml-2 h-6 w-6" />
              </Button>
            </Link>
          </div>

          {/* Terminal Demo */}
          <div className="mt-20 max-w-3xl mx-auto">
            <div className="relative group">
              <div className="absolute -inset-1 bg-gradient-to-r from-cyan-500 to-blue-500 rounded-2xl opacity-30 group-hover:opacity-50 blur-xl transition duration-1000" />
              <div className="relative bg-slate-950/90 backdrop-blur-xl rounded-2xl p-6 border border-cyan-400/30 font-mono text-left">
                <div className="flex items-center gap-2 mb-4">
                  <div className="w-3 h-3 rounded-full bg-red-500" />
                  <div className="w-3 h-3 rounded-full bg-yellow-500" />
                  <div className="w-3 h-3 rounded-full bg-green-500" />
                  <span className="ml-4 text-cyan-400 text-sm">redis-cli</span>
                </div>
                <div className="space-y-2 text-sm">
                  <div className="flex items-center">
                    <span className="text-cyan-400">&gt;</span>
                    <span className="ml-2 text-green-400">SET</span>
                    <span className="ml-2 text-white">user:1</span>
                    <span className="ml-2 text-yellow-400">"Alice"</span>
                  </div>
                  <div className="text-green-400 ml-4">+OK</div>
                  <div className="flex items-center mt-3">
                    <span className="text-cyan-400">&gt;</span>
                    <span className="ml-2 text-green-400">GET</span>
                    <span className="ml-2 text-white">user:1</span>
                  </div>
                  <div className="text-yellow-400 ml-4">"Alice"</div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Features Section */}
      <div id="features" className="relative z-10 container mx-auto px-4 py-20">
        <h2 className="text-5xl font-[Orbitron] font-bold text-center text-white mb-16">
          Why Choose MiniRedis?
        </h2>
        
        <div className="grid md:grid-cols-3 gap-8 max-w-6xl mx-auto">
          {[
            { icon: Zap, title: "Lightning Fast", desc: "In-memory storage with microsecond latency" },
            { icon: Shield, title: "Secure & Isolated", desc: "Each instance runs in its own secure container" },
            { icon: Code2, title: "Simple API", desc: "Standard Redis protocol, works with any client" }
          ].map((feature, i) => (
            <div key={i} className="relative group h-full">
              <div className="absolute -inset-1 bg-gradient-to-r from-cyan-500 to-blue-500 rounded-2xl opacity-20 group-hover:opacity-40 blur-xl transition duration-500" />
              <div className="relative h-full bg-slate-950/80 backdrop-blur-xl rounded-2xl p-8 border border-cyan-400/30 hover:border-cyan-400/60 transition-all duration-300 flex flex-col">
                <feature.icon className="h-16 w-16 text-cyan-400 mb-6" />
                <h3 className="text-2xl font-[Orbitron] font-bold text-white mb-4">{feature.title}</h3>
                <p className="text-cyan-100/70 text-lg leading-relaxed">{feature.desc}</p>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};

export default Index;
