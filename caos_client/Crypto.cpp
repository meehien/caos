#include "Crypto.hpp"
#include "Log.hpp"

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/conf.h>
#include <openssl/rand.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void AES::Setup()
{
	// Initialise OpenSSL
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	OPENSSL_config(NULL);
}

void AES::Cleanup()
{
	EVP_cleanup();
	ERR_free_strings();
}

uint32_t AES::GetRandom(uint32_t range)
{
	uint8_t buffer[4];
	
	if( !(RAND_bytes((uint8_t *)buffer, sizeof(buffer)))) {
		Log::Write(Log::FATAL,"Needs more entropy");
	}
	uint32_t rand = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]);
	return (abs((int)rand)%(uint32_t)range);
}


int AES::EncryptBytes(bytes<Key> key, bytes<IV> iv, byte_t *plaintext, int plen, byte_t *ciphertext)
{
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	
	if (!ctx) {
		Log::Write(Log::FATAL,"Failed to create new cipher");
	}
	
	// Initialise the encryption operation
	if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data()) != 1) {
		Log::Write(Log::FATAL,"Failed to initialise encryption");
	}
	
	// Encrypt
	int len;
	if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plen) != 1) {
		Log::Write(Log::FATAL,"Failed to complete EncryptUpdate");
	}
	
	int clen = len;
	
	if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
		Log::Write(Log::FATAL,"Failed to complete EncryptFinal");
	}
	clen += len;
	
	EVP_CIPHER_CTX_free(ctx);
	
	return clen;
}

int AES::DecryptBytes(bytes<Key> key, bytes<IV> iv, byte_t *ciphertext, int clen, byte_t *plaintext)
{
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	
	if (!ctx) {
		Log::Write(Log::FATAL,"Failed to create new cipher");
	}
	
	// Initialise the decryption operation
	if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data()) != 1) {
		Log::Write(Log::FATAL,"Failed to initialise decryption");
	}
	
	// Decrypt
	int len;
	if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, clen) != 1) {
		Log::Write(Log::WARNING,"Failed to complete DecryptUpdate");
		return -1;
	}
	
	int plen = len;
	
	if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) {
		return -1;
	}
	plen += len;
	
	EVP_CIPHER_CTX_free(ctx);
	
	return plen;
}

blockData AES::EncryptBlock(bytes<Key> key, bytes<IV> iv, blockData plaintext)
{
	int clen = GetCiphertextLength(plaintext.size());
	blockData ciphertext(clen);

	int plen = plaintext.size();
	EncryptBytes(key, iv, plaintext.data(), plen, ciphertext.data());
	
	return ciphertext;
}

blockData AES::DecryptBlock(bytes<Key> key, bytes<IV> iv, blockData ciphertext, size_t blockSize)
{
	int clen = ciphertext.size();
	blockData plaintext(clen);

	int plen = DecryptBytes(key, iv, ciphertext.data(), clen, plaintext.data());

	// Trim plaintext to actual size
	if (plen==-1){
		return Block(-1, NUM_CLIENTS, 0, blockData(blockSize)).toBytes(); //TODO: cleaner method?
	}
	plaintext.resize(plen);

	return plaintext;
}

blockData AES::Encrypt(bytes<Key> key, blockData plaintext)
{
	bytes<IV> iv = AES::GenerateIV();
	
	blockData ciphertext = EncryptBlock(key, iv, plaintext);

	// Put randomised IV at the front of the ciphertext
	ciphertext.insert(ciphertext.begin(), iv.begin(), iv.end());

	return ciphertext;
}

blockData AES::Decrypt(bytes<Key> key, blockData ciphertext, size_t blockSize)
{
	// Extract the IV
	bytes<IV> iv;
	std::copy(ciphertext.begin(), ciphertext.begin() + IV, iv.begin());

	ciphertext.erase(ciphertext.begin(), ciphertext.begin() + IV);

	// Perform the decryption
	blockData plaintext = DecryptBlock(key, iv, ciphertext, blockSize);

	return plaintext;
}

int AES::GetCiphertextLength(int plen)
{
	// Round up to the next 16 bytes (due to padding)
	return (plen/16 + 1) * 16;
}

bytes<IV> AES::GenerateIV()
{
	bytes<IV> iv;
	
	if (RAND_bytes(iv.data(), iv.size()) != 1) {
		// Bytes generated aren't cryptographically strong
		Log::Write(Log::FATAL,"Needs more entropy");
	}
	
	return iv;
}
