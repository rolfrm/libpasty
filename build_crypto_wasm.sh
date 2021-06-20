export CC=emcc
export CXX=emcc
export LINK=${CXX}
export ARCH_FLAGS=""
export ARCH_LINK=""
export CPPFLAGS=" ${ARCH_FLAGS} "
export CXXFLAGS=" ${ARCH_FLAGS} "
export CFLAGS=" ${ARCH_FLAGS} -O3 -g0"
export LDFLAGS=" ${ARCH_LINK} -O3 -g0"
echo  $OSTYPE | grep -i darwin > /dev/null 2> /dev/null
rm -r archive
mkdir archive
cd submodule_openssl
git clean -dfx
./Configure purify --openssldir=/tmp --api=1.1.0 no-engine no-dso no-dgram no-sock no-srtp no-stdio no-ui no-err no-ocsp no-psk no-stdio no-ts
make -j 8
cp libcrypto.a ../archive
cd ../archive
ar x libcrypto.a 
rm ./liblegacy*
emcc *.o -r -s WASM=1 -s USE_GLFW=3 -o libcrypto.bc
cp libcrypto.bc ../
 
