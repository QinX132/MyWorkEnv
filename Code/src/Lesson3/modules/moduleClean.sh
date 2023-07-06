for dir in $(ls .)
do
    if [ -d $dir ]; then
        pushd $dir > /dev/null
        echo "########## $dir ##########"
        make clean
        popd > /dev/null
        echo
    fi
done
