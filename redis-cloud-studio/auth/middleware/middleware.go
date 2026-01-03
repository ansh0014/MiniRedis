package middleware

import (
	"context"
	"net/http"
	"strings"

	"auth/config"

	fbauth "firebase.google.com/go/v4/auth"
)

type ctxKey string

const firebaseUserKey ctxKey = "firebaseUser"

// FirebaseAuthMiddleware verifies Firebase token from session cookie
func FirebaseAuthMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		var tokenStr string

		// Method 1: Parse from Cookie header string
		cookieHeader := r.Header.Get("Cookie")
		if cookieHeader != "" {
			for _, cookie := range strings.Split(cookieHeader, ";") {
				parts := strings.SplitN(strings.TrimSpace(cookie), "=", 2)
				if len(parts) == 2 && parts[0] == "session_token" {
					tokenStr = parts[1]
					break
				}
			}
		}

		// Method 2: Fallback to http.Cookie
		if tokenStr == "" {
			c, err := r.Cookie("session_token")
			if err == nil {
				tokenStr = c.Value
			}
		}

		if tokenStr == "" {
			w.Header().Set("Content-Type", "application/json")
			http.Error(w, `{"error":"Unauthorized"}`, http.StatusUnauthorized)
			return
		}

		// Verify token with Firebase
		verifiedToken, err := config.FirebaseAuth.VerifyIDToken(r.Context(), tokenStr)
		if err != nil {
			w.Header().Set("Content-Type", "application/json")
			http.Error(w, `{"error":"Unauthorized"}`, http.StatusUnauthorized)
			return
		}

		// Store token in context
		ctx := context.WithValue(r.Context(), firebaseUserKey, verifiedToken)
		next.ServeHTTP(w, r.WithContext(ctx))
	})
}

// FromContext returns verified Firebase token or nil
func FromContext(ctx context.Context) *fbauth.Token {
	tok := ctx.Value(firebaseUserKey)
	if tok == nil {
		return nil
	}
	token, ok := tok.(*fbauth.Token)
	if !ok {
		return nil
	}
	return token
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}
