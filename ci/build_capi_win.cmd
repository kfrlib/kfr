echo Building for x86_64...
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
set build_dir=build-capi-windows-x64
shift
echo Preparing directories...
rmdir /s /q %build_dir%
mkdir %build_dir%
pushd %build_dir%
set arguments=-DENABLE_DFT=ON -DENABLE_CAPI_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=C:/LLVM9/bin/clang-cl.exe
echo Running cmake -GNinja %arguments%  ..
cmake -GNinja %arguments% .. || exit /b
echo Running ninja...
ninja kfr_capi || exit /b
popd

echo Building for x86...
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
set build_dir=build-capi-windows-x32
shift
echo Preparing directories...
rmdir /s /q %build_dir%
mkdir %build_dir%
pushd %build_dir%
set arguments=-DENABLE_DFT=ON -DENABLE_CAPI_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_CXX_COMPILER=C:/LLVM9/bin/clang-cl.exe
echo Running cmake -GNinja %arguments%  ..
cmake -GNinja %arguments% .. || exit /b
echo Running ninja...
ninja kfr_capi || exit /b
popd
