package config

import (
	"context"
	"fmt"
	"os"

	firebase "firebase.google.com/go/v4"
	fbauth "firebase.google.com/go/v4/auth"
	"google.golang.org/api/option"
)

var FirebaseAuth *fbauth.Client

// InitFirebase initializes Firebase Auth client. Provide either
// GOOGLE_APPLICATION_CREDENTIALS (file path) or GOOGLE_CREDENTIALS_JSON (raw JSON).

func InitFirebase() error {
	ctx := context.Background()

	credFile := os.Getenv("GOOGLE_APPLICATION_CREDENTIALS")
	var opt option.ClientOption

	if credFile != "" {
		opt = option.WithCredentialsFile(credFile)
	} else if json := os.Getenv("GOOGLE_CREDENTIALS_JSON"); json != "" {
		opt = option.WithCredentialsJSON([]byte(json))
	} else {
		return fmt.Errorf("firebase credentials not provided; set GOOGLE_APPLICATION_CREDENTIALS or GOOGLE_CREDENTIALS_JSON")
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
	return nil
}

