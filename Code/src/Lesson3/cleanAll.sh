pushd modules && ./moduleClean.sh && popd
pushd client && make clean && popd
pushd server && make clean && popd
