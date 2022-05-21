prepare() {
    if [ ! -d "ext/sokol-tools-bin" ] ; then
        git clone --depth 1 https://github.com/floooh/sokol-tools-bin ext/sokol-tools-bin
    fi
    if [ ! -d "ext/fips-cimgui" ] ; then
        git clone --depth 1 --recursive https://github.com/fips-libs/fips-cimgui ext/fips-cimgui
    fi
}

build() {
    cfg=$1
    backend=$2
    mkdir -p build/$cfg && cd build/$cfg
    cmake -G Ninja -D SOKOL_BACKEND=$backend ../..
    cmake --build .
    cd ../..
}

build_arc() {
    cfg=$1
    backend=$2
    mkdir -p build/$cfg && cd build/$cfg
    cmake -G Ninja -D SOKOL_BACKEND=$backend -D USE_ARC:BOOL=ON ../..
    cmake --build .
    cd ../..
}

build_ios() {
    cfg=$1
    backend=$2
    toolchain=$3
    mkdir -p build/$cfg && cd build/$cfg
    cmake -G Xcode -D SOKOL_BACKEND=$backend -D CMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake ../..
    cmake --build . -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO
    cd ../..
}

build_arc_ios() {
    cfg=$1
    backend=$2
    toolchain=$3
    mkdir -p build/$cfg && cd build/$cfg
    cmake -G Xcode -D SOKOL_BACKEND=$backend -DUSE_ARC:BOOL=ON -D CMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake ../..
    cmake --build . -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO
    cd ../..
}

runtest() {
    cfg=$1
    cd build/$cfg
    ./sokol-test
    cd ../../..
}
