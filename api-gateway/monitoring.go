package main

import (
	"fmt"
	"io"
	"net/http"

	"github.com/gorilla/mux"
)

const MONITORING_SERVICE_URL = "http://localhost:9000" // Changed from 7000 to 9000

type NodeMonitoring struct {
	TenantID           string  `json:"tenant_id"`
	Name               string  `json:"name"`
	Port               int     `json:"port"`
	Status             string  `json:"status"`
	KeyCount           int     `json:"key_count"`
	MemoryUsedBytes    int64   `json:"memory_used_bytes"`
	MemoryUsedMB       float64 `json:"memory_used_mb"`
	MemoryLimitMB      int     `json:"memory_limit_mb"`
	MemoryUsagePercent float64 `json:"memory_usage_percent"`
	RedisCLICommand    string  `json:"redis_cli_command"`
}

type RedisInfo struct {
	TenantID         string  `json:"tenant_id"`
	Port             int     `json:"port"`
	Connected        bool    `json:"connected"`
	UptimeSeconds    int64   `json:"uptime_seconds"`
	TotalCommands    int64   `json:"total_commands"`
	ConnectedClients int     `json:"connected_clients"`
	UsedMemoryHuman  string  `json:"used_memory_human"`
	UsedMemoryBytes  int64   `json:"used_memory_bytes"`
	OpsPerSec        float64 `json:"ops_per_sec"`
	TotalKeys        int     `json:"total_keys"`
}

// Get all nodes monitoring data
func getAllNodesHandler(w http.ResponseWriter, r *http.Request) {
	// Forward request to node manager
	resp, err := http.Get(MONITORING_SERVICE_URL + "/monitoring/nodes")
	if err != nil {
		http.Error(w, fmt.Sprintf("Failed to connect to monitoring service: %v", err), http.StatusServiceUnavailable)
		return
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		http.Error(w, "Failed to read response", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(resp.StatusCode)
	w.Write(body)
}

// Get detailed Redis info for a specific tenant
func getRedisInfoHandler(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	tenantID := vars["tenant_id"]

	// Forward request to node manager
	resp, err := http.Get(fmt.Sprintf("%s/monitoring/redis/%s", MONITORING_SERVICE_URL, tenantID))
	if err != nil {
		http.Error(w, fmt.Sprintf("Failed to connect to monitoring service: %v", err), http.StatusServiceUnavailable)
		return
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		http.Error(w, "Failed to read response", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(resp.StatusCode)
	w.Write(body)
}

// Register monitoring routes
func registerMonitoringRoutes(r *mux.Router) {
	r.HandleFunc("/api/monitoring/nodes", getAllNodesHandler).Methods("GET", "OPTIONS")
	r.HandleFunc("/api/monitoring/redis/{tenant_id}", getRedisInfoHandler).Methods("GET", "OPTIONS")
}
