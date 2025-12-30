package router

import (
	"auth/config"
	"auth/handler"
	"auth/middleware"

	"github.com/gorilla/mux"
)

// NewAuthRouter initializes and returns the authentication router
func NewAuthRouter() *mux.Router {
	// ensure firebase initialized before registering routes
	_ = config.InitFirebase()

	r := mux.NewRouter()

	// Public routes (no authentication required)
	r.HandleFunc("/auth/google", handler.GoogleLoginHandler).Methods("POST", "OPTIONS")
	r.HandleFunc("/auth/healthz", handler.HealthHandler).Methods("GET")

	// Protected routes (require valid session cookie)
	protected := r.PathPrefix("/auth").Subrouter()
	protected.Use(middleware.FirebaseAuthMiddleware)
	protected.HandleFunc("/me", handler.ProtectedMeHandler).Methods("GET")
	protected.HandleFunc("/logout", handler.LogoutHandler).Methods("POST")

	return r
}
