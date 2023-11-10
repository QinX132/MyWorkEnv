ret=0

echo && echo "########################## Utils ##########################"
pushd myUtils > /dev/null 
make -B
ret=$?
popd > /dev/null
if [ "$ret" != "0" ]; then
    exit -1
fi

echo && echo "########################### client ###########################"
pushd client > /dev/null 
make -B 
ret=$?
popd > /dev/null
if [ "$ret" != "0" ]; then
    exit -1
fi

echo && echo "########################### server ###########################"
pushd server > /dev/null
make -B
ret=$?
popd > /dev/null
if [ "$ret" != "0" ]; then
    exit -1
fi
