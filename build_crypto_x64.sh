export CC=gcc
export CXX=gcc
export LINK=${CXX}
export ARCH_FLAGS=""
export ARCH_LINK=""
export CPPFLAGS=" ${ARCH_FLAGS} "
export CXXFLAGS=" ${ARCH_FLAGS} "
export CFLAGS=" ${ARCH_FLAGS} "
export LDFLAGS=" ${ARCH_LINK} "
echo  $OSTYPE | grep -i darwin > /dev/null 2> /dev/null
cd openssl
./Configure purify --openssldir=/tmp --api=1.1.0 no-engine no-dso no-dgram no-sock no-srtp no-stdio no-ui no-ocsp no-psk 
make 
