#!/bin/sh

filesdir=$1
searchstr=$2

# Check if both arguments are provided
if [ -z "$filesdir" ] || [ -z "$searchstr" ]
then
	echo "Error: Please provide filesdir and searchstr arguments"
	exit 1
fi

# Check if filesdir is a valid directory
if [ ! -d "$filesdir" ]
then
	echo "Error: $filesdir is not a directory"
	exit 1
fi

# Count files and matching lines
numfiles=$(find "$filesdir" -type f | wc -l)
numlines=$(grep -r "$searchstr" "$filesdir" | wc -l)

echo "The number of files are $numfiles and the number of matching lines are $numlines"
