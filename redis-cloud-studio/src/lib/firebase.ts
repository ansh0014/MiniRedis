import { initializeApp } from 'firebase/app';
import {
  getAuth,
  GoogleAuthProvider,
  signInWithPopup,
  signOut as firebaseSignOut,
  onAuthStateChanged,
  User
} from 'firebase/auth';

const firebaseConfig = {
  apiKey: import.meta.env.VITE_FIREBASE_API_KEY,
  authDomain: import.meta.env.VITE_FIREBASE_AUTH_DOMAIN,
  projectId: import.meta.env.VITE_FIREBASE_PROJECT_ID,
};

const app = initializeApp(firebaseConfig);
export const auth = getAuth(app);
export const googleProvider = new GoogleAuthProvider();

const API_URL = import.meta.env.VITE_API_URL || 'http://localhost:8080';

export interface UserInfo {
  uid?: string;
  email?: string;
  name?: string;
  picture?: string;
  emailVerified?: boolean;
}

function clearOldCookies() {
  const cookies = document.cookie.split(';');
  cookies.forEach(cookie => {
    const name = cookie.split('=')[0].trim();
    if (name.startsWith('session_token') || name.startsWith('refresh_token') || 
        name.startsWith('__clerk') || name.startsWith('__client_uat')) {
      document.cookie = `${name}=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;`;
    }
  });
}

async function gatewayFetch(path: string, opts: RequestInit = {}) {
  const makeRequest = async (isRetry = false) => {
    const res = await fetch(`${API_URL}${path}`, {
      credentials: 'include',
      headers: { 'Content-Type': 'application/json', ...(opts.headers || {}) },
      ...opts,
    });

    if (res.status === 401 && !isRetry) {
      await new Promise(resolve => setTimeout(resolve, 500));
      return makeRequest(true);
    }

    if (!res.ok) throw new Error(`Gateway ${path} failed: ${res.status}`);
    return res.json().catch(() => ({}));
  };

  return makeRequest();
}

export const signInWithGoogle = async (): Promise<UserInfo> => {
  try {
    clearOldCookies();
    
    const result = await signInWithPopup(auth, googleProvider);
    const idToken = await result.user.getIdToken();

    const response = await fetch(`${API_URL}/auth/login`, {
      method: 'POST',
      credentials: 'include',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ idToken }),
    });

    if (!response.ok) {
      throw new Error(`Authentication failed: ${response.status}`);
    }

    const userData = await response.json().catch(() => ({}));

    return {
      uid: result.user.uid,
      email: result.user.email || userData.email,
      name: result.user.displayName || userData.name,
      picture: result.user.photoURL || userData.picture,
      emailVerified: result.user.emailVerified,
    };
  } catch (error: any) {
    throw error;
  }
};

export const signOut = async (): Promise<void> => {
  try {
    await fetch(`${API_URL}/auth/logout`, { method: 'POST', credentials: 'include' });
    await firebaseSignOut(auth);
    clearOldCookies();
  } catch (error) {
    throw error;
  }
};

export const getCurrentUser = async (): Promise<UserInfo | null> => {
  try {
    const res = await fetch(`${API_URL}/auth/me`, { credentials: 'include' });
    if (!res.ok) return null;
    return res.json();
  } catch (e) {
    return null;
  }
};

export const onAuthStateChange = (cb: (u: User | null) => void) => onAuthStateChanged(auth, cb);
export const isAuthenticated = () => auth.currentUser !== null;