package middleware

import (
	"context"
	"net/http"
	"strings"

	"auth/config"
)

type ctxKey string

const firebaseUserKey ctxKey = "firebaseUser"

// FirebaseAuthMiddleware validates Firebase ID token from Authorization header
// and injects the token claims into request context (key: "firebaseUser").
func FirebaseAuthMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		authHeader := r.Header.Get("Authorization")
		if authHeader == "" {
			http.Error(w, "missing authorization header", http.StatusUnauthorized)
			return
		}

		idToken := strings.TrimSpace(strings.TrimPrefix(authHeader, "Bearer"))
		if idToken == "" {
			http.Error(w, "invalid authorization header", http.StatusUnauthorized)
			return
		}

		// verify token
		token, err := config.FirebaseAuth.VerifyIDToken(r.Context(), idToken)
		if err != nil {
			http.Error(w, "invalid token: "+err.Error(), http.StatusUnauthorized)
			return
		}

		// attach claims to context
		ctx := context.WithValue(r.Context(), firebaseUserKey, token)
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
type configToken = interface{ /* firebase.Token-like */ }