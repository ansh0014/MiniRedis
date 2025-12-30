import { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";

const Switch = ({ checked, onCheckedChange, className, ...props }: any) => (
  <input
    type="checkbox"
    checked={checked}
    onChange={(e) => onCheckedChange?.(e.target.checked)}
    className={className}
    {...props}
  />
);
import { useToast } from "@/hooks/use-toast";
import Navbar from "@/components/Navbar";
import { User, Key, Bell, Trash2, RefreshCw } from "lucide-react";

const Settings = () => {
  const { toast } = useToast();
  const navigate = useNavigate();
  const [loading, setLoading] = useState(false);
  const [user, setUser] = useState<any>(null);
  const [settings, setSettings] = useState({
    emailNotifications: true,
    autoBackup: false,
    twoFactorAuth: false,
  });

  useEffect(() => {
    const userData = localStorage.getItem("user");
    if (!userData) {
      navigate("/login");
      return;
    }
    setUser(JSON.parse(userData));
  }, [navigate]);

  const handleResetPassword = async () => {
    setLoading(true);
    try {
      const response = await fetch("http://localhost:8080/api/instance/reset-password", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          Authorization: `Bearer ${user?.idToken}`,
        },
      });

      if (!response.ok) throw new Error("Failed to reset password");

      const data = await response.json();

      toast({
        title: "Password Reset",
        description: `New password: ${data.newPassword}`,
      });
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to reset password",
        variant: "destructive",
      });
    } finally {
      setLoading(false);
    }
  };

  const handleDeleteAccount = async () => {
    if (!confirm("Are you sure? This action cannot be undone!")) return;

    setLoading(true);
    try {
      const response = await fetch(`http://localhost:8080/api/user/${user?.uid}`, {
        method: "DELETE",
        headers: {
          Authorization: `Bearer ${user?.idToken}`,
        },
      });

      if (!response.ok) throw new Error("Failed to delete account");

      localStorage.clear();
      toast({
        title: "Account Deleted",
        description: "Your account has been permanently deleted",
      });
      navigate("/");
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to delete account",
        variant: "destructive",
      });
    } finally {
      setLoading(false);
    }
  };

  if (!user) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-slate-900 via-slate-800 to-slate-900 flex items-center justify-center">
        <RefreshCw className="h-8 w-8 animate-spin text-cyan-400" />
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-900 via-slate-800 to-slate-900">
      <Navbar />

      <div className="container mx-auto p-6 pt-24 max-w-4xl">
        <h1 className="text-4xl font-[Orbitron] font-bold text-white mb-8">Settings</h1>

        {/* Profile */}
        <Card className="bg-slate-900/50 border-cyan-400/30 backdrop-blur-xl mb-6">
          <CardHeader>
            <CardTitle className="text-cyan-400 font-[Orbitron] flex items-center gap-2">
              <User className="h-5 w-5" />
              Profile Information
            </CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            <div>
              <Label className="text-cyan-100/70">Email</Label>
              <Input
                value={user.email || ""}
                disabled
                className="bg-slate-800/50 border-cyan-400/30 text-cyan-100 mt-2"
              />
            </div>
            <div>
              <Label className="text-cyan-100/70">User ID</Label>
              <Input
                value={user.uid || ""}
                disabled
                className="bg-slate-800/50 border-cyan-400/30 text-cyan-100 mt-2 font-mono"
              />
            </div>
          </CardContent>
        </Card>

        {/* Security */}
        <Card className="bg-slate-900/50 border-cyan-400/30 backdrop-blur-xl mb-6">
          <CardHeader>
            <CardTitle className="text-cyan-400 font-[Orbitron] flex items-center gap-2">
              <Key className="h-5 w-5" />
              Security
            </CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            <Button
              onClick={handleResetPassword}
              disabled={loading}
              variant="outline"
              className="w-full border-cyan-400/50 text-cyan-400 hover:bg-cyan-400/10"
            >
              <RefreshCw className={`h-4 w-4 mr-2 ${loading ? "animate-spin" : ""}`} />
              Reset Redis Password
            </Button>
          </CardContent>
        </Card>

        {/* Notifications */}
        <Card className="bg-slate-900/50 border-cyan-400/30 backdrop-blur-xl mb-6">
          <CardHeader>
            <CardTitle className="text-cyan-400 font-[Orbitron] flex items-center gap-2">
              <Bell className="h-5 w-5" />
              Notifications
            </CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="flex items-center justify-between">
              <div>
                <Label className="text-cyan-100">Email Notifications</Label>
                <p className="text-cyan-100/60 text-sm">Receive updates via email</p>
              </div>
              <Switch
                checked={settings.emailNotifications}
                onCheckedChange={(checked) =>
                  setSettings({ ...settings, emailNotifications: checked })
                }
              />
            </div>

            <div className="flex items-center justify-between">
              <div>
                <Label className="text-cyan-100">Auto Backup</Label>
                <p className="text-cyan-100/60 text-sm">Daily automatic backups</p>
              </div>
              <Switch
                checked={settings.autoBackup}
                onCheckedChange={(checked) =>
                  setSettings({ ...settings, autoBackup: checked })
                }
              />
            </div>
          </CardContent>
        </Card>

        {/* Danger Zone */}
        <Card className="bg-red-900/20 border-red-400/30 backdrop-blur-xl">
          <CardHeader>
            <CardTitle className="text-red-400 font-[Orbitron] flex items-center gap-2">
              <Trash2 className="h-5 w-5" />
              Danger Zone
            </CardTitle>
          </CardHeader>
          <CardContent>
            <p className="text-red-100/70 mb-4">
              Once you delete your account, there is no going back. All your data will be
              permanently deleted.
            </p>
            <Button
              onClick={handleDeleteAccount}
              disabled={loading}
              variant="destructive"
              className="bg-red-500 hover:bg-red-600"
            >
              <Trash2 className="h-4 w-4 mr-2" />
              Delete Account
            </Button>
          </CardContent>
        </Card>
      </div>
    </div>
  );
};

export default Settings;
