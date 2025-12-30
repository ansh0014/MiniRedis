import { initializeApp } from 'firebase/app';
import { 
  getAuth, 
  GoogleAuthProvider, 
  signInWithPopup,
  signOut as firebaseSignOut,
  onAuthStateChanged,
  User
} from 'firebase/auth';

// Firebase configuration from environment variables
const firebaseConfig = {
  apiKey: import.meta.env.VITE_FIREBASE_API_KEY,
  authDomain: import.meta.env.VITE_FIREBASE_AUTH_DOMAIN,
  projectId: import.meta.env.VITE_FIREBASE_PROJECT_ID,
  storageBucket: import.meta.env.VITE_FIREBASE_STORAGE_BUCKET,
  messagingSenderId: import.meta.env.VITE_FIREBASE_MESSAGING_SENDER_ID,
  appId: import.meta.env.VITE_FIREBASE_APP_ID,
};

// Validate config (only in development)
if (import.meta.env.DEV) {
  const isConfigValid = Object.values(firebaseConfig).every(val => val && val !== 'undefined');
  if (!isConfigValid) {
    console.error('Firebase configuration is missing. Check your .env file.');
  }
}

// Initialize Firebase
const app = initializeApp(firebaseConfig);
export const auth = getAuth(app);
export const googleProvider = new GoogleAuthProvider();

// Auth service URL
const AUTH_SERVICE_URL = import.meta.env.VITE_AUTH_SERVICE_URL || 'http://localhost:8000';

// User info from backend (NO SENSITIVE DATA)
export interface UserInfo {
  email: string;
  name: string;
  picture?: string;
  emailVerified: boolean;
}

/**
 * Sign in with Google popup
 * Returns user info from Auth Service
 */
export const signInWithGoogle = async (): Promise<UserInfo> => {
  try {
    // 1. Firebase popup login
    const result = await signInWithPopup(auth, googleProvider);
    const user = result.user;

    // 2. Get Firebase ID token
    const idToken = await user.getIdToken();

    // 3. Send token to Auth Service
    const response = await fetch(`${AUTH_SERVICE_URL}/auth/google`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      credentials: 'include',
      body: JSON.stringify({ idToken }),
    });

    if (!response.ok) {
      const errorData = await response.json().catch(() => ({ error: 'Unknown error' }));
      throw new Error(errorData.error || `Auth service error: ${response.status}`);
    }

    const userData: UserInfo = await response.json();

    // Return only non-sensitive data
    return {
      email: userData.email,
      name: userData.name,
      picture: userData.picture,
      emailVerified: userData.emailVerified,
    };
  } catch (error: any) {
    // Handle specific Firebase errors
    if (error.code === 'auth/popup-closed-by-user') {
      throw new Error('Sign-in cancelled. Please try again.');
    } else if (error.code === 'auth/network-request-failed') {
      throw new Error('Network error. Please check your connection.');
    } else if (error.message) {
      throw error;
    } else {
      throw new Error('Failed to sign in. Please try again.');
    }
  }
};

/**
 * Sign out user
 */
export const signOut = async (): Promise<void> => {
  try {
    // 1. Call backend logout
    await fetch(`${AUTH_SERVICE_URL}/auth/logout`, {
      method: 'POST',
      credentials: 'include',
    });

    // 2. Sign out from Firebase
    await firebaseSignOut(auth);
  } catch (error) {
    throw new Error('Failed to sign out');
  }
};

/**
 * Get current user info from Auth Service
 */
export const getCurrentUser = async (): Promise<UserInfo | null> => {
  try {
    const response = await fetch(`${AUTH_SERVICE_URL}/auth/me`, {
      method: 'GET',
      credentials: 'include',
    });

    if (!response.ok) {
      return null;
    }

    const userData: UserInfo = await response.json();
    
    // Return only non-sensitive data
    return {
      email: userData.email,
      name: userData.name,
      picture: userData.picture,
      emailVerified: userData.emailVerified,
    };
  } catch (error) {
    return null;
  }
};


export const onAuthStateChange = (callback: (user: User | null) => void) => {
  return onAuthStateChanged(auth, callback);
};




export const isAuthenticated = (): boolean => {
  return auth.currentUser !== null;
};