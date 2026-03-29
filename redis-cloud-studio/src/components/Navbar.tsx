import { Link, useNavigate, useLocation } from "react-router-dom";
import { Database, LogOut, Activity } from "lucide-react";
import { toast } from "sonner";

export default function Navbar() {
  const navigate = useNavigate();
  const location = useLocation();

  const handleLogout = () => {
    localStorage.removeItem("user");
    toast.success("Session terminated");
    navigate("/login");
  };

  const navLinks = [
    { path: "/dashboard", label: "Console" },
    { path: "/monitoring", label: "Monitor" },
  ];

  return (
    <nav className="glass sticky top-0 z-50 border-b border-white/5">
      <div className="container mx-auto px-4 sm:px-6 py-4">
        <div className="flex items-center justify-between">
          {/* Logo */}
          <Link to="/dashboard" className="flex items-center gap-3 group">
            <div className="relative">
              <div className="w-8 h-8 rounded-lg bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center transition-all group-hover:border-emerald-500/60 group-hover:bg-emerald-500/15">
                <Database className="w-4 h-4 text-emerald-400" />
              </div>
              <div className="absolute -top-0.5 -right-0.5 w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
            </div>
            <div>
              <span className="text-sm font-bold tracking-widest text-white mono uppercase">
                Mini<span className="text-emerald-400">Redis</span>
              </span>
            </div>
          </Link>

          {/* Nav Links */}
          <div className="hidden md:flex items-center gap-1">
            {navLinks.map((link) => {
              const isActive = location.pathname === link.path;
              return (
                <Link
                  key={link.path}
                  to={link.path}
                  className={`relative px-4 py-2 text-xs font-medium mono tracking-wider uppercase transition-all duration-200 rounded ${
                    isActive
                      ? "text-emerald-400 bg-emerald-500/10"
                      : "text-white/40 hover:text-white/80 hover:bg-white/5"
                  }`}
                >
                  {isActive && (
                    <span className="absolute bottom-0 left-1/2 -translate-x-1/2 w-4 h-px bg-emerald-400" />
                  )}
                  {link.label}
                </Link>
              );
            })}
          </div>

          {/* Actions */}
          <div className="flex items-center gap-3">
            <div className="hidden sm:flex items-center gap-2 px-3 py-1.5 rounded bg-emerald-500/5 border border-emerald-500/15">
              <Activity className="w-3 h-3 text-emerald-400" />
              <span className="text-xs mono text-emerald-400/80 tracking-wider">ONLINE</span>
            </div>
            <button
              onClick={handleLogout}
              className="flex items-center gap-2 px-3 py-1.5 rounded border border-white/8 bg-white/3 hover:bg-white/8 hover:border-white/15 transition-all text-white/50 hover:text-white/80"
            >
              <LogOut className="w-3.5 h-3.5" />
              <span className="hidden sm:inline text-xs mono tracking-wider">EXIT</span>
            </button>
          </div>
        </div>
      </div>
    </nav>
  );
}
