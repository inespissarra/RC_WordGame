#!/usr/bin/env bash

MY_IP=$(curl -s http://ipecho.net/plain)
TEJO="nc tejo.tecnico.ulisboa.pt 59000"
TOTAL_TESTS=$(ls tests/scripts/*.txt | wc -l)
CORRECT_TESTS=0

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

function run_server() {
  ./GS/server word_eng.txt -v -p 58082 > tests/tmp/server.log
}

function run_command() {
    echo "Testing $1"
    FILE_NAME=$(basename $1 .txt)
    TEST=$(echo $FILE_NAME | cut -d'_' -f2)
    COMMAND=$(echo "$MY_IP 58082 $TEST" | $TEJO > tests/tmp/report-$TEST.html)
    if [ ! -s tests/tmp/report-$TEST.html ] || grep -q "color=\"red\"" tests/tmp/report-$TEST.html; then
        echo -e "${RED}Test failed${NC}"
    else
        echo -e "${GREEN}Test passed${NC}"
        CORRECT_TESTS=$((CORRECT_TESTS+1))
    fi
}

function handle_test() {
    run_server &
    run_command $1
    make clean
    make
    killall GS/server
    sleep 2 # to avoid being timed out ;-;
}

# Script core

echo "You can find the results of each test in the tests/tmp folder"

# create the tests/tmp folder if it doesn't exist
mkdir -p tests/tmp
# clear it
rm -rf tests/tmp/*

for test in tests/scripts/*.txt
do
    handle_test $test
done

echo -e "${BLUE}Total tests: $TOTAL_TESTS${NC}"
echo -e "${GREEN}Correct tests: $CORRECT_TESTS${NC}"

if [ $CORRECT_TESTS -eq $TOTAL_TESTS ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit -1
fi