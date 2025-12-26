package handler

import (
    
    "encoding/json"
    "net/http"

    "auth/config" // adjust module path if needed
)

// GoogleLoginHandler accepts a JSON body { "idToken": "..." }, verifies it with
// Firebase and returns basic user info. Use this from the client after Google Sign-in.
func GoogleLoginHandler(w http.ResponseWriter, r *http.Request) {
    ctx := r.Context()

    var body struct {
        IDToken string `json:"idToken"`
    }
    if err := json.NewDecoder(r.Body).Decode(&body); err != nil || body.IDToken == "" {
        http.Error(w, "invalid request payload", http.StatusBadRequest)
        return
    }

    // verify id token
    token, err := config.FirebaseAuth.VerifyIDToken(ctx, body.IDToken)
    if err != nil {
        http.Error(w, "token verification failed: "+err.Error(), http.StatusUnauthorized)
        return
    }

    // optional: fetch user record for more details
    userRecord, _ := config.FirebaseAuth.GetUser(ctx, token.UID)

    resp := map[string]interface{}{
        "uid":         token.UID,
        "email":       token.Claims["email"],
        "emailVerified": token.Claims["email_verified"],
        "name":        userRecord.DisplayName,
        "picture":     userRecord.PhotoURL,
        "claims":      token.Claims,
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(resp)
}

// ProtectedMeHandler example - requires FirebaseAuthMiddleware to have run.
func ProtectedMeHandler(w http.ResponseWriter, r *http.Request) {
    // token stored in context by middleware
    tok := r.Context().Value("firebaseUser")
    if tok == nil {
        http.Error(w, "unauthorized", http.StatusUnauthorized)
        return
    }
    // return token claims (or fetch user record)
    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(tok)
}