import { useState, useEffect } from 'react';
import { Button } from './ui/button';

export function ThemeToggle() {
  const [theme, setTheme] = useState<'light' | 'dark'>('light');

  useEffect(() => {
    const root = document.documentElement;
    root.classList.remove('light', 'dark');
    root.classList.add(theme);
  }, [theme]);

  const toggleTheme = () => {
    setTheme(prev => prev === 'light' ? 'dark' : 'light');
  };

  return (
    <Button
      onClick={toggleTheme}
      className="fixed bottom-4 right-4 rounded-full p-3 shadow-lg z-50"
      variant="outline"
    >
      {theme === 'light' ? '🌙' : '☀️'}
    </Button>
  );
}