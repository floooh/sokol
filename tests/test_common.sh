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
    gen_preset=$1
    build_preset=$2
    cmake --preset $gen_preset
    cmake --build --preset $build_preset
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

runtest() {
    cfg=$1
    cd build/$cfg
    ./sokol-test
    cd ../../..
}
