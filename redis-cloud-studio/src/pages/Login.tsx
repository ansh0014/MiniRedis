import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { Database, Loader2, Terminal } from "lucide-react";
import { signInWithGoogle } from "@/lib/firebase";
import { toast } from "sonner";
import { useLenis } from "@/hooks/useLenis";

export default function Login() {
  useLenis();
  const navigate = useNavigate();
  const [loading, setLoading] = useState(false);

  const handleGoogleLogin = async () => {
    setLoading(true);
    try {
      const userData = await signInWithGoogle();
      const safeData = {
        email: userData.email,
        name: userData.name,
        picture: userData.picture,
        emailVerified: userData.emailVerified,
      };
      localStorage.setItem("user", JSON.stringify(safeData));
      toast.success(`Access granted — welcome, ${userData.name}`);
      setTimeout(() => navigate("/dashboard"), 500);
    } catch {
      toast.error("Authentication failed");
    } finally {
      setLoading(false);
    }
  };


  return (
    <div className="min-h-screen hero-bg flex items-center justify-center p-4 relative overflow-hidden">
      {/* Corner decoration */}
      <div className="absolute top-0 left-0 w-64 h-64 bg-emerald-500/5 rounded-full blur-3xl" />
      <div className="absolute bottom-0 right-0 w-96 h-96 bg-cyan-500/5 rounded-full blur-3xl" />

      {/* Top-left brand */}
      <div className="absolute top-6 left-6 flex items-center gap-2">
        <div className="relative">
          <div className="w-7 h-7 rounded-lg bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center">
            <Database className="w-3.5 h-3.5 text-emerald-400" />
          </div>
          <div className="absolute -top-0.5 -right-0.5 w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse" />
        </div>
        <span className="text-xs font-bold tracking-widest text-white/50 mono uppercase">
          Mini<span className="text-emerald-400">Redis</span>
        </span>
      </div>

      {/* Login card */}
      <div className="w-full max-w-sm animate-fade-up">
        <div className="terminal-window rounded-2xl overflow-hidden">
          {/* Terminal header */}
          <div className="terminal-header">
            <div className="terminal-dot bg-red-500/90" />
            <div className="terminal-dot bg-amber-500/90" />
            <div className="terminal-dot bg-emerald-500/90" />
           
          </div>

          {/* Body */}
          <div className="p-8 space-y-8">
            {/* Icon cluster */}
            <div className="flex justify-center">
              <div className="relative">
                <div className="w-20 h-20 rounded-2xl bg-emerald-500/10 border border-emerald-500/25 flex items-center justify-center neon-glow">
                  <Database className="w-9 h-9 text-emerald-400" />
                </div>
                <div className="absolute -bottom-1 -right-1 w-7 h-7 rounded-lg bg-cyan-500/15 border border-cyan-500/30 flex items-center justify-center">
                  <Terminal className="w-3.5 h-3.5 text-cyan-400" />
                </div>
              </div>
            </div>

            {/* Heading */}
            <div className="text-center space-y-2">
           
              <h1 className="text-2xl font-extrabold text-white/90 tracking-tight">
                Sign In
              </h1>
              <p className="text-xs text-white/35 mono">
                
              </p>
            </div>

            {/* CLI-style divider */}
            <div className="mono text-xs text-white/15 text-center">
         
            </div>

            {/* Google Button */}
            <button
              id="google-signin-btn"
              onClick={handleGoogleLogin}
              disabled={loading}
              className="w-full h-12 rounded-lg bg-white hover:bg-gray-50 text-gray-900 font-semibold text-sm transition-all flex items-center justify-center gap-3 shadow-lg shadow-black/30 disabled:opacity-60 disabled:cursor-not-allowed">
                
              {loading ? (
                <>
                  <Loader2 className="h-4 w-4 animate-spin" />
                  <span className="mono text-sm">Authenticating...</span>
                </>
              ) : (
                <>
                  <svg className="h-4 w-4 flex-shrink-0" viewBox="0 0 24 24">
                    <path fill="#4285F4" d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z" />
                    <path fill="#34A853" d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z" />
                    <path fill="#FBBC05" d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.07H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.93l2.85-2.22.81-.62z" />
                    <path fill="#EA4335" d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.07l3.66 2.84c.87-2.6 3.3-4.53 6.16-4.53z" />
                  </svg>
                  Continue with Google
                </>
              )}
            </button>

            {/* Terms */}
            <p className="text-center text-xs text-white/20 mono leading-relaxed">
              By continuing you agree to our{" "}
              <a href="#" className="text-emerald-400/60 hover:text-emerald-400 underline underline-offset-2 transition-colors">Terms</a>
              {" "}and{" "}
              <a href="#" className="text-emerald-400/60 hover:text-emerald-400 underline underline-offset-2 transition-colors">Privacy</a>
            </p>
          </div>
        </div>

        <p className="text-center text-xs text-white/20 mono mt-6 tracking-wider">
          REDIS CLOUD
        </p>
      </div>
    </div>
  );
}
