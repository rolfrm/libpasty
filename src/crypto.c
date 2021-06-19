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

void crypto_main_evp(void){
  logd("Test crypto EVP!\n");

  EVP_PKEY *pkey = NULL;
  EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
  EVP_PKEY_keygen_init(pctx);
  EVP_PKEY_keygen(pctx, &pkey);
  
  const char * message = "hej hej";
  char * encrypt = (char *)malloc(100);
  size_t encrypt_len = 100;
  var err = (char *)malloc(130);

  
  
  if((encrypt_len = EVP_PKEY_encrypt(pctx,message, strlen(message)+1,  (unsigned char*)encrypt, &encrypt_len )) == -1) {
    ERR_load_crypto_strings();
    ERR_error_string(ERR_get_error(), err);
    fprintf(stderr, "Error encrypting message: %s\n", err);    
  }
  logd("Encrypt length: %i\n", encrypt_len);
  size_t decrypt_len = 100;
  char * decrypt = (char *)alloc0(decrypt_len);

  EVP_PKEY_decrypt(pctx, decrypt, &decrypt_len, decrypt, encrypt, encrypt_len);
  logd("Decrypt length %i\n", decrypt_len);
  logd("Decrypt %s\n", decrypt);
  
}

