package handler

import (
	"auth/config"
	"auth/middleware"
	"auth/utils"
	"encoding/json"
	"net/http"
	"time"
)

type GoogleLoginRequest struct {
	IDToken string `json:"idToken"`
}

type LoginResponse struct {
	Email         string `json:"email"`
	Name          string `json:"name"`
	Picture       string `json:"picture,omitempty"`
	EmailVerified bool   `json:"emailVerified"`
}

type ErrorResponse struct {
	Error string `json:"error"`
}

func respondError(w http.ResponseWriter, message string, status int) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(ErrorResponse{Error: message})
}

func GoogleLoginHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodOptions {
		w.WriteHeader(http.StatusOK)
		return
	}

	ctx := r.Context()

	var req GoogleLoginRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		respondError(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if req.IDToken == "" {
		respondError(w, "idToken is required", http.StatusBadRequest)
		return
	}

	token, err := config.FirebaseAuth.VerifyIDToken(ctx, req.IDToken)
	if err != nil {
		respondError(w, "Invalid or expired token", http.StatusUnauthorized)
		return
	}

	userRecord, err := config.FirebaseAuth.GetUser(ctx, token.UID)
	if err != nil {
		respondError(w, "Failed to retrieve user information", http.StatusInternalServerError)
		return
	}

	if config.DB != nil {
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
			respondError(w, "Database error", http.StatusInternalServerError)
			return
		}
	}

	refreshToken, err := utils.GenerateRefreshToken()
	if err != nil {
		respondError(w, "Token generation failed", http.StatusInternalServerError)
		return
	}

	_, err = config.DB.Exec(`
        INSERT INTO refresh_tokens (user_id, token, access_token, expires_at, created_at, last_used_at)
        VALUES ($1, $2, $3, $4, NOW(), NOW())
    `, token.UID, refreshToken, req.IDToken, time.Now().Add(utils.RefreshTokenDuration))

	if err != nil {
		respondError(w, "Token storage failed", http.StatusInternalServerError)
		return
	}

	http.SetCookie(w, &http.Cookie{
		Name:     "session_token",
		Value:    req.IDToken,
		HttpOnly: true,
		Secure:   false,
		SameSite: http.SameSiteLaxMode,
		MaxAge:   3600,
		Path:     "/",
	})

	http.SetCookie(w, &http.Cookie{
		Name:     "refresh_token",
		Value:    refreshToken,
		HttpOnly: true,
		Secure:   false,
		SameSite: http.SameSiteLaxMode,
		MaxAge:   int(utils.RefreshTokenDuration.Seconds()),
		Path:     "/",
	})

	resp := LoginResponse{
		Email:         userRecord.Email,
		Name:          userRecord.DisplayName,
		Picture:       userRecord.PhotoURL,
		EmailVerified: userRecord.EmailVerified,
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(resp)
}

func ProtectedMeHandler(w http.ResponseWriter, r *http.Request) {
	token := middleware.FromContext(r.Context())
	if token == nil {
		w.Header().Set("Content-Type", "application/json")
		http.Error(w, `{"error":"Unauthorized"}`, http.StatusUnauthorized)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"uid":           token.UID,
		"email":         token.Claims["email"],
		"name":          token.Claims["name"],
		"picture":       token.Claims["picture"],
		"emailVerified": token.Claims["email_verified"],
	})
}

func LogoutHandler(w http.ResponseWriter, r *http.Request) {
	cookie, err := r.Cookie("refresh_token")
	if err == nil && config.DB != nil {
		config.DB.Exec(`DELETE FROM refresh_tokens WHERE token = $1`, cookie.Value)
	}

	http.SetCookie(w, &http.Cookie{
		Name:     "session_token",
		Value:    "",
		HttpOnly: true,
		MaxAge:   -1,
		Path:     "/",
	})

	http.SetCookie(w, &http.Cookie{
		Name:     "refresh_token",
		Value:    "",
		HttpOnly: true,
		MaxAge:   -1,
		Path:     "/",
	})

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{"message": "Logged out"})
}

func HealthHandler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{"status": "ok"})
}
