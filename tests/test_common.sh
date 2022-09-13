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
        unzip -q $sdk_file
        cd tools/bin
        yes | ./sdkmanager "platforms;android-28" >/dev/null
        yes | ./sdkmanager "build-tools;29.0.3" >/dev/null
        yes | ./sdkmanager "platform-tools" >/dev/null
        yes | ./sdkmanager "ndk-bundle" >/dev/null
        cd ../../../..
    fi
}

build() {
    cfg=$1
    backend=$2
    mode=$3
    mkdir -p build/$cfg && cd build/$cfg
    cmake -GNinja -DSOKOL_BACKEND=$backend -DCMAKE_BUILD_TYPE=$mode ../..
    cmake --build .
    cd ../..
}

analyze() {
    cfg=$1
    backend=$2
    mode=$3
    mkdir -p build/$cfg && cd build/$cfg
    cmake -GNinja -DSOKOL_BACKEND=$backend -DCMAKE_BUILD_TYPE=$mode -DUSE_ANALYZER=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ../..
    cmake --build .
    cd ../..
}

build_arc() {
    cfg=$1
    backend=$2
    mode=$3
    mkdir -p build/$cfg && cd build/$cfg
    cmake -GNinja -DSOKOL_BACKEND=$backend -DUSE_ARC:BOOL=ON -DCMAKE_BUILD_TYPE=$mode ../..
    cmake --build .
    cd ../..
}

analyze_arc() {
    cfg=$1
    backend=$2
    mode=$3
    mkdir -p build/$cfg && cd build/$cfg
    cmake -GNinja -DSOKOL_BACKEND=$backend -DUSE_ARC:BOOL=ON -DCMAKE_BUILD_TYPE=$mode -DUSE_ANALYZER=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ../..
    cmake --build .
    cd ../..
}

build_ios() {
    cfg=$1
    backend=$2
    mode=$3
    mkdir -p build/$cfg && cd build/$cfg
    cmake -GXcode -DSOKOL_BACKEND=$backend -DCMAKE_SYSTEM_NAME=iOS ../..
    cmake --build . --config $mode -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO
    cd ../..
}

analyze_ios() {
    cfg=$1
    backend=$2
    mode=$3
    mkdir -p build/$cfg && cd build/$cfg
    cmake -GNinja -DSOKOL_BACKEND=$backend -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_BUILD_TYPE=$mode -DUSE_ANALYZER=ON ../..
    cmake --build .
    cd ../..
}

build_arc_ios() {
    cfg=$1
    backend=$2
    mode=$3
    mkdir -p build/$cfg && cd build/$cfg
    cmake -GXcode -DSOKOL_BACKEND=$backend -DUSE_ARC:BOOL=ON -DCMAKE_SYSTEM_NAME=iOS ../..
    cmake --build . --config $mode -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO
    cd ../..
}

analyze_arc_ios() {
    cfg=$1
    backend=$2
    mode=$3
    mkdir -p build/$cfg && cd build/$cfg
    cmake -GNinja -DSOKOL_BACKEND=$backend -DUSE_ARC:BOOL=ON -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_BUILD_TYPE=$mode -DUSE_ANALYZER=ON ../..
    cmake --build .
    cd ../..
}

build_emsc() {
    cfg=$1
    backend=$2
    mode=$3
    mkdir -p build/$cfg && cd build/$cfg
    emcmake cmake -GNinja -DSOKOL_BACKEND=$backend -DCMAKE_BUILD_TYPE=$mode ../..
    cmake --build .
    cd ../..
}

build_android() {
    cfg=$1
    backend=$2
    mode=$3
    mkdir -p build/$cfg && cd build/$cfg
    cmake -GNinja -DSOKOL_BACKEND=$backend -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-28 -DCMAKE_TOOLCHAIN_FILE=../android_sdk/ndk-bundle/build/cmake/android.toolchain.cmake -DCMAKE_BUILD_TYPE=$mode ../..
    cmake --build .
    cd ../..
}

runtest() {
    cfg=$1
    cd build/$cfg
    ./sokol-test
    cd ../../..
}
