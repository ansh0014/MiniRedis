import { useState, useEffect } from "react";
import { Link } from "react-router-dom";
import { Button } from "@/components/ui/button";
import { useToast } from "@/hooks/use-toast";
import { Database, ArrowLeft, Moon, Sun } from "lucide-react";

const Login = () => {
  const { toast } = useToast();
  const [isLoading, setIsLoading] = useState(false);
  const [isDark, setIsDark] = useState(true);

  useEffect(() => {
    if (isDark) {
      document.documentElement.classList.add("dark");
    } else {
      document.documentElement.classList.remove("dark");
    }
  }, [isDark]);

  const handleGoogleLogin = () => {
    setIsLoading(true);

    setTimeout(() => {
      toast({
        title: "Welcome back!",
        description: "Successfully signed in with Google.",
      });
      setIsLoading(false);
      window.location.href = "/dashboard";
    }, 2000);
  };

  return (
    <div className="min-h-screen relative overflow-hidden bg-[#0a0e27]">
      {/* 3D Background */}
      <div className="absolute inset-0 grid-3d opacity-30" style={{ 
        transform: 'perspective(500px) rotateX(60deg)',
        transformOrigin: 'center bottom'
      }} />

      {/* Vertical Neon Lines */}
      <div className="absolute inset-0 overflow-hidden pointer-events-none">
        {[15, 25, 75, 85].map((position, i) => (
          <div
            key={i}
            className="absolute top-0 h-full w-[2px] bg-gradient-to-b from-transparent via-cyan-400 to-transparent opacity-30"
            style={{
              left: `${position}%`,
              animation: `pulse ${2 + i}s ease-in-out infinite`
            }}
          />
        ))}
      </div>

      {/* Bottom Glow */}
      <div className="absolute bottom-0 left-0 right-0 h-64 bg-gradient-to-t from-cyan-500/30 via-cyan-400/10 to-transparent" />
      <div className="absolute bottom-0 left-0 right-0 h-[2px] bg-gradient-to-r from-transparent via-cyan-400 to-transparent shadow-[0_0_50px_rgba(34,211,238,1)]" />

      {/* Scan Effect */}
      <div className="absolute inset-0 scan-effect pointer-events-none" />

      {/* Theme Toggle */}
      <button
        onClick={() => setIsDark(!isDark)}
        className="fixed top-6 right-6 z-50 p-3 rounded-full bg-slate-900/80 backdrop-blur-xl border border-cyan-400/30 hover:border-cyan-400 transition-all duration-300 hover:scale-110 hover:shadow-[0_0_20px_rgba(34,211,238,0.5)] group"
      >
        {isDark ? (
          <Sun className="h-6 w-6 text-cyan-400 group-hover:rotate-180 transition-transform duration-500" />
        ) : (
          <Moon className="h-6 w-6 text-cyan-400 group-hover:rotate-180 transition-transform duration-500" />
        )}
      </button>

      {/* Back to Home */}
      <Link to="/" className="fixed top-6 left-6 z-50 flex items-center gap-2 text-cyan-400 hover:text-cyan-300 transition-colors group">
        <ArrowLeft className="h-5 w-5 group-hover:-translate-x-1 transition-transform" />
        <span className="font-medium">Back to Home</span>
      </Link>

      {/* Main Content */}
      <div className="relative z-10 flex items-center justify-center min-h-screen p-4">
        <div className="w-full max-w-md">
          {/* Card */}
          <div className="relative group">
            {/* Glow Effect */}
            <div className="absolute -inset-1 bg-gradient-to-r from-cyan-500 via-blue-500 to-cyan-500 rounded-3xl opacity-30 group-hover:opacity-50 blur-2xl transition duration-1000 animate-pulse" />
            
            <div className="relative bg-slate-950/90 backdrop-blur-2xl rounded-3xl shadow-2xl p-12 border border-cyan-400/30">
              {/* Logo */}
              <div className="flex justify-center mb-8">
                <div className="relative">
                  <div className="absolute inset-0 bg-cyan-400 blur-2xl opacity-60 animate-pulse" />
                  <Database className="relative h-20 w-20 text-cyan-400" />
                </div>
              </div>

              {/* Title */}
              <h1 className="text-5xl font-[Orbitron] font-black text-center text-white mb-3 drop-shadow-[0_0_20px_rgba(34,211,238,0.8)]">
                Login
              </h1>
              <p className="text-center text-cyan-100/70 mb-12 text-lg">
                Sign in to access your Redis instance
              </p>

              {/* Google Sign In Button - ONLY THIS */}
              <Button
                onClick={handleGoogleLogin}
                disabled={isLoading}
                className="w-full h-16 bg-white hover:bg-gray-50 text-gray-800 font-bold text-lg rounded-2xl shadow-[0_0_30px_rgba(255,255,255,0.2)] hover:shadow-[0_0_40px_rgba(255,255,255,0.3)] transition-all duration-300 transform hover:scale-105 border-0"
              >
                {isLoading ? (
                  <div className="flex items-center gap-3">
                    <div className="w-6 h-6 border-4 border-gray-300 border-t-cyan-500 rounded-full animate-spin" />
                    <span>Connecting to Google...</span>
                  </div>
                ) : (
                  <div className="flex items-center gap-4">
                    <svg className="w-7 h-7" viewBox="0 0 24 24">
                      <path fill="#4285F4" d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z" />
                      <path fill="#34A853" d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z" />
                      <path fill="#FBBC05" d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.07H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.93l2.85-2.22.81-.62z" />
                      <path fill="#EA4335" d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.07l3.66 2.84c.87-2.6 3.3-4.53 6.16-4.53z" />
                    </svg>
                    <span>Sign in with Google</span>
                  </div>
                )}
              </Button>

              {/* Register Link */}
              <p className="mt-8 text-center text-cyan-100/60">
                Don't have an account?{" "}
                <Link to="/register" className="font-bold text-cyan-400 hover:text-cyan-300 transition-colors">
                  Create one now
                </Link>
              </p>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Login;
