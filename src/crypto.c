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

typedef struct{
  EVP_CIPHER_CTX * ctx;
  u8 * block;
  u32 buffer_size;
  bool encrypt;
}crypter;

crypter * crypto_encrypt_new(int buffer_size, const char * key, const char * iv){
  var ctx = EVP_CIPHER_CTX_new();
  EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (void *) key, (void *)iv);
  crypter c = {.ctx = ctx, .block = alloc0(buffer_size), .buffer_size = buffer_size, .encrypt = true};
  return iron_clone(&c, sizeof(c));
}

crypter * crypto_decrypt_new(int buffer_size, const char * key, const char * iv){
  var ctx = EVP_CIPHER_CTX_new();
  EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (void *) key, (void *)iv);
  crypter c = {.ctx = ctx, .block = alloc0(buffer_size), .buffer_size = buffer_size, .encrypt = false};
  return iron_clone(&c, sizeof(c));
}

u32 crypto_update(crypter * crypt, void * in_data, u32 in_len, void * out_data, u32 out_len){
  i32 l = (i32) out_len;
  EVP_CipherUpdate(crypt->ctx, out_data, &l, in_data, in_len);
  return l;
}

u32 crypto_finalize(crypter * crypt, void *out_data, u32 out_len){
  i32 l = (i32) out_len;
  EVP_CipherFinal_ex(crypt->ctx, out_data, &l);
  return l;
}

void crypto_delete(crypter * crypt){
  EVP_CIPHER_CTX_free(crypt->ctx);
  dealloc(crypt->block);
  dealloc(crypt);
}

void crypter_test(void){
  const char *key = "01234567890123456789012345678901";
    // A 128 bit IV 
  const char  *iv = "0123456789012345";
  char *plaintext =
    (char *)"The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over2e21e2e12e12e12e12e21e12e21e2-i- -1 lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over2e21e2e12e12e12e12e21e12e21e2-i- -1The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps ";

  u8 block[1024 * 24] = {0};
  crypter * c1 = crypto_encrypt_new(1024, key, iv);
  u32 block_offset = 0;
  for(u32 i = 0; i < strlen(plaintext); i += 256){
    u32 w = crypto_update(c1, plaintext + i, MIN(strlen(plaintext) - i, 256), block + block_offset, sizeof(block) - block_offset);
    block_offset += w;
    logd("%i\n", block_offset);
  }
  block_offset += crypto_finalize(c1, block + block_offset, sizeof(block) - block_offset);
  logd("Crypting done.\n size: %i\n", block_offset);
  logd("input size: %i\n", strlen(plaintext));
  for(u32 i = 0; i < block_offset + 2; i++){
    logd("%x ", block[i]);
    if(i%32 == 31)
      logd("\n");
  }
  logd("\n");
  u8 block2[1024 * 24] = {0};
  u32 block_offset2 = 0;
  crypter * c2 = crypto_decrypt_new(1024, key, iv);
  for(u32 i = 0; i < block_offset; i += 256){
    u32 w = crypto_update(c2, block + i, MIN(block_offset - i, 256), block2 + block_offset2, sizeof(block2) - block_offset2);
    block_offset2 += w;
  }
  block_offset2 += crypto_finalize(c2, block2 + block_offset2, sizeof(block) - block_offset2);
  logd("%i (%i): %s\n", strlen(block2), block_offset2, block2);
  logd("==: %i\n", strcmp(block2, plaintext));
  crypto_delete(c2);
  crypto_delete(c1);
}
