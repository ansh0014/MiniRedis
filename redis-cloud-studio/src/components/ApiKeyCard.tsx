import { useState } from "react";
import { Button } from "@/components/ui/button";
import { Eye, EyeOff, Copy, RefreshCw, Check } from "lucide-react";
import { useToast } from "@/hooks/use-toast";

interface ApiKeyCardProps {
  apiKey: string;
}

const ApiKeyCard = ({ apiKey }: ApiKeyCardProps) => {
  const [isVisible, setIsVisible] = useState(false);
  const [copied, setCopied] = useState(false);
  const [regenerating, setRegenerating] = useState(false);
  const { toast } = useToast();

  const maskedKey = `sk_••••••••••••${apiKey.slice(-4)}`;

  const handleCopy = async () => {
    await navigator.clipboard.writeText(apiKey);
    setCopied(true);
    toast({
      title: "Copied!",
      description: "API key copied to clipboard",
    });
    setTimeout(() => setCopied(false), 2000);
  };

  const handleRegenerate = () => {
    setRegenerating(true);
    // Mock regeneration
    setTimeout(() => {
      setRegenerating(false);
      toast({
        title: "Key Regenerated",
        description: "Your new API key is ready to use",
      });
    }, 1500);
  };

  return (
    <div className="bg-card rounded-lg border border-border p-6 shadow-sm">
      <div className="flex flex-col gap-4 md:flex-row md:items-center md:justify-between">
        <div className="space-y-2">
          <h3 className="text-lg font-semibold text-foreground">Your API Key</h3>
          <div className="flex items-center gap-3">
            <code className="rounded-md bg-muted px-3 py-2 font-mono text-sm">
              {isVisible ? apiKey : maskedKey}
            </code>
          </div>
          <p className="text-sm text-muted-foreground">
            Connection URL: <code className="text-primary">http://localhost:6300</code>
          </p>
        </div>

        <div className="flex flex-wrap gap-2">
          <Button
            variant="outline"
            size="sm"
            onClick={() => setIsVisible(!isVisible)}
          >
            {isVisible ? (
              <>
                <EyeOff className="h-4 w-4 mr-2" />
                Hide
              </>
            ) : (
              <>
                <Eye className="h-4 w-4 mr-2" />
                Show
              </>
            )}
          </Button>
          <Button
            variant="outline"
            size="sm"
            onClick={handleCopy}
          >
            {copied ? (
              <>
                <Check className="h-4 w-4 text-green-500 mr-2" />
                Copied
              </>
            ) : (
              <>
                <Copy className="h-4 w-4 mr-2" />
                Copy
              </>
            )}
          </Button>
          <Button
            variant="outline"
            size="sm"
            onClick={handleRegenerate}
            disabled={regenerating}
          >
            <RefreshCw className={`h-4 w-4 mr-2 ${regenerating ? 'animate-spin' : ''}`} />
            {regenerating ? 'Regenerating...' : 'Regenerate'}
          </Button>
        </div>
      </div>
    </div>
  );
};

export default ApiKeyCard;
