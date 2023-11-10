echo && echo "########################## myUtils ##########################"
pushd myUtils > /dev/null && make clean && popd > /dev/null
echo && echo "########################### client ###########################"
pushd client > /dev/null && make clean && popd > /dev/null
echo && echo "########################### server ###########################"
pushd server > /dev/null && make clean && popd > /dev/null
