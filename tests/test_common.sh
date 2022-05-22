prepare() {
    if [ ! -d "ext/fips-cimgui" ] ; then
        git clone --depth 1 --recursive https://github.com/fips-libs/fips-cimgui ext/fips-cimgui
    fi
}

setup_emsdk() {
    if [ ! -d "build/emsdk" ] ; then
        mkdir -p build && cd build
        git clone https://github.com/emscripten-core/emsdk.git
        cd emsdk
        ./emsdk install latest
        ./emsdk activate latest
        cd ../..
    fi
    source build/emsdk/emsdk_env.sh
}

setup_android() {
    if [ ! -d "build/android_sdk" ] ; then
        mkdir -p build/android_sdk && cd build/android_sdk
        sdk_file="sdk-tools-linux-3859397.zip"
        wget --no-verbose https://dl.google.com/android/repository/$sdk_file
        unzip $sdk_file
        cd tools/bin
        yes | ./sdkmanager "platforms;android-28"
        yes | ./sdkmanager "build-tools;29.0.3"
        yes | ./sdkmanager "platform-tools"
        yes | ./sdkmanager "ndk-bundle"
        cd ../../../..
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
    mkdir -p build/$cfg && cd build/$cfg
    cmake -G Xcode -D SOKOL_BACKEND=$backend -D CMAKE_SYSTEM_NAME=iOS ../..
    cmake --build . -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO
    cd ../..
}

build_arc_ios() {
    cfg=$1
    backend=$2
    mkdir -p build/$cfg && cd build/$cfg
    cmake -G Xcode -D SOKOL_BACKEND=$backend -DUSE_ARC:BOOL=ON -D CMAKE_SYSTEM_NAME=iOS ../..
    cmake --build . -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO
    cd ../..
}

build_emsc() {
    cfg=$1
    backend=$2
    mkdir -p build/$cfg && cd build/$cfg
    emcmake cmake -G Ninja -D SOKOL_BACKEND=$backend ../..
    cmake --build .
    cd ../..
}

build_android() {
    cfg=$1
    backend=$2
    mkdir -p build/$cfg && cd build/$cfg
    cmake -G Ninja -D SOKOL_BACKEND=$backend -D ANDROID_ABI=armeabi-v7a -D ANDROID_PLATFORM=android-28 -D CMAKE_TOOLCHAIN_FILE=../android_sdk/ndk-bundle/build/cmake/android.toolchain.cmake ../..
    cmake --build .
    cd ../..
}

runtest() {
    cfg=$1
    cd build/$cfg
    ./sokol-test
    cd ../../..
}
