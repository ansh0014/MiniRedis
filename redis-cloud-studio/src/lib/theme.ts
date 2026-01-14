export const themes = {
  dark: {
    // Colors
    bg: {
      primary: '#0a0e1a',
      secondary: '#13182b',
      tertiary: '#1a1f35',
      card: 'rgba(26, 31, 53, 0.6)',
    },
    text: {
      primary: '#e8e9ed',
      secondary: '#9ca3af',
      muted: '#6b7280',
    },
    accent: {
      primary: '#d4a373', // muted gold
      secondary: '#ff8c42', // soft orange
      glow: 'rgba(212, 163, 115, 0.15)',
    },
    border: 'rgba(255, 255, 255, 0.08)',
    
    // Typography
    font: {
      family: '"Space Grotesk", "Inter", sans-serif',
      weight: {
        heading: '600',
        body: '400',
      },
      spacing: '-0.02em',
    },
    
    // Effects
    shadow: {
      sm: '0 2px 8px rgba(0, 0, 0, 0.4)',
      md: '0 4px 16px rgba(0, 0, 0, 0.5)',
      lg: '0 8px 32px rgba(0, 0, 0, 0.6)',
      glow: '0 0 20px rgba(212, 163, 115, 0.2)',
    },
    
    // Spline
    spline: 'https://prod.spline.design/your-dark-space-scene/scene.splinecode',
  },
  
  light: {
    // Colors
    bg: {
      primary: '#f5f3ff',
      secondary: '#ede9fe',
      tertiary: '#e9d5ff',
      card: 'rgba(255, 255, 255, 0.7)',
    },
    text: {
      primary: '#3730a3',
      secondary: '#6366f1',
      muted: '#8b89a8',
    },
    accent: {
      primary: '#7c3aed', // violet
      secondary: '#6366f1', // indigo
      glow: 'transparent',
    },
    border: 'rgba(124, 58, 237, 0.15)',
    
    // Typography
    font: {
      family: '"Space Grotesk", "Inter", sans-serif',
      weight: {
        heading: '500',
        body: '400',
      },
      spacing: '0em',
    },
    
    // Effects
    shadow: {
      sm: '0 2px 8px rgba(124, 58, 237, 0.08)',
      md: '0 4px 16px rgba(124, 58, 237, 0.1)',
      lg: '0 8px 32px rgba(124, 58, 237, 0.12)',
      glow: 'none',
    },
    
    // Spline
    spline: 'https://prod.spline.design/your-light-moonlight-scene/scene.splinecode',
  },
};

export type Theme = typeof themes.dark;