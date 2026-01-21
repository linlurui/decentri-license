package handler

import (
	"context"
	"encoding/json"
	"log"
	"net/http"
	"sync"
	"time"

	"github.com/linlurui/decentrilicense/server/internal/model"
	"github.com/linlurui/decentrilicense/server/internal/service"
	"golang.org/x/time/rate"
)

// APIHandler handles HTTP API requests with rate limiting and connection pooling
type APIHandler struct {
	service      *service.RegistryService
	limiter      *rate.Limiter
	responsePool sync.Pool
	reqTimeout   time.Duration
}

// NewAPIHandler creates a new API handler with optimizations
func NewAPIHandler(service *service.RegistryService) *APIHandler {
	return &APIHandler{
		service:    service,
		limiter:    rate.NewLimiter(rate.Limit(1000), 2000), // 1000 req/s burst 2000
		reqTimeout: 5 * time.Second,
		responsePool: sync.Pool{
			New: func() interface{} {
				return make(map[string]interface{})
			},
		},
	}
}

// SetupRoutes sets up HTTP routes with middleware
func (h *APIHandler) SetupRoutes(mux *http.ServeMux) {
	mux.HandleFunc("/api/health", h.rateLimitMiddleware(h.handleHealth))
	mux.HandleFunc("/api/stats", h.rateLimitMiddleware(h.handleStats))
	mux.HandleFunc("/api/devices/register", h.rateLimitMiddleware(h.handleRegister))
	mux.HandleFunc("/api/devices/heartbeat", h.rateLimitMiddleware(h.handleHeartbeat))
	mux.HandleFunc("/api/licenses/", h.rateLimitMiddleware(h.handleGetLicenseHolder))
	mux.HandleFunc("/api/tokens/transfer", h.rateLimitMiddleware(h.handleTokenTransfer))
}

// rateLimitMiddleware applies rate limiting to requests
func (h *APIHandler) rateLimitMiddleware(next http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		ctx, cancel := context.WithTimeout(r.Context(), h.reqTimeout)
		defer cancel()

		if !h.limiter.Allow() {
			http.Error(w, "Rate limit exceeded", http.StatusTooManyRequests)
			return
		}

		next.ServeHTTP(w, r.WithContext(ctx))
	}
}

// writeJSONResponse writes JSON response using pooled objects
func (h *APIHandler) writeJSONResponse(w http.ResponseWriter, data interface{}) {
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("X-Content-Type-Options", "nosniff")
	json.NewEncoder(w).Encode(data)
}

// getResponseMap gets a response map from pool
func (h *APIHandler) getResponseMap() map[string]interface{} {
	return h.responsePool.Get().(map[string]interface{})
}

// putResponseMap returns response map to pool
func (h *APIHandler) putResponseMap(m map[string]interface{}) {
	for k := range m {
		delete(m, k)
	}
	h.responsePool.Put(m)
}

// handleStats handles statistics query
func (h *APIHandler) handleStats(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	stats := h.service.GetStats()
	h.writeJSONResponse(w, stats)
}

// handleHealth handles health check requests with optimized response
func (h *APIHandler) handleHealth(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	response := h.getResponseMap()
	defer h.putResponseMap(response)

	response["status"] = "healthy"
	response["service"] = "decentrilicense-registry"
	response["timestamp"] = time.Now().Unix()

	h.writeJSONResponse(w, response)
}

// handleRegister handles device registration with pooled responses
func (h *APIHandler) handleRegister(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var device model.Device
	decoder := json.NewDecoder(r.Body)
	decoder.DisallowUnknownFields()

	if err := decoder.Decode(&device); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	// Get client IP if not provided
	if device.PublicIP == "" {
		device.PublicIP = r.RemoteAddr
	}

	if err := h.service.RegisterDevice(&device); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	response := h.getResponseMap()
	defer h.putResponseMap(response)

	response["success"] = true
	response["message"] = "Device registered successfully"
	response["device_id"] = device.DeviceID

	h.writeJSONResponse(w, response)
}

// handleHeartbeat handles heartbeat requests with pooled responses
func (h *APIHandler) handleHeartbeat(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req struct {
		DeviceID string `json:"device_id"`
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	success := h.service.DeviceHeartbeat(req.DeviceID)

	response := h.getResponseMap()
	defer h.putResponseMap(response)

	response["success"] = success

	h.writeJSONResponse(w, response)
}

// handleGetLicenseHolder handles license holder query with caching support
func (h *APIHandler) handleGetLicenseHolder(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	// Extract license code from URL path
	// URL format: /api/licenses/{code}/holder
	licenseCode := r.URL.Path[len("/api/licenses/"):]
	if len(licenseCode) > 7 && licenseCode[len(licenseCode)-7:] == "/holder" {
		licenseCode = licenseCode[:len(licenseCode)-7]
	}

	device, exists := h.service.GetLicenseHolder(licenseCode)

	if !exists {
		http.Error(w, "License not found", http.StatusNotFound)
		return
	}

	// Add cache control header for frequently accessed data
	w.Header().Set("Cache-Control", "public, max-age=10")
	h.writeJSONResponse(w, device)
}

// handleTokenTransfer handles token transfer coordination with pooled responses
func (h *APIHandler) handleTokenTransfer(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var transfer model.TokenTransfer
	if err := json.NewDecoder(r.Body).Decode(&transfer); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if err := h.service.RequestTokenTransfer(&transfer); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	response := h.getResponseMap()
	defer h.putResponseMap(response)

	response["success"] = true
	response["message"] = "Transfer request recorded"

	h.writeJSONResponse(w, response)

	log.Printf("Token transfer: %s -> %s", transfer.FromDevice, transfer.ToDevice)
}
