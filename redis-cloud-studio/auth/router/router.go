package router

import (
	"net/http"

	"auth/config"
	"auth/handler"    
	"auth/middleware"

	"github.com/gorilla/mux"
)

// NewAuthRouter initializes the authentication routes
func NewAuthRouter() *mux.Router {
	// ensure firebase initialized before registering routes
	_ = config.InitFirebase()

	r := mux.NewRouter()

	// public endpoint - verify ID token sent from client and return user info
	r.HandleFunc("/auth/google", handler.GoogleLoginHandler).Methods("POST")

	// protected example
	protected := r.PathPrefix("/auth").Subrouter()
	protected.Use(middleware.FirebaseAuthMiddleware)
	protected.HandleFunc("/me", handler.ProtectedMeHandler).Methods("GET")

	// health
	r.HandleFunc("/auth/healthz", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("ok"))
	})

	return r
}
