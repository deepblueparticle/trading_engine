#!/bin/bash
LD_LIBRARY_PATH="../lib:$LD_LIBRARY_PATH"

if [[ $1 == "d" ]]
then
    gdb --args ./ctp_md_ord ../conf/ctp_md.ini
else
    ./ctp_md_ord ../conf/ctp_md.ini
fi
