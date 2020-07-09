#!/bin/sh

# Description:
#        Test harness. It is written in sh, since csh does not
# have subroutine facility.
#
# Usage:
# ../scripts/runTests.sh "test1 test2" - to run test1 and test2
# ../scripts/runTests.sh               - to run all tests
# 
usage() {
	echo `basename $0`: ERROR: $* 1>&2
	echo usage: `basename $0` 
	exit 1
}

# include/define other reqired vars
scriptPath=${0%/*}
#. $scriptPath/vars.sh
#. $scriptPath/global_vars.sh

# variables
RUN_STATUS=1

REMOVE_RECURSIVE () {
	FILES=`find . -name $1\* -print`
	if [ "x$FILES" != "x" ]; then
		/bin/rm -fr $FILES
	fi
}

REMOVE_FILES () {
    if [ "x$1" != "x" ]; then
		/bin/rm -fr $1
	fi
}

CLEAN() {
	REMOVE_FILES "*.tmp"
	REMOVE_FILES "*.diff"
	REMOVE_RECURSIVE jobs
	REMOVE_RECURSIVE *.log
}

COMPARE () {
	DIFF=/usr/bin/diff
	$DIFF ${1}.ref ${1}.out > ${1}.diff 2>&1 
}

RUN_CUTTER() {
	../bin/cutter.exe $1 ${1}.out
	RUN_STATUS=$?
}

RUN_CLIENT() {
	. `pwd`/${1}
	RUN_STATUS=$?
}

REPORT() {
	test=`echo "$1                                " | cut -b 1-31`

	status=FAIL
	if [ "x$RUN_STATUS" != "x0" ] ; then
		status="FAIL (run)  $2"
	elif [ "$3" != "0" ]; then
		status="FAIL (diff) $2"
	else
		status="pass        $2"
		REMOVE_FILES "$1.out"
	fi ;


	echo "$test          $status"; 
}

RUN_TESTS() {
	echo $1; echo "============="
	for test in $2
	do
		if [ ! -f $test ]; then
			continue ;
		fi;

		CLEAN 
		start=`date +%s`
		if [ "x$1" == "xCUTTER_TESTS" ] ; then
			RUN_CUTTER $test
		elif [ "x$1" == "xCLIENT_TESTS" ]; then
			RUN_CLIENT $test
		else
			echo noop
		fi
		runtime=$((`date +%s`-$start))sec
		COMPARE $test
		REPORT $test $runtime $?

	done
}

CUTTER_TESTS="cutter.1 \
		cutter.2 \
		cutter.3 \
		cutter.4 \
		cutter.5 \
		cutter.6 \
		cutter.7"
RUN_TESTS CUTTER_TESTS "$CUTTER_TESTS"

CLIENT_TESTS="client.1 \
		client.2 \
		client.3 \
		client.4 \
		client.5"
RUN_TESTS CLIENT_TESTS "$CLIENT_TESTS"
