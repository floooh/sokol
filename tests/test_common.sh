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

runtest() {
    cfg=$1
    cd build/$cfg/functional
    echo $(pwd)
    ./sokol-test
    cd ../../..
}
