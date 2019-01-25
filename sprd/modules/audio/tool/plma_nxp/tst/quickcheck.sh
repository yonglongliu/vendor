#!/bin/bash
#
# . quickcheck.sh
# cdiff or gdiff for showing diff
# rdiff for reverting diff

# aliasses
alias quickcheck="check > quickcheck.sh.log.txt"
alias cdiff="svn diff quickcheck.sh.log.txt | colordiff"
alias gdiff="svn diff --diff-cmd=meld quickcheck.sh.log.txt"
alias revert="svn revert quickcheck.sh.log.txt"

function check {
	export LD_LIBRARY_PATH=..
	$LD_LIBRARY_PATH/climax -ddummy -l N1A12.cnt -t   --start
	$LD_LIBRARY_PATH/climax -ddummy -l N1A12.cnt -t   --profile=1 --start --currentprof=0
	$LD_LIBRARY_PATH/climax -ddummy -l N1A12.cnt -t   --profile=2 --start --currentprof=1
	$LD_LIBRARY_PATH/climax -ddummy -l N1A12.cnt -t   --profile=3 --start --currentprof=2
	$LD_LIBRARY_PATH/climax -ddummy -l N1A12.cnt -t   --profile=0 --start --currentprof=3
	$LD_LIBRARY_PATH/climax -ddummy -l N1A12.cnt -t   --diag i2c
	$LD_LIBRARY_PATH/climax -ddummy -l N1A12.cnt -t   --diag dsp
}

check > quickcheck.sh.log.txt

echo "to check the diff compared to svn, type: cdiff or gdiff"
echo "to revert changes, type: revert"
echo "to quick check again, type: quickcheck"

