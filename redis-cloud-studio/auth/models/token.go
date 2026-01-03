package models

import "time"


type RefreshToken struct {
	ID          int       `json:"id"`
	UserID      string    `json:"user_id"`      
	Token       string    `json:"token"`        
	AccessToken string    `json:"access_token"` 
	ExpiresAt   time.Time `json:"expires_at"`
	CreatedAt   time.Time `json:"created_at"`
	LastUsedAt  time.Time `json:"last_used_at"`
}
