package decenlicense

import (
	"encoding/json"
	"fmt"
	"os"
	"testing"
)

func getTestFile(path string) string {
	data, err := os.ReadFile(path)
	if err != nil {
		return ""
	}
	return string(data)
}

func newTestClient(t *testing.T) *Client {
	client, err := NewClient()
	if err != nil {
		t.Fatalf("NewClient failed: %v", err)
	}
	t.Cleanup(func() { client.Shutdown() })
	err = client.Initialize(Config{})
	if err != nil {
		t.Fatalf("Initialize failed: %v", err)
	}
	return client
}

func TestClient_ImportAndValidate_RSA(t *testing.T) {
	tokenJSON := getTestFile("../../dl-issuer/token_windsurf-free_AUTO-GENERATED-LICENSE-2026-010-INE5MH_20260427031124.json")
	productKey := getTestFile("../../dl-issuer/public_windsurf-free_20260424141144.pem")
	if tokenJSON == "" || productKey == "" {
		t.Skip("RSA test files not found")
	}

	client := newTestClient(t)
	client.SetProductPublicKey(productKey)

	err := client.ImportToken(tokenJSON)
	if err != nil {
		t.Fatalf("ImportToken RSA failed: %v", err)
	}

	result, err := client.ValidateToken(tokenJSON)
	if err != nil {
		t.Fatalf("ValidateToken RSA failed: %v", err)
	}
	if !result.Valid {
		t.Errorf("RSA token validation failed: %s", result.ErrorMessage)
	}

	var tokenData map[string]interface{}
	json.Unmarshal([]byte(tokenJSON), &tokenData)
	if tokenData["alg"] != "RSA" {
		t.Errorf("expected alg=RSA, got %v", tokenData["alg"])
	}
	fmt.Println("✅ Go SDK: RSA token import+validate passed")
}

func TestClient_ImportAndValidate_Ed25519(t *testing.T) {
	tokenJSON := getTestFile("../../dl-issuer/token_test-ed25519_AUTO-GENERATED-LICENSE-2026-012-G7WJ5G_20260427031313.json")
	productKey := getTestFile("../../dl-issuer/public_test-ed25519_20260427031239.pem")
	if tokenJSON == "" || productKey == "" {
		t.Skip("Ed25519 test files not found")
	}

	client := newTestClient(t)
	client.SetProductPublicKey(productKey)

	err := client.ImportToken(tokenJSON)
	if err != nil {
		t.Fatalf("ImportToken Ed25519 failed: %v", err)
	}

	result, err := client.ValidateToken(tokenJSON)
	if err != nil {
		t.Fatalf("ValidateToken Ed25519 failed: %v", err)
	}
	if !result.Valid {
		t.Errorf("Ed25519 token validation failed: %s", result.ErrorMessage)
	}

	var tokenData map[string]interface{}
	json.Unmarshal([]byte(tokenJSON), &tokenData)
	if tokenData["alg"] != "Ed25519" {
		t.Errorf("expected alg=Ed25519, got %v", tokenData["alg"])
	}
	fmt.Println("✅ Go SDK: Ed25519 token import+validate passed")
}

func TestClient_EncryptedToken_RSA(t *testing.T) {
	encryptedToken := getTestFile("../../dl-issuer/token_windsurf-free_AUTO-GENERATED-LICENSE-2026-010-INE5MH_20260427031124_encrypted.txt")
	productKey := getTestFile("../../dl-issuer/public_windsurf-free_20260424141144.pem")
	if encryptedToken == "" || productKey == "" {
		t.Skip("RSA encrypted token file not found")
	}

	client := newTestClient(t)
	client.SetProductPublicKey(productKey)

	err := client.ImportToken(encryptedToken)
	if err != nil {
		t.Fatalf("ImportToken encrypted RSA failed: %v", err)
	}

	result, err := client.ValidateToken(encryptedToken)
	if err != nil {
		t.Fatalf("ValidateToken encrypted RSA failed: %v", err)
	}
	if !result.Valid {
		t.Errorf("Encrypted RSA token validation failed: %s", result.ErrorMessage)
	}
	fmt.Println("✅ Go SDK: Encrypted RSA token import+validate passed")
}

func TestClient_EncryptedToken_Ed25519(t *testing.T) {
	encryptedToken := getTestFile("../../dl-issuer/token_test-ed25519_AUTO-GENERATED-LICENSE-2026-012-G7WJ5G_20260427031313_encrypted.txt")
	productKey := getTestFile("../../dl-issuer/public_test-ed25519_20260427031239.pem")
	if encryptedToken == "" || productKey == "" {
		t.Skip("Ed25519 encrypted token file not found")
	}

	client := newTestClient(t)
	client.SetProductPublicKey(productKey)

	err := client.ImportToken(encryptedToken)
	if err != nil {
		t.Fatalf("ImportToken encrypted Ed25519 failed: %v", err)
	}

	result, err := client.ValidateToken(encryptedToken)
	if err != nil {
		t.Fatalf("ValidateToken encrypted Ed25519 failed: %v", err)
	}
	if !result.Valid {
		t.Errorf("Encrypted Ed25519 token validation failed: %s", result.ErrorMessage)
	}
	fmt.Println("✅ Go SDK: Encrypted Ed25519 token import+validate passed")
}

func TestClient_WrongKeyFails(t *testing.T) {
	tokenJSON := getTestFile("../../dl-issuer/token_windsurf-free_AUTO-GENERATED-LICENSE-2026-010-INE5MH_20260427031124.json")
	productKey := getTestFile("../../dl-issuer/public_test-ed25519_20260427031239.pem")
	if tokenJSON == "" || productKey == "" {
		t.Skip("test files not found")
	}

	client := newTestClient(t)
	client.SetProductPublicKey(productKey)

	err := client.ImportToken(tokenJSON)
	if err != nil {
		fmt.Println("✅ Go SDK: Wrong key import correctly rejected")
		return
	}

	result, _ := client.ValidateToken(tokenJSON)
	if result.Valid {
		t.Error("RSA token should NOT validate with Ed25519 product key")
	}
	fmt.Println("✅ Go SDK: Wrong key validation correctly failed")
}
