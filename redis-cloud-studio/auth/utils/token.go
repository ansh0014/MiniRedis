package utils

import (
	"crypto/rand"
	"encoding/base64"
	"time"
)

func GenerateRefreshToken() (string, error) {
	b := make([]byte, 32)
	_, err := rand.Read(b)
	if err != nil {
		return "", err
	}
	return base64.URLEncoding.EncodeToString(b), nil
}


const (
	AccessTokenDuration  = 1 * time.Hour      
	RefreshTokenDuration = 7 * 24 * time.Hour 
)