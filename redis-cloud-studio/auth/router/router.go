package router

import (
	"auth/handler"
	"auth/middleware"

	"github.com/gorilla/mux"
)


func SetupRouter() *mux.Router {
	r := mux.NewRouter()


	r.HandleFunc("/auth/login", handler.GoogleLoginHandler).Methods("POST", "OPTIONS")
	r.HandleFunc("/auth/google", handler.GoogleLoginHandler).Methods("POST", "OPTIONS")
	r.HandleFunc("/auth/refresh", handler.RefreshTokenHandler).Methods("POST", "OPTIONS")

	r.HandleFunc("/auth/healthz", handler.HealthHandler).Methods("GET")
	r.HandleFunc("/health", handler.HealthHandler).Methods("GET")


	protected := r.PathPrefix("/auth").Subrouter()
	protected.Use(middleware.FirebaseAuthMiddleware)
	protected.HandleFunc("/me", handler.ProtectedMeHandler).Methods("GET", "OPTIONS")
	protected.HandleFunc("/logout", handler.LogoutHandler).Methods("POST", "OPTIONS")

	return r
}
