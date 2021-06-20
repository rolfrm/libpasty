export CC=gcc
export CXX=gcc
export LINK=${CXX}
export ARCH_FLAGS=""
export ARCH_LINK=""
export CPPFLAGS=" ${ARCH_FLAGS} "
export CXXFLAGS=" ${ARCH_FLAGS} "
export CFLAGS=" ${ARCH_FLAGS} -O4 -g0"
export LDFLAGS=" ${ARCH_LINK} -O4 -g0"
echo  $OSTYPE | grep -i darwin > /dev/null 2> /dev/null
cd submodule_openssl
git clean -dfx
./Configure  --openssldir=/tmp --api=1.1.0   no-dgram no-sock no-srtp no-stdio no-dso  no-ocsp enable-ec_nistp_64_gcc_128 no-shared 
make -j 8
cp libcrypto.a .. 
