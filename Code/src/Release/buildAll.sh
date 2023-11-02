ret=0

pushd myUtils > /dev/null
if [ $# -eq 0 ] || { [ $# -gt 0 ] && [ "$1" != "q" ]; }; then
    echo "####################### Utils test #########################"
    make DEBUG=true -B
    make test
    ret=$?
    if [ "$ret" != "0" ]; then
        exit -1
    fi
    echo
fi
echo "########################## Utils ##########################"
make -B
ret=$?
if [ "$ret" != "0" ]; then
    exit -1
fi
popd > /dev/null

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
