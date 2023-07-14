pushd modules > /dev/null 
./moduleBuild.sh
popd > /dev/null

echo && echo "########################### client ###########################"
pushd client > /dev/null 
make -B 
popd > /dev/null

echo && echo "########################### server ###########################"
pushd server > /dev/null
make -B
popd > /dev/null
