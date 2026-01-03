package handler

import (
	"auth/config"
	"auth/utils"
	"bytes"
	"database/sql"
	"encoding/json"
	"net/http"
	"os"
	"time"
)

type RefreshRequest struct {
	RefreshToken string `json:"refreshToken"`
}

type RefreshResponse struct {
	AccessToken string `json:"accessToken"`
	ExpiresIn   int    `json:"expiresIn"`
}

type FirebaseTokenExchangeRequest struct {
	Token             string `json:"token"`
	ReturnSecureToken bool   `json:"returnSecureToken"`
}

type FirebaseTokenExchangeResponse struct {
	IDToken      string `json:"idToken"`
	RefreshToken string `json:"refreshToken"`
	ExpiresIn    string `json:"expiresIn"`
}

func exchangeCustomTokenForIDToken(customToken string) (string, error) {
	apiKey := os.Getenv("FIREBASE_API_KEY")
	if apiKey == "" {
		return "", http.ErrNotSupported
	}

	url := "https://identitytoolkit.googleapis.com/v1/accounts:signInWithCustomToken?key=" + apiKey

	reqBody := FirebaseTokenExchangeRequest{
		Token:             customToken,
		ReturnSecureToken: true,
	}

	jsonData, err := json.Marshal(reqBody)
	if err != nil {
		return "", err
	}

	resp, err := http.Post(url, "application/json", bytes.NewBuffer(jsonData))
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return "", http.ErrNotSupported
	}

	var tokenResp FirebaseTokenExchangeResponse
	if err := json.NewDecoder(resp.Body).Decode(&tokenResp); err != nil {
		return "", err
	}

	return tokenResp.IDToken, nil
}

func RefreshTokenHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodOptions {
		w.WriteHeader(http.StatusOK)
		return
	}

	cookie, err := r.Cookie("refresh_token")
	if err != nil {
		http.Error(w, `{"error":"No refresh token"}`, http.StatusUnauthorized)
		return
	}

	refreshToken := cookie.Value

	var userID, accessToken string
	var expiresAt time.Time

	err = config.DB.QueryRow(`
        SELECT user_id, access_token, expires_at 
        FROM refresh_tokens 
        WHERE token = $1 AND expires_at > NOW()
    `, refreshToken).Scan(&userID, &accessToken, &expiresAt)

	if err == sql.ErrNoRows {
		http.Error(w, `{"error":"Invalid refresh token"}`, http.StatusUnauthorized)
		return
	}
	if err != nil {
		http.Error(w, `{"error":"Database error"}`, http.StatusInternalServerError)
		return
	}

	_, err = config.FirebaseAuth.GetUser(r.Context(), userID)
	if err != nil {
		http.Error(w, `{"error":"User not found"}`, http.StatusUnauthorized)
		return
	}

	newAccessToken := accessToken

	if time.Until(expiresAt) < 5*time.Minute {
		customToken, err := config.FirebaseAuth.CustomToken(r.Context(), userID)
		if err != nil {
			http.Error(w, `{"error":"Token generation failed"}`, http.StatusInternalServerError)
			return
		}

		idToken, err := exchangeCustomTokenForIDToken(customToken)
		if err != nil {
			http.Error(w, `{"error":"Token exchange failed"}`, http.StatusInternalServerError)
			return
		}

		newAccessToken = idToken

		config.DB.Exec(`
            UPDATE refresh_tokens 
            SET access_token = $2,
                expires_at = $3,
                last_used_at = NOW()
            WHERE token = $1
        `, refreshToken, newAccessToken, time.Now().Add(1*time.Hour))
	} else {
		config.DB.Exec(`
            UPDATE refresh_tokens 
            SET last_used_at = NOW()
            WHERE token = $1
        `, refreshToken)
	}

	http.SetCookie(w, &http.Cookie{
		Name:     "session_token",
		Value:    newAccessToken,
		HttpOnly: true,
		Secure:   true,
		SameSite: http.SameSiteStrictMode,
		MaxAge:   int(utils.AccessTokenDuration.Seconds()),
		Path:     "/",
	})

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(RefreshResponse{
		AccessToken: newAccessToken,
		ExpiresIn:   int(utils.AccessTokenDuration.Seconds()),
	})
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}
