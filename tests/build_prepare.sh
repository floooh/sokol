if [ ! -d "ext/sokol-tools/bin" ] ; then
    git clone --depth 1 https://github.com/floooh/sokol-tools-bin ext/sokol-tools-bin
fi
if [ ! -d "ext/fips-cimgui" ] ; then
    git clone --depth 1 --recursive https://github.com/fips-libs/fips-cimgui ext/fips-cimgui
fi
