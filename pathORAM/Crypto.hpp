#pragma once

#include "Types.hpp"
#include "BlockStore.hpp"

#include <array>
#include <cstdint>

// Encryption constants (in bytes)
constexpr int IV = 16;
constexpr int Key = 32;

/*
 * Performs encryption using AES-256-CBC
 * and provides various helper functions
 */
class AES {
	// Encrypts a plaintext into a given ciphertext buffer
	static int EncryptBytes(bytes<Key> key, bytes<IV> iv, byte_t *plaintext, int plen, byte_t *ciphertext);

	// Decrypts a ciphertext, putting the resulting plaintext into a given buffer
	static int DecryptBytes(bytes<Key> key, bytes<IV> iv, byte_t *ciphertext, int clen, byte_t *plaintext);
	
	// Encrypts/decrypts a block of data
	static block EncryptBlock(bytes<Key> key, bytes<IV> iv, block plaintext);
	static block DecryptBlock(bytes<Key> key, bytes<IV> iv, block ciphertext);

public:
	static void Setup();
	static void Cleanup();
	
	static uint32_t GetRandom(uint32_t range);

	// Probabilistically encrypts a block using a
	// random IV, and places it at the start of
	// the ciphertext
	static block Encrypt(bytes<Key> key, block b);

	// Decrypts a ciphertext which has the IV at
	// the beginning of it
	static block Decrypt(bytes<Key> key, block b);
	
	// Gets the length of the corresponding ciphertext
	// given the length of a plaintext
	static int GetCiphertextLength(int plen);
	
	// Generate a randomised initialisation vector
	static bytes<IV> GenerateIV();
};
