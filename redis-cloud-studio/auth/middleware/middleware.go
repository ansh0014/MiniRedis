package middleware

import (
	"context"
	"fmt"
	"net/http"

	"auth/config"
)

type ctxKey string

const firebaseUserKey ctxKey = "firebaseUser"

// FirebaseAuthMiddleware verifies Firebase token from session cookie
func FirebaseAuthMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// Get session token from cookie
		cookie, err := r.Cookie("session_token")
		if err != nil {
			http.Error(w, `{"error":"unauthorized - no session cookie"}`, http.StatusUnauthorized)
			return
		}

		if cookie.Value == "" {
			http.Error(w, `{"error":"unauthorized - empty session token"}`, http.StatusUnauthorized)
			return
		}

		// Verify Firebase ID token
		token, err := config.FirebaseAuth.VerifyIDToken(r.Context(), cookie.Value)
		if err != nil {
			fmt.Printf("❌ Token verification failed: %v\n", err)
			http.Error(w, `{"error":"unauthorized - invalid token"}`, http.StatusUnauthorized)
			return
		}

		// Add token to context
		ctx := context.WithValue(r.Context(), "firebaseUser", token)
		next.ServeHTTP(w, r.WithContext(ctx))
	})
}

// FromContext returns verified Firebase token (auth.Token) or nil.
func FromContext(ctx context.Context) *configToken {
	tok := ctx.Value(firebaseUserKey)
	if tok == nil {
		return nil
	}
	claims, ok := tok.(*configToken)
	if !ok {
		return nil
	}
	return claims
}

// helper type alias to avoid importing firebase auth inside other packages
type configToken = interface { /* firebase.Token-like */
}
