package model

import (
	"sync"
	"sync/atomic"
	"time"
)

// Device represents a registered device
type Device struct {
	DeviceID    string    `json:"device_id"`
	LicenseCode string    `json:"license_code"`
	PublicIP    string    `json:"public_ip"`
	TCPPort     int       `json:"tcp_port"`
	LastSeen    time.Time `json:"last_seen"`
}

// TokenTransfer represents a token transfer request
type TokenTransfer struct {
	TokenID      string    `json:"token_id"`
	TokenData    string    `json:"token_data,omitempty"`    // JSON serialized token
	FromDevice   string    `json:"from_device"`
	ToDevice     string    `json:"to_device"`
	LicenseCode  string    `json:"license_code"`
	Timestamp    time.Time `json:"timestamp"`
}

// DeviceRegistry manages device registrations with high concurrency support
type DeviceRegistry struct {
	devices  sync.Map // device_id -> *Device (lock-free map)
	licenses sync.Map // license_code -> string (device_id holder)
	stats    *RegistryStats
}

// RegistryStats tracks registry statistics
type RegistryStats struct {
	TotalDevices   int64
	TotalLicenses  int64
	Registrations  int64
	Heartbeats     int64
	Queries        int64
}

// NewDeviceRegistry creates a new registry with optimized concurrent access
func NewDeviceRegistry() *DeviceRegistry {
	return &DeviceRegistry{
		stats: &RegistryStats{},
	}
}

// Register registers or updates a device with lock-free operation
func (r *DeviceRegistry) Register(device *Device) {
	device.LastSeen = time.Now()
	
	// Check if new device
	_, isNew := r.devices.LoadOrStore(device.DeviceID, device)
	if isNew {
		atomic.AddInt64(&r.stats.TotalDevices, 1)
	} else {
		r.devices.Store(device.DeviceID, device)
	}
	
	// Update license holder
	_, licenseExists := r.licenses.LoadOrStore(device.LicenseCode, device.DeviceID)
	if !licenseExists {
		atomic.AddInt64(&r.stats.TotalLicenses, 1)
	} else {
		r.licenses.Store(device.LicenseCode, device.DeviceID)
	}
	
	atomic.AddInt64(&r.stats.Registrations, 1)
}

// Heartbeat updates device last seen time with optimistic locking
func (r *DeviceRegistry) Heartbeat(deviceID string) bool {
	val, exists := r.devices.Load(deviceID)
	if !exists {
		return false
	}
	
	device := val.(*Device)
	device.LastSeen = time.Now()
	r.devices.Store(deviceID, device)
	
	atomic.AddInt64(&r.stats.Heartbeats, 1)
	return true
}

// GetLicenseHolder gets the current holder of a license (lock-free read)
func (r *DeviceRegistry) GetLicenseHolder(licenseCode string) (*Device, bool) {
	val, exists := r.licenses.Load(licenseCode)
	if !exists {
		return nil, false
	}

	deviceID := val.(string)
	deviceVal, exists := r.devices.Load(deviceID)
	if !exists {
		return nil, false
	}

	atomic.AddInt64(&r.stats.Queries, 1)
	return deviceVal.(*Device), true
}

// UpdateLicenseHolder updates which device holds a license (lock-free)
func (r *DeviceRegistry) UpdateLicenseHolder(licenseCode, deviceID string) {
	r.licenses.Store(licenseCode, deviceID)
	atomic.AddInt64(&r.stats.Registrations, 1)
}

// GetDevice gets a device by ID (lock-free read)
func (r *DeviceRegistry) GetDevice(deviceID string) (*Device, bool) {
	val, exists := r.devices.Load(deviceID)
	if !exists {
		return nil, false
	}
	
	atomic.AddInt64(&r.stats.Queries, 1)
	return val.(*Device), true
}

// CleanupInactive removes inactive devices (optimized for concurrent access)
func (r *DeviceRegistry) CleanupInactive(timeout time.Duration) int {
	count := 0
	now := time.Now()
	
	// Use Range for concurrent-safe iteration
	r.devices.Range(func(key, value interface{}) bool {
		deviceID := key.(string)
		device := value.(*Device)
		
		if now.Sub(device.LastSeen) > timeout {
			// Delete device
			r.devices.Delete(deviceID)
			
			// Check and remove from licenses
			if holderID, exists := r.licenses.Load(device.LicenseCode); exists && holderID.(string) == deviceID {
				r.licenses.Delete(device.LicenseCode)
				atomic.AddInt64(&r.stats.TotalLicenses, -1)
			}
			
			atomic.AddInt64(&r.stats.TotalDevices, -1)
			count++
		}
		
		return true // Continue iteration
	})
	
	return count
}

// GetStats returns current registry statistics
func (r *DeviceRegistry) GetStats() RegistryStats {
	return RegistryStats{
		TotalDevices:   atomic.LoadInt64(&r.stats.TotalDevices),
		TotalLicenses:  atomic.LoadInt64(&r.stats.TotalLicenses),
		Registrations:  atomic.LoadInt64(&r.stats.Registrations),
		Heartbeats:     atomic.LoadInt64(&r.stats.Heartbeats),
		Queries:        atomic.LoadInt64(&r.stats.Queries),
	}
}
