export CROSS_TOOLS=arm-linux-androideabi-
export G_MACHINE=armv7 #i686
export G_SYSTEM=android #linux
export G_ARCH=arm #x86
export ROOT_DIR=${PWD}
export INSTALL_DIR=${ROOT_DIR}/out

mkdir -p ${INSTALL_DIR}

cd ${ROOT_DIR}/src
./build.sh

