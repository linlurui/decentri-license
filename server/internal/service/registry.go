package service

import (
	"fmt"
	"log"
	"strings"
	"time"

	"github.com/linlurui/decentrilicense/server/internal/model"
)

// RegistryService manages device registrations and license coordination
type RegistryService struct {
	registry *model.DeviceRegistry
}

// NewRegistryService creates a new registry service
func NewRegistryService() *RegistryService {
	service := &RegistryService{
		registry: model.NewDeviceRegistry(),
	}

	// Start background cleanup task
	go service.cleanupLoop()

	return service
}

// RegisterDevice registers a new device
func (s *RegistryService) RegisterDevice(device *model.Device) error {
	s.registry.Register(device)
	log.Printf("Device registered: %s (license: %s, IP: %s)",
		device.DeviceID[:16], device.LicenseCode, device.PublicIP)
	return nil
}

// DeviceHeartbeat updates device heartbeat
func (s *RegistryService) DeviceHeartbeat(deviceID string) bool {
	return s.registry.Heartbeat(deviceID)
}

// GetLicenseHolder returns the current holder of a license
func (s *RegistryService) GetLicenseHolder(licenseCode string) (*model.Device, bool) {
	return s.registry.GetLicenseHolder(licenseCode)
}

// GetDevice returns device information
func (s *RegistryService) GetDevice(deviceID string) (*model.Device, bool) {
	return s.registry.GetDevice(deviceID)
}

// GetStats returns registry statistics
func (s *RegistryService) GetStats() model.RegistryStats {
	return s.registry.GetStats()
}

// RequestTokenTransfer processes a token transfer request
func (s *RegistryService) RequestTokenTransfer(transfer *model.TokenTransfer) error {
	log.Printf("Token transfer request: %s -> %s (license: %s, token: %s)",
		transfer.FromDevice[:16], transfer.ToDevice[:16], transfer.LicenseCode, transfer.TokenID[:16])

	// Validate the transfer request
	if err := s.validateTokenTransfer(transfer); err != nil {
		return err
	}

	// Update license holder if transfer is valid
	s.registry.UpdateLicenseHolder(transfer.LicenseCode, transfer.ToDevice)

	log.Printf("Token transfer approved: license %s now held by %s", transfer.LicenseCode, transfer.ToDevice[:16])
	return nil
}

// validateTokenTransfer validates a token transfer request
func (s *RegistryService) validateTokenTransfer(transfer *model.TokenTransfer) error {
	// Check if FromDevice currently holds the license
	currentHolder, exists := s.registry.GetLicenseHolder(transfer.LicenseCode)
	if !exists {
		return fmt.Errorf("license not found: %s", transfer.LicenseCode)
	}

	if currentHolder.DeviceID != transfer.FromDevice {
		return fmt.Errorf("device %s does not hold license %s (current holder: %s)",
			transfer.FromDevice, transfer.LicenseCode, currentHolder.DeviceID)
	}

	// Check if ToDevice exists and is active
	toDevice, exists := s.registry.GetDevice(transfer.ToDevice)
	if !exists {
		return fmt.Errorf("target device not registered: %s", transfer.ToDevice)
	}

	// Check if target device is recently active
	if time.Since(toDevice.LastSeen) > 10*time.Minute {
		return fmt.Errorf("target device not recently active: %s", transfer.ToDevice)
	}

	// Basic token validation (if token data is provided)
	if transfer.TokenData != "" {
		if err := s.validateTokenData(transfer.TokenData, transfer.LicenseCode); err != nil {
			return fmt.Errorf("token validation failed: %v", err)
		}
	}

	return nil
}

// validateTokenData performs basic token validation
func (s *RegistryService) validateTokenData(tokenData, expectedLicense string) error {
	// This is a simplified validation - in production, this would:
	// 1. Parse token JSON
	// 2. Verify RSA signature
	// 3. Check token integrity
	// 4. Validate against root/product keys

	// For now, just check if license code matches
	if !strings.Contains(tokenData, expectedLicense) {
		return fmt.Errorf("token license code mismatch")
	}

	return nil
}

// cleanupLoop periodically cleans up inactive devices
func (s *RegistryService) cleanupLoop() {
	ticker := time.NewTicker(1 * time.Minute)
	defer ticker.Stop()

	for range ticker.C {
		count := s.registry.CleanupInactive(5 * time.Minute)
		if count > 0 {
			log.Printf("Cleaned up %d inactive devices", count)
		}
	}
}
