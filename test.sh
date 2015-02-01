#!/bin/bash

CRED="\033[1;31m"
CGREEN="\033[1;32m"
CNORMAL="\033[0m"

ERROR=0

function fail(){
	echo -en "${CRED}FAIL: $1${CNORMAL} "
	shift 1
	echo $*
	ERROR=$(( $ERROR+1 ))
}

function success(){
	echo -en "${CGREEN}$1${CNORMAL} "
	shift 1
	echo $*
}

function assert_eq(){
	local r1=`$1`
	if [ "$r1" != "$2" ]; then
		fail "Assert" $1, $r1 == $2
	else
		success "Assert" $1 == $2
	fi
}
function assert_eq_stderr(){
	local r1=`$1 2>&1`
	if [ "$r1" != "$2" ]; then
		fail "Assert" $1, $r1 == $2
	else
		success "Assert" $1 == $2
	fi
}

function assert_ok(){
	$* >/dev/null
	if [ "$?" != '0' ]; then
		fail "Assert run ok" $1
	else
		success "Assert run ok" $1
	fi
}
function assert_nok(){
	$* >/dev/null
	if [ "$?" == '0' ]; then
		fail "Assert run nok" $1
	else
		success "Assert run nok" $1 
	fi
}

assert_ok "./commands"
assert_ok "./commands --list"
assert_nok "./commands --help"
assert_ok "./commands help"
assert_eq "./commands --which=example" "`./commands --which example`"
assert_eq "./commands --which=example --which=example" "`./commands --which example --which=example`"
assert_ok "./commands example"
assert_nok "./commands unknown"
assert_eq "./commands example" "`./commands-example`"
assert_eq "./commands example testing" "`./commands-example testing`"
assert_eq "./commands example --one-line-help" "Just an example"
assert_eq "./commands --version --which=help" "1.0"
assert_eq_stderr "./commands --which=help" "[This is an internal command]"


if [ "$ERROR" != "0" ]; then
	fail $CRED"$ERROR error/s"
	exit 1
fi