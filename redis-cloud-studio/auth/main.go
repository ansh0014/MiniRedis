package main

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"auth/config"
	"auth/router"

	"github.com/gorilla/handlers"
)

func main() {
	fmt.Println("🚀 Starting MiniRedis Auth Service...")

	// Load environment variables
	if err := config.LoadEnv(); err != nil {
		log.Printf("⚠️  Failed to load .env: %v", err)
	}

	// Initialize Firebase
	if err := config.InitFirebase(); err != nil {
		log.Fatalf(" Firebase initialization failed: %v", err)
	}

	// Initialize Database
	if err := config.InitDatabase(); err != nil {
		log.Fatalf(" Database initialization failed: %v", err)
	}
	defer config.Close()

	// Setup router
	r := router.NewAuthRouter()

	// CORS configuration
	corsHandler := handlers.CORS(
		handlers.AllowedOrigins([]string{
			"http://localhost:5173",
			"http://localhost:3000",
			"http://localhost:8080",
		}),
		handlers.AllowedMethods([]string{"GET", "POST", "PUT", "DELETE", "OPTIONS"}),
		handlers.AllowedHeaders([]string{"Content-Type", "Authorization"}),
		handlers.AllowCredentials(),
	)(r)

	// Server configuration
	port := getEnvOrDefault("AUTH_SERVICE_PORT", "8000")
	srv := &http.Server{
		Addr:         ":" + port,
		Handler:      corsHandler,
		ReadTimeout:  15 * time.Second,
		WriteTimeout: 15 * time.Second,
		IdleTimeout:  60 * time.Second,
	}

	// Start server in goroutine
	go func() {
		fmt.Printf(" Auth Service running on http://localhost:%s\n", port)
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatalf("❌ Server failed: %v", err)
		}
	}()

	// Graceful shutdown
	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
	<-quit

	fmt.Println("\n Shutting down server...")
	if err := srv.Close(); err != nil {
		log.Printf("  Server shutdown error: %v", err)
	}
	fmt.Println("Server stopped")
}

func getEnvOrDefault(key, defaultVal string) string {
	if val := os.Getenv(key); val != "" {
		return val
	}
	return defaultVal
}
