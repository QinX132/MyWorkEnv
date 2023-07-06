pushd modules && ./moduleBuild.sh && popd
pushd client && make -B && popd
pushd server && make -B && popd
