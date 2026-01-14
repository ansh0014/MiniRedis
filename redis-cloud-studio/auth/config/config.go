package config

import (
	"context"
	"database/sql"
	"fmt"
	"log"
	"os"
	"path/filepath"

	"firebase.google.com/go/v4"
	"firebase.google.com/go/v4/auth"
	"github.com/joho/godotenv"
	_ "github.com/lib/pq"
	"google.golang.org/api/option"
)

var (
	FirebaseAuth *auth.Client
	DB           *sql.DB
)


func LoadEnv() error {
	
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


func InitFirebase() {
	opt := option.WithCredentialsFile("firebase-service-account.json")
	app, err := firebase.NewApp(context.Background(), nil, opt)
	if err != nil {
		log.Fatalf(" Firebase init error: %v\n", err)
	}

	FirebaseAuth, err = app.Auth(context.Background())
	if err != nil {
		log.Fatalf(" Firebase Auth error: %v\n", err)
	}
}
func InitFirebasekey(){
	log.Println(" Initializing Firebase with API Key...")
	if err := LoadEnv(); err != nil {
		log.Fatalf(" Error loading .env file: %v\n", err)
	}
}


func InitDatabase() {
	connStr := fmt.Sprintf(
		"host=%s port=%s user=%s password=%s dbname=%s sslmode=disable",
		os.Getenv("DB_HOST"),
		os.Getenv("DB_PORT"),
		os.Getenv("DB_USER"),
		os.Getenv("DB_PASSWORD"),
		os.Getenv("DB_NAME"),
	)

	var err error
	DB, err = sql.Open("postgres", connStr)
	if err != nil {
		log.Fatalf(" Database connection error: %v\n", err)
	}

	if err = DB.Ping(); err != nil {
		log.Fatalf(" Database ping error: %v\n", err)
	}

	fmt.Println(" Database connected")
}

// Close closes database connection
func CloseDatabase() {
	if DB != nil {
		DB.Close()
	}
}
