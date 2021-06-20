#include <iron/full.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
//#include <openssl/evp.h>

void crypto_main(void){
  logd("Test crypto!\n");
  var bne = BN_new();
  unsigned long e = RSA_F4;
  BN_set_word(bne, e);
  RSA *r = RSA_new();
  int bits = 2048;
  RSA_generate_key_ex(r, bits, bne, NULL);
  BIO *pbkeybio =  BIO_new(BIO_s_mem());
  BIO *pkeybio = BIO_new(BIO_s_mem());
  
  //PEM_write_bio_RSAPublicKey(bp_public, r);
  //PEM_write_bio_RSAPrivateKey(bp_private, r, NULL, NULL, 0, NULL, NULL);
  const char * message = "hej hej";
  char * encrypt = (char *)malloc(RSA_size(r));
  int encrypt_len;
  var err = (char *)malloc(130);
  if((encrypt_len = RSA_public_encrypt(strlen(message)+1, (unsigned char*)message, (unsigned char*)encrypt,
				       r, RSA_PKCS1_OAEP_PADDING)) == -1) {
    ERR_load_crypto_strings();
    ERR_error_string(ERR_get_error(), err);
    fprintf(stderr, "Error encrypting message: %s\n", err);    
  }
  logd("Encrypt length: %i\n", encrypt_len);
  char * decrypt = (char *)alloc0(RSA_size(r));
  int decrypt_len = RSA_private_decrypt(encrypt_len, encrypt, decrypt,r,  RSA_PKCS1_OAEP_PADDING);
  logd("Decrypt length %i\n", decrypt_len);
  logd("Decrypt %s\n", decrypt);  
}


int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
  int len;
  int ciphertext_len;
  var ctx = EVP_CIPHER_CTX_new();
  EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
  EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
  ciphertext_len = len;
  EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
  ciphertext_len += len;
  EVP_CIPHER_CTX_free(ctx);
  return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv,  char ** plaintext)
{
    int len;
    int plaintext_len = 0;
    void * outbuf = NULL;
    var ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    u8 block[256] = {0};
    bool finished = false;
    while(!finished){
      len = sizeof(block);
      int read = MIN(sizeof(block), ciphertext_len);
      EVP_DecryptUpdate(ctx, block, &len, ciphertext, read );
      ciphertext += read;
      ciphertext_len -= read;
      if(len == 0){
	EVP_DecryptFinal_ex(ctx, block, &len);
	finished = true;
      }
      outbuf = realloc(outbuf, plaintext_len + len);
      memcpy(outbuf + plaintext_len, block, len);
      plaintext_len += len;
    }
    *plaintext = outbuf;
    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}

int aes_main (void)
{
  //A 256 bit key 
    unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
    // A 128 bit IV 
    unsigned char *iv = (unsigned char *)"0123456789012345";
    unsigned char *plaintext =
      (unsigned char *)"The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over2e21e2e12e12e12e12e21e12e21e2-i- -1 lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over2e21e2e12e12e12e12e21e12e21e2-i- -1 ";
    
    unsigned char ciphertext[1028];

    int decryptedtext_len, ciphertext_len;
    ciphertext_len = encrypt (plaintext, strlen ((char *)plaintext) + 1, key, iv,
                              ciphertext);

    char * decbuf = NULL;
    
    decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, iv, &decbuf);
    printf("Decrypted text is:\n");
    printf("%s\n", decbuf);
    printf("Length was: %i\n", decryptedtext_len);

    return 0;
}
