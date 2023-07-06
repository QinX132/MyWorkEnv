for dir in $(ls .)
do
    if [ -d $dir ]; then
        pushd $dir > /dev/null
        echo "########## $dir ##########"
        make -B
        popd > /dev/null
        echo
    fi
done
