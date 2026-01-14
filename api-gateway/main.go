package main

import (
	"bytes"
	"encoding/json"
	"io"
	"log"
	"net/http"
	"net/http/httputil"
	"net/url"
	"os"
	"strings"
	"time"
	

	"github.com/gorilla/mux"
)

func newProxy(target string) *httputil.ReverseProxy {
	u, err := url.Parse(target)
	if err != nil {
		log.Fatalf("Invalid proxy target: %s", target)
	}

	proxy := httputil.NewSingleHostReverseProxy(u)

	proxy.ModifyResponse = func(resp *http.Response) error {
		resp.Header.Del("Access-Control-Allow-Origin")
		resp.Header.Del("Access-Control-Allow-Credentials")
		resp.Header.Del("Access-Control-Allow-Headers")
		resp.Header.Del("Access-Control-Allow-Methods")
		return nil
	}

	return proxy
}

func corsMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		origin := r.Header.Get("Origin")
		if origin == "http://localhost:5173" {
			w.Header().Set("Access-Control-Allow-Origin", origin)
			w.Header().Set("Access-Control-Allow-Credentials", "true")
			w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
			w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
		}

		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}

		next.ServeHTTP(w, r)
	})
}

func checkAuth(w http.ResponseWriter, r *http.Request) bool {
	var sessionToken string
	cookieHeader := r.Header.Get("Cookie")

	if cookieHeader != "" {
		for _, cookie := range strings.Split(cookieHeader, ";") {
			parts := strings.SplitN(strings.TrimSpace(cookie), "=", 2)
			if len(parts) == 2 && parts[0] == "session_token" {
				sessionToken = parts[1]
				break
			}
		}
	}

	if sessionToken == "" {
		if cookie, err := r.Cookie("session_token"); err == nil {
			sessionToken = cookie.Value
		}
	}

	if sessionToken == "" {
		http.Error(w, `{"error":"unauthorized"}`, http.StatusUnauthorized)
		return false
	}

	authReq, err := http.NewRequest("GET", "http://localhost:8000/auth/me", nil)
	if err != nil {
		http.Error(w, `{"error":"internal error"}`, http.StatusInternalServerError)
		return false
	}

	authReq.Header.Set("Cookie", "session_token="+sessionToken)

	client := &http.Client{Timeout: 5 * time.Second}
	resp, err := client.Do(authReq)
	if err != nil {
		http.Error(w, `{"error":"auth service unavailable"}`, http.StatusServiceUnavailable)
		return false
	}
	defer resp.Body.Close()

	if resp.StatusCode == 401 {
		refreshReq, err := http.NewRequest("POST", "http://localhost:8000/auth/refresh", nil)
		if err != nil {
			http.Error(w, `{"error":"session expired"}`, http.StatusUnauthorized)
			return false
		}

		refreshReq.Header.Set("Cookie", r.Header.Get("Cookie"))

		refreshResp, err := client.Do(refreshReq)
		if err != nil {
			http.Error(w, `{"error":"session expired"}`, http.StatusUnauthorized)
			return false
		}
		defer refreshResp.Body.Close()

		if refreshResp.StatusCode != 200 {
			http.Error(w, `{"error":"session expired"}`, http.StatusUnauthorized)
			return false
		}

		for _, cookie := range refreshResp.Cookies() {
			http.SetCookie(w, cookie)
		}

		return true
	}

	if resp.StatusCode != 200 {
		http.Error(w, `{"error":"unauthorized"}`, http.StatusUnauthorized)
		return false
	}

	return true
}

func getUserId(r *http.Request) (string, error) {
	var sessionToken string
	if cookie, err := r.Cookie("session_token"); err == nil {
		sessionToken = cookie.Value
	}

	if sessionToken == "" {
		return "", http.ErrNoCookie
	}

	authReq, _ := http.NewRequest("GET", "http://localhost:8000/auth/me", nil)
	authReq.Header.Set("Cookie", "session_token="+sessionToken)

	client := &http.Client{Timeout: 5 * time.Second}
	resp, err := client.Do(authReq)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		return "", http.ErrNotSupported
	}

	var userData map[string]interface{}
	if err := json.NewDecoder(resp.Body).Decode(&userData); err != nil {
		return "", err
	}

	uid, ok := userData["uid"].(string)
	if !ok {
		return "", http.ErrNotSupported
	}

	return uid, nil
}

func main() {
	log.SetOutput(io.Discard)

	authProxy := newProxy("http://localhost:8000")
	backendProxy := newProxy("http://localhost:5500")
	nodeProxy := newProxy("http://localhost:7000")

	r := mux.NewRouter()

	r.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]string{"status": "ok", "service": "api-gateway"})
	}).Methods("GET")

	r.PathPrefix("/auth").Handler(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		authProxy.ServeHTTP(w, r)
	}))

	r.HandleFunc("/api/tenants", func(w http.ResponseWriter, r *http.Request) {
		if !checkAuth(w, r) {
			return
		}

		if r.Method == "GET" {
			uid, err := getUserId(r)
			if err != nil {
				http.Error(w, `{"error":"failed to get user id"}`, http.StatusUnauthorized)
				return
			}

			r.URL.Path = "/api/user/" + uid + "/tenants"
			backendProxy.ServeHTTP(w, r)
			return
		}

		if r.Method == "POST" {
			uid, err := getUserId(r)
			if err != nil {
				http.Error(w, `{"error":"failed to get user id"}`, http.StatusUnauthorized)
				return
			}

			body, _ := io.ReadAll(r.Body)
			var data map[string]interface{}
			json.Unmarshal(body, &data)
			data["firebase_uid"] = uid

			newBody, _ := json.Marshal(data)
			r.Body = io.NopCloser(bytes.NewBuffer(newBody))
			r.ContentLength = int64(len(newBody))

			backendProxy.ServeHTTP(w, r)
			return
		}

		backendProxy.ServeHTTP(w, r)
	}).Methods("GET", "POST", "OPTIONS")

	r.HandleFunc("/api/tenants/{id}", func(w http.ResponseWriter, r *http.Request) {
		if !checkAuth(w, r) {
			return
		}

		backendProxy.ServeHTTP(w, r)
	}).Methods("GET", "PUT", "DELETE", "OPTIONS")

	r.PathPrefix("/api/apikeys").Handler(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if !checkAuth(w, r) {
			return
		}

		backendProxy.ServeHTTP(w, r)
	})).Methods("GET", "POST", "DELETE", "OPTIONS")

	r.PathPrefix("/api/nodes").Handler(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if !checkAuth(w, r) {
			return
		}

		newPath := strings.Replace(r.URL.Path, "/api/nodes", "/node", 1)
		r.URL.Path = newPath

		nodeProxy.ServeHTTP(w, r)
	})).Methods("GET", "POST", "DELETE", "OPTIONS")

	registerMonitoringRoutes(r)

	log.SetOutput(os.Stdout)
	log.Println("API Gateway running on :8080")
	log.SetOutput(io.Discard)

	log.Fatal(http.ListenAndServe(":8080", corsMiddleware(r)))
}
