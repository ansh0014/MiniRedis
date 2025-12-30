package config

import (
	"context"
	"database/sql"
	"fmt"
	"os"
	"path/filepath"

	firebase "firebase.google.com/go/v4"
	fbauth "firebase.google.com/go/v4/auth"
	"github.com/joho/godotenv"
	"google.golang.org/api/option"

	_ "github.com/lib/pq"
)

var (
	FirebaseAuth *fbauth.Client
	DB           *sql.DB
)

// LoadEnv loads .env file
func LoadEnv() error {
	// Try multiple paths
	paths := []string{".env", "../.env", "../../.env"}

	for _, path := range paths {
		absPath, _ := filepath.Abs(path)
		if err := godotenv.Load(absPath); err == nil {
			fmt.Printf(" Loaded .env ")
			return nil
		}
	}

	fmt.Println("  No .env file found, using system environment")
	return nil
}

// InitFirebase initializes Firebase Admin SDK
func InitFirebase() error {
	ctx := context.Background()

	credFile := os.Getenv("GOOGLE_APPLICATION_CREDENTIALS")
	if credFile == "" {
		return fmt.Errorf("GOOGLE_APPLICATION_CREDENTIALS not set")
	}

	if _, err := os.Stat(credFile); os.IsNotExist(err) {
		return fmt.Errorf("firebase credentials file not found: %s", credFile)
	}

	opt := option.WithCredentialsFile(credFile)
	app, err := firebase.NewApp(ctx, nil, opt)
	if err != nil {
		return fmt.Errorf("firebase NewApp: %w", err)
	}

	authClient, err := app.Auth(ctx)
	if err != nil {
		return fmt.Errorf("firebase auth client: %w", err)
	}

	FirebaseAuth = authClient;
	return nil
}

// InitDatabase connects to PostgreSQL (auth database only)
func InitDatabase() error {
	connStr := fmt.Sprintf("host=%s port=%s user=%s password=%s dbname=%s sslmode=disable",
		getEnvOrDefault("DB_HOST", "localhost"),
		getEnvOrDefault("DB_PORT", "5432"),
		getEnvOrDefault("DB_USER", "postgres"),
		os.Getenv("DB_PASSWORD"),
		getEnvOrDefault("DB_NAME", "auth_db"), // ← Separate database for auth
	)

	db, err := sql.Open("postgres", connStr)
	if err != nil {
		return fmt.Errorf("failed to connect: %w", err)
	}

	if err := db.Ping(); err != nil {
		return fmt.Errorf("failed to ping database: %w", err)
	}

	DB = db
	fmt.Println(" Database connected")
	return nil
}

// Close closes database connection
func Close() {
	if DB != nil {
		DB.Close()
	}
}

func getEnvOrDefault(key, defaultVal string) string {
	if val := os.Getenv(key); val != "" {
		return val
	}
	return defaultVal
}
