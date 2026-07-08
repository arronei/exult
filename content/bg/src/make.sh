#!/bin/bash

UCC=../../../ucc.exe
if [[ ! -x "$UCC" ]]; then
	UCC=../../../ucc
fi

if [[ -e "usecode.uc" ]] ; then
	echo "Compiling Usecode..."
	if "$UCC" -o usecode usecode.uc; then
		echo "Usecode has been successfuly compiled!"
	else
		echo "There were error(s) compiling usecode!"
	fi
fi
