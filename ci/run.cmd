set build_dir=%1
shift
echo Preparing directories...
rmdir /s /q %build_dir%
mkdir %build_dir%
pushd %build_dir%
echo Running cmake -GNinja -DENABLE_TESTS=ON %* ..
cmake -GNinja -DENABLE_TESTS=ON -DCMAKE_INSTALL_PREFIX=dist %* .. || exit /b
echo Running ninja...
ninja %JOBS% || exit /b
ninja install || exit /b
echo Running tests...
cd tests && ctest -V || exit /b
popd
