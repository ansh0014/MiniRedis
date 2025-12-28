package handler

import (
	"bytes"
	"context"
	"encoding/json"
	"net/http"
	"os"
	"time"

	"auth/config" // adjust module path if needed
)

type GoogleLoginRequest struct {
	IDToken string `json:"idToken"`
}

type LoginResponse struct {
	UID             string `json:"uid"`
	Email           string `json:"email"`
	EmailVerified   bool   `json:"emailVerified"`
	Name            string `json:"name"`
	Picture         string `json:"picture"`
	TenantID        string `json:"tenantId"`
	MiniRedisAPIKey string `json:"miniRedisApiKey"`
	Host            string `json:"host"`
	Port            int    `json:"port"`
	Password        string `json:"password"`
}

type BackendInstanceRequest struct {
	FirebaseUID string `json:"firebaseUid"`
	Email       string `json:"email"`
}

type BackendInstanceResponse struct {
	Success       bool   `json:"success"`
	TenantID      string `json:"tenantId"`
	APIKey        string `json:"apiKey"`
	Host          string `json:"host"`
	Port          int    `json:"port"`
	Password      string `json:"password"`
	ContainerName string `json:"containerName"`
	Status        string `json:"status"`
}

// GoogleLoginHandler accepts a JSON body { "idToken": "..." }, verifies it with
// Firebase and returns basic user info. Use this from the client after Google Sign-in.
func GoogleLoginHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()

	var body GoogleLoginRequest
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil || body.IDToken == "" {
		http.Error(w, "invalid request payload", http.StatusBadRequest)
		return
	}

	// Verify Firebase ID token
	token, err := config.FirebaseAuth.VerifyIDToken(ctx, body.IDToken)
	if err != nil {
		http.Error(w, "token verification failed: "+err.Error(), http.StatusUnauthorized)
		return
	}

	// Get user record
	userRecord, err := config.FirebaseAuth.GetUser(ctx, token.UID)
	if err != nil {
		http.Error(w, "failed to get user: "+err.Error(), http.StatusInternalServerError)
		return
	}

	// Call Backend to create/get Redis instance
	instance, err := createOrGetInstance(ctx, token.UID, userRecord.Email)
	if err != nil {
		http.Error(w, "failed to create instance: "+err.Error(), http.StatusInternalServerError)
		return
	}

	// Set HTTP-only cookie with Firebase token
	http.SetCookie(w, &http.Cookie{
		Name:     "firebase_token",
		Value:    body.IDToken,
		HttpOnly: true,
		Secure:   true, // Use true in production
		SameSite: http.SameSiteStrictMode,
		MaxAge:   3600, // 1 hour
		Path:     "/",
	})

	// Also set API key cookie
	http.SetCookie(w, &http.Cookie{
		Name:     "miniredis_api_key",
		Value:    instance.APIKey,
		HttpOnly: true,
		Secure:   true,
		SameSite: http.SameSiteStrictMode,
		MaxAge:   30 * 24 * 3600, // 30 days
		Path:     "/",
	})

	resp := LoginResponse{
		UID:             token.UID,
		Email:           userRecord.Email,
		EmailVerified:   userRecord.EmailVerified,
		Name:            userRecord.DisplayName,
		Picture:         userRecord.PhotoURL,
		TenantID:        instance.TenantID,
		MiniRedisAPIKey: instance.APIKey,
		Host:            instance.Host,
		Port:            instance.Port,
		Password:        instance.Password,
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(resp)
}

func createOrGetInstance(ctx context.Context, firebaseUID, email string) (*BackendInstanceResponse, error) {
	backendURL := os.Getenv("BACKEND_URL")
	if backendURL == "" {
		backendURL = "http://localhost:8080"
	}

	reqBody := BackendInstanceRequest{
		FirebaseUID: firebaseUID,
		Email:       email,
	}

	jsonData, err := json.Marshal(reqBody)
	if err != nil {
		return nil, err
	}

	req, err := http.NewRequestWithContext(ctx, "POST", backendURL+"/api/user/instance", bytes.NewBuffer(jsonData))
	if err != nil {
		return nil, err
	}

	req.Header.Set("Content-Type", "application/json")

	client := &http.Client{Timeout: 10 * time.Second}
	resp, err := client.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, err
	}

	var instanceResp BackendInstanceResponse
	if err := json.NewDecoder(resp.Body).Decode(&instanceResp); err != nil {
		return nil, err
	}

	return &instanceResp, nil
}

// ProtectedMeHandler example - requires FirebaseAuthMiddleware to have run.
func ProtectedMeHandler(w http.ResponseWriter, r *http.Request) {
	tok := r.Context().Value("firebaseUser")
	if tok == nil {
		http.Error(w, "unauthorized", http.StatusUnauthorized)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(tok)
}
