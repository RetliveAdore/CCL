if ls CMakeLists.txt;then
    echo "Starting..."
    if ls build/;then
        rm -rf build/
    fi
    mkdir build
    cmake -S . -B build/
    cd build
    make
else
    echo "CMakeLists.txt not found!!!"
fi