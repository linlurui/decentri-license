package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"runtime"
	"syscall"
	"time"

	"github.com/linlurui/decentrilicense/server/internal/handler"
	"github.com/linlurui/decentrilicense/server/internal/service"
)

func main() {
	// Parse command line flags
	port := flag.Int("port", 3883, "HTTP server port")
	workers := flag.Int("workers", runtime.NumCPU(), "Number of worker goroutines")
	flag.Parse()

	// Set GOMAXPROCS for optimal performance
	runtime.GOMAXPROCS(*workers)
	log.Printf("Using %d CPU cores", *workers)

	// Create service
	registryService := service.NewRegistryService()

	// Create API handler
	apiHandler := handler.NewAPIHandler(registryService)

	// Setup HTTP routes
	mux := http.NewServeMux()
	apiHandler.SetupRoutes(mux)

	// Create optimized server
	addr := fmt.Sprintf(":%d", *port)
	server := &http.Server{
		Addr:           addr,
		Handler:        loggingMiddleware(compressionMiddleware(mux)),
		ReadTimeout:    10 * time.Second,
		WriteTimeout:   10 * time.Second,
		IdleTimeout:    120 * time.Second,
		MaxHeaderBytes: 1 << 20, // 1MB
	}

	// Start server in goroutine
	go func() {
		log.Printf("Starting DecentriLicense Registry Server on %s", addr)
		log.Printf("Rate limit: 1000 req/s, Burst: 2000")
		if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatalf("Server error: %v", err)
		}
	}()

	// Wait for interrupt signal
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)
	<-sigChan

	log.Println("Shutting down server...")

	// Graceful shutdown with timeout
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()

	if err := server.Shutdown(ctx); err != nil {
		log.Printf("Server shutdown error: %v", err)
	}

	log.Println("Server stopped gracefully")
}

// loggingMiddleware logs HTTP requests (optimized)
func loggingMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// Only log non-health check requests to reduce overhead
		if r.URL.Path != "/api/health" {
			log.Printf("%s %s from %s", r.Method, r.URL.Path, r.RemoteAddr)
		}
		next.ServeHTTP(w, r)
	})
}

// compressionMiddleware adds gzip compression support
func compressionMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// Set common security headers
		w.Header().Set("X-Frame-Options", "DENY")
		w.Header().Set("X-XSS-Protection", "1; mode=block")
		w.Header().Set("X-Content-Type-Options", "nosniff")

		next.ServeHTTP(w, r)
	})
}
