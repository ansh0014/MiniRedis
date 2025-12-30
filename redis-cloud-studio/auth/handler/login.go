package handler

import (
	"database/sql"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"auth/config"

	fbauth "firebase.google.com/go/v4/auth"
)

type GoogleLoginRequest struct {
	IDToken string `json:"idToken"`
}

type LoginResponse struct {
	// REMOVE: UID string `json:"uid"` - Firebase UID is sensitive
	Email         string `json:"email"`
	Name          string `json:"name"`
	Picture       string `json:"picture,omitempty"`
	EmailVerified bool   `json:"emailVerified"`
	// Add only non-sensitive user data
}

type ErrorResponse struct {
	Error string `json:"error"`
}

// GoogleLoginHandler handles Google OAuth login via Firebase
func GoogleLoginHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()

	// Parse request body
	var req GoogleLoginRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		respondError(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if req.IDToken == "" {
		respondError(w, "idToken is required", http.StatusBadRequest)
		return
	}

	// 1. Verify Firebase ID token
	token, err := config.FirebaseAuth.VerifyIDToken(ctx, req.IDToken)
	if err != nil {
		fmt.Printf("❌ Token verification failed: %v\n", err)
		respondError(w, "Invalid or expired token", http.StatusUnauthorized)
		return
	}

	// 2. Get user details from Firebase
	userRecord, err := config.FirebaseAuth.GetUser(ctx, token.UID)
	if err != nil {
		fmt.Printf("❌ Failed to get user from Firebase: %v\n", err)
		respondError(w, "Failed to retrieve user information", http.StatusInternalServerError)
		return
	}

	fmt.Printf("✅ Firebase user verified: %s (%s)\n", userRecord.Email, token.UID)

	// 3. Store or update user in database
	var userID int64
	err = config.DB.QueryRowContext(ctx,
		`INSERT INTO users (firebase_uid, email, name, picture_url, email_verified, created_at, last_login)
         VALUES ($1, $2, $3, $4, $5, NOW(), NOW())
         ON CONFLICT (firebase_uid) 
         DO UPDATE SET 
            name = EXCLUDED.name,
            picture_url = EXCLUDED.picture_url,
            email_verified = EXCLUDED.email_verified,
            last_login = NOW()
         RETURNING id`,
		token.UID,
		userRecord.Email,
		userRecord.DisplayName,
		userRecord.PhotoURL,
		userRecord.EmailVerified,
	).Scan(&userID)

	if err != nil {
		fmt.Printf("❌ Database error: %v\n", err)
		respondError(w, "Database error", http.StatusInternalServerError)
		return
	}

	fmt.Printf(" User stored in database")

	// 4. Set HTTP-only session cookie
	http.SetCookie(w, &http.Cookie{
		Name:     "session_token",
		Value:    req.IDToken,
		HttpOnly: true,
		Secure:   false, // Set to true in production with HTTPS
		SameSite: http.SameSiteStrictMode,
		MaxAge:   3600, // 1 hour
		Path:     "/",
	})
    

	// 5. Return user information
	resp := LoginResponse{
		Email:         userRecord.Email,
		Name:          userRecord.DisplayName,
		Picture:       userRecord.PhotoURL,
		EmailVerified: userRecord.EmailVerified,
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(resp)

	fmt.Printf(" Login successful for %s\n", userRecord.Email)
}

// ProtectedMeHandler returns current user information (protected route)
func ProtectedMeHandler(w http.ResponseWriter, r *http.Request) {
	// Get user from context (set by middleware)
	firebaseUser := r.Context().Value("firebaseUser")
	if firebaseUser == nil {
		respondError(w, "Unauthorized", http.StatusUnauthorized)
		return
	}

	token := firebaseUser.(*fbauth.Token)

	// Get user from database
	var user struct {
		ID            int64     `json:"id"`
		FirebaseUID   string    `json:"firebaseUid"`
		Email         string    `json:"email"`
		Name          string    `json:"name"`
		PictureURL    string    `json:"pictureUrl"`
		EmailVerified bool      `json:"emailVerified"`
		CreatedAt     time.Time `json:"createdAt"`
		LastLogin     time.Time `json:"lastLogin"`
	}

	err := config.DB.QueryRowContext(r.Context(),
		`SELECT id, firebase_uid, email, name, picture_url, email_verified, created_at, last_login
         FROM users
         WHERE firebase_uid = $1`,
		token.UID,
	).Scan(
		&user.ID,
		&user.FirebaseUID,
		&user.Email,
		&user.Name,
		&user.PictureURL,
		&user.EmailVerified,
		&user.CreatedAt,
		&user.LastLogin,
	)

	if err != nil {
		if err == sql.ErrNoRows {
			respondError(w, "User not found", http.StatusNotFound)
			return
		}
		fmt.Printf("❌ Database error: %v\n", err)
		respondError(w, "Database error", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(user)
}

// LogoutHandler clears the session cookie
func LogoutHandler(w http.ResponseWriter, r *http.Request) {
	http.SetCookie(w, &http.Cookie{
		Name:     "session_token",
		Value:    "",
		HttpOnly: true,
		Secure:   false,
		SameSite: http.SameSiteStrictMode,
		MaxAge:   -1, // Delete cookie
		Path:     "/",
	})

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"message": "Logged out successfully",
	})
}

// HealthHandler returns service health status
func HealthHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":  "healthy",
		"service": "auth",
		"time":    time.Now().Unix(),
	})
}

// respondError is a helper to send JSON error responses
func respondError(w http.ResponseWriter, message string, statusCode int) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(statusCode)
	json.NewEncoder(w).Encode(ErrorResponse{Error: message})
}
