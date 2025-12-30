import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { Button } from "@/components/ui/button";
import { Database, Loader2 } from "lucide-react";
import { signInWithGoogle } from "@/lib/firebase";
import { toast } from "sonner";

const Login = () => {
  const navigate = useNavigate();
  const [loading, setLoading] = useState(false);

  const handleGoogleLogin = async () => {
    setLoading(true);

    try {
      console.log("🔐 Starting Google login...");
      
      const userData = await signInWithGoogle();
      
      // Store only non-sensitive data
      const safeData = {
        email: userData.email,
        name: userData.name,
        picture: userData.picture,
        emailVerified: userData.emailVerified,
      };
      
      localStorage.setItem("user", JSON.stringify(safeData));
      
      console.log("✅ Login successful:", userData);
      
      toast.success(`Welcome, ${userData.name}!`);
      
      setTimeout(() => {
        navigate("/dashboard");
      }, 500);
      
    } catch (err: any) {
      console.error("❌ Login failed:", err);
      
      const errorMessage = err.message || "Failed to sign in. Please try again.";
      toast.error(errorMessage);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen flex items-center justify-center p-4 bg-gradient-to-br from-[#0a192f] via-[#112240] to-[#0a192f]">
      {/* Login Card */}
      <div className="w-full max-w-md bg-[#1e293b]/80 backdrop-blur-xl rounded-3xl p-10 border border-cyan-500/30 shadow-2xl shadow-cyan-500/10">
        {/* Icon */}
        <div className="flex justify-center mb-8">
          <div className="w-24 h-24 rounded-2xl bg-gradient-to-br from-cyan-400 to-cyan-600 flex items-center justify-center shadow-lg shadow-cyan-500/50">
            <Database className="w-14 h-14 text-white" strokeWidth={2} />
          </div>
        </div>

        {/* Title */}
        <div className="text-center mb-10">
          <h1 className="text-4xl font-bold text-white mb-3 tracking-tight">
            Create Account
          </h1>
          <p className="text-gray-400 text-base">
            Get started with MiniRedis Cloud
          </p>
        </div>

        {/* Google Sign Up Button */}
        <Button
          onClick={handleGoogleLogin}
          disabled={loading}
          className="w-full h-14 bg-white hover:bg-gray-50 text-gray-800 rounded-xl font-semibold text-base transition-all duration-200 shadow-lg hover:shadow-xl"
        >
          {loading ? (
            <>
              <Loader2 className="mr-2 h-5 w-5 animate-spin" />
              Signing in...
            </>
          ) : (
            <>
              <svg className="mr-3 h-5 w-5" viewBox="0 0 24 24">
                <path
                  fill="#4285F4"
                  d="M22.56 12.25c0-.78-.07-1.53-.2-2.25H12v4.26h5.92c-.26 1.37-1.04 2.53-2.21 3.31v2.77h3.57c2.08-1.92 3.28-4.74 3.28-8.09z"
                />
                <path
                  fill="#34A853"
                  d="M12 23c2.97 0 5.46-.98 7.28-2.66l-3.57-2.77c-.98.66-2.23 1.06-3.71 1.06-2.86 0-5.29-1.93-6.16-4.53H2.18v2.84C3.99 20.53 7.7 23 12 23z"
                />
                <path
                  fill="#FBBC05"
                  d="M5.84 14.09c-.22-.66-.35-1.36-.35-2.09s.13-1.43.35-2.09V7.07H2.18C1.43 8.55 1 10.22 1 12s.43 3.45 1.18 4.93l2.85-2.22.81-.62z"
                />
                <path
                  fill="#EA4335"
                  d="M12 5.38c1.62 0 3.06.56 4.21 1.64l3.15-3.15C17.45 2.09 14.97 1 12 1 7.7 1 3.99 3.47 2.18 7.07l3.66 2.84c.87-2.6 3.3-4.53 6.16-4.53z"
                />
              </svg>
              Sign up with Google
            </>
          )}
        </Button>

        {/* Sign In Link */}
        <div className="text-center mt-8">
          <p className="text-gray-400 text-sm">
            Already have an account?{" "}
            <button
              onClick={handleGoogleLogin}
              className="text-cyan-400 hover:text-cyan-300 font-semibold transition-colors"
            >
              Sign in
            </button>
          </p>
        </div>
      </div>
    </div>
  );
};

export default Login;
