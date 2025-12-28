package main

import (
    
    "log"
    "net/http"
    "os"
    "time"

    "auth/config"
    "auth/router"

    "github.com/gorilla/handlers"
)

func main() {
    // Initialize Firebase
    if err := config.InitFirebase(); err != nil {
        log.Fatalf("Failed to initialize Firebase: %v", err)
    }

    // Initialize PostgreSQL (if needed in auth service)
    if err := config.InitDatabase(); err != nil {
        log.Fatalf("Failed to initialize database: %v", err)
    }

    // Setup router
    r := router.NewAuthRouter()

    // CORS middleware
    corsHandler := handlers.CORS(
        handlers.AllowedOrigins([]string{"http://localhost:5173", "http://localhost:3000"}),
        handlers.AllowedMethods([]string{"GET", "POST", "PUT", "DELETE", "OPTIONS"}),
        handlers.AllowedHeaders([]string{"Content-Type", "Authorization"}),
        handlers.AllowCredentials(),
    )(r)

    port := os.Getenv("PORT")
    if port == "" {
        port = "8000"
    }

    server := &http.Server{
        Addr:         ":" + port,
        Handler:      corsHandler,
        ReadTimeout:  15 * time.Second,
        WriteTimeout: 15 * time.Second,
        IdleTimeout:  60 * time.Second,
    }

    log.Printf(" Auth Service running on http://localhost:%s", port)
    if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
        log.Fatalf("Server failed: %v", err)
    }
}