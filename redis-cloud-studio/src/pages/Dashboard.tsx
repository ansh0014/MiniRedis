import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Avatar, AvatarFallback, AvatarImage } from "@/components/ui/avatar";
import { Loader2, LogOut } from "lucide-react";
import { getCurrentUser, signOut, UserInfo } from "@/lib/firebase";
import { toast } from "sonner";
import MetricCard from "@/components/MetricCard";
import Terminal from "@/components/Terminal";

const Dashboard = () => {
  const navigate = useNavigate();
  const [user, setUser] = useState<UserInfo | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadUser();
  }, []);

  const loadUser = async () => {
    try {
      // Try to get from localStorage first
      const cachedUser = localStorage.getItem("user");
      if (cachedUser) {
        setUser(JSON.parse(cachedUser));
      }

      // Then fetch from server
      const userData = await getCurrentUser();
      if (userData) {
        setUser(userData);
        localStorage.setItem("user", JSON.stringify(userData));
      } else {
        // Not authenticated, redirect to login
        navigate("/login");
      }
    } catch (error) {
      console.error("Failed to load user:", error);
      navigate("/login");
    } finally {
      setLoading(false);
    }
  };

  const handleSignOut = async () => {
    try {
      await signOut();
      localStorage.removeItem("user");
      toast.success("Signed out successfully");
      navigate("/login");
    } catch (error) {
      console.error("Sign out error:", error);
      toast.error("Failed to sign out");
    }
  };

  if (loading) {
    return (
      <div className="min-h-screen flex items-center justify-center">
        <Loader2 className="h-8 w-8 animate-spin text-primary" />
      </div>
    );
  }

  if (!user) {
    return null; // Will redirect
  }

  const userInitials = user.name
    ?.split(" ")
    .map((n) => n[0])
    .join("")
    .toUpperCase() || "U";

  return (
    <div className="min-h-screen bg-gradient-to-br from-gray-50 to-gray-100 dark:from-gray-900 dark:to-gray-800">
      {/* Header */}
      <header className="bg-white dark:bg-gray-800 shadow-sm border-b">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center space-x-4">
              <h1 className="text-2xl font-bold text-gray-900 dark:text-white">
                MiniRedis Dashboard
              </h1>
            </div>
            
            <div className="flex items-center space-x-4">
              <div className="text-right hidden sm:block">
                <p className="text-sm font-medium text-gray-900 dark:text-white">
                  {user.name}
                </p>
                <p className="text-xs text-gray-500 dark:text-gray-400">
                  {user.email}
                </p>
              </div>
              
              <Avatar>
                <AvatarImage src={user.picture} alt={user.name} />
                <AvatarFallback>{userInitials}</AvatarFallback>
              </Avatar>
              
              <Button
                variant="ghost"
                size="icon"
                onClick={handleSignOut}
                title="Sign out"
              >
                <LogOut className="h-5 w-5" />
              </Button>
            </div>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        {/* Metrics Grid */}
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-8">
          <MetricCard
            title="Memory Usage"
            value="15.2 MB"
            change="+2.5%"
            trend="up"
          />
          <MetricCard
            title="Total Keys"
            value="1,234"
            change="+12%"
            trend="up"
          />
          <MetricCard
            title="Commands/sec"
            value="456"
            change="-3%"
            trend="down"
          />
          <MetricCard
            title="Connected Clients"
            value="8"
            change="+1"
            trend="up"
          />
        </div>

        {/* Terminal */}
        <Card>
          <CardHeader>
            <CardTitle>Redis Terminal</CardTitle>
          </CardHeader>
          <CardContent>
            <Terminal />
          </CardContent>
        </Card>

      
        <Card>
          <CardHeader>
            <CardTitle>Account Information</CardTitle>
          </CardHeader>
          <CardContent className="space-y-3">
            <div className="flex justify-between items-center">
              <span className="text-muted-foreground">Email:</span>
              <span>{user.email}</span>
            </div>
            <div className="flex justify-between items-center">
              <span className="text-muted-foreground">Email Verified:</span>
              <span>{user.emailVerified ? "✅ Yes" : "❌ No"}</span>
            </div>
            {/* REMOVED: uid, firebase_uid, internal IDs */}
          </CardContent>
        </Card>
      </main>
    </div>
  );
};

export default Dashboard;
