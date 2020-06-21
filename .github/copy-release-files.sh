#!/bin/bash

OUTPUT_GLOB="c6021light/.pio/build/**/firmware.*"

DESTINATION_DIR=".github/releasefiles"

# Create destination directory
mkdir -p "$DESTINATION_DIR"

# Copy all files to destination directory
for f in $OUTPUT_GLOB; do
    echo "Source: $f"
    dironly=`dirname $f`
    target=`basename $dironly`
    filename=`basename $f`
    destination="$DESTINATION_DIR/$target-$filename"
    echo "Destionation: $destination"
    cp "$f" "$destination"
done