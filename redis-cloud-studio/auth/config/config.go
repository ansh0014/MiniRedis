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

// LoadEnv loads environment variables from .env file
func LoadEnv() error {
	// Try to find .env in multiple locations
	envPaths := []string{
		".env",
		"../.env",
		"../../.env",
	}

	var loadedPath string
	for _, path := range envPaths {
		absPath, _ := filepath.Abs(path)
		if err := godotenv.Load(absPath); err == nil {
			loadedPath = absPath
			break
		}
	}

	if loadedPath != "" {
		fmt.Printf(" Loaded .env from: %s\n", loadedPath)
	} else {
		fmt.Println(" No .env file found, using system environment")
	}

	return nil
}

// InitFirebase initializes Firebase Auth client
func InitFirebase() error {
	ctx := context.Background()

	credFile := os.Getenv("GOOGLE_APPLICATION_CREDENTIALS")
	var opt option.ClientOption

	if credFile != "" {
		if _, err := os.Stat(credFile); os.IsNotExist(err) {
			return fmt.Errorf("firebase credentials file not found: %s", credFile)
		}
		opt = option.WithCredentialsFile(credFile)
	} else if json := os.Getenv("GOOGLE_CREDENTIALS_JSON"); json != "" {
		opt = option.WithCredentialsJSON([]byte(json))
	} else {
		return fmt.Errorf("firebase credentials not provided")
	}

	app, err := firebase.NewApp(ctx, nil, opt)
	if err != nil {
		return fmt.Errorf("firebase NewApp: %w", err)
	}

	authClient, err := app.Auth(ctx)
	if err != nil {
		return fmt.Errorf("firebase auth client: %w", err)
	}

	FirebaseAuth = authClient
	fmt.Println(" Firebase initialized")
	return nil
}

// InitDatabase initializes PostgreSQL connection
func InitDatabase() error {
	dbHost := os.Getenv("DB_HOST")
	dbPort := os.Getenv("DB_PORT")
	dbUser := os.Getenv("DB_USER")
	dbPass := os.Getenv("DB_PASSWORD")
	dbName := os.Getenv("DB_NAME")

	if dbHost == "" {
		dbHost = "localhost"
	}
	if dbPort == "" {
		dbPort = "5432"
	}
	if dbUser == "" {
		dbUser = "postgres"
	}
	if dbName == "" {
		dbName = "miniredis"
	}

	connStr := fmt.Sprintf("host=%s port=%s user=%s password=%s dbname=%s sslmode=disable",
		dbHost, dbPort, dbUser, dbPass, dbName)

	fmt.Printf(" Connecting to PostgreSQL at %s:%s/%s\n", dbHost, dbPort, dbName)

	db, err := sql.Open("postgres", connStr)
	if err != nil {
		return fmt.Errorf("failed to open database: %w", err)
	}

	db.SetMaxOpenConns(25)
	db.SetMaxIdleConns(5)

	if err := db.Ping(); err != nil {
		return fmt.Errorf("failed to ping database: %w", err)
	}

	DB = db
	fmt.Println(" Database connected")
	return nil
}

func Close() {
	if DB != nil {
		DB.Close()
	}
}
