#/bin/bash

# set -x

FREERTOS_DL_URL="https://github.com/FreeRTOS/FreeRTOS/releases/download/V10.4.1/FreeRTOSv10.4.1.zip"


ZIP_FILENAME=`basename $FREERTOS_DL_URL`
TMP_DIR="tmp"
TARGET_DIR="freertos"

function download_if_exists {
    URL="$1"
    FILENAME="$2"
    if [ -e "$FILENAME" ]; then
        echo "File '$FILENAME' already exists, not downloading."
    else
        echo "Downloading '$URL' to '$FILENAME'."
        curl -o "$FILENAME" -L "$URL"
    fi
}

function unpack {
    ZIPFILE="$1"
    TARGET="$2"

    FREERTOS_FOLDER_NAME=`basename "$ZIPFILE" .zip`

    FILE_LIST="$FREERTOS_FOLDER_NAME/FreeRTOS/License/* $FREERTOS_FOLDER_NAME/FreeRTOS/Source/* $FREERTOS_FOLDER_NAME/FreeRTOS/Source/include/* $FREERTOS_FOLDER_NAME/FreeRTOS/Source/portable/GCC/ARM_CM3/* $FREERTOS_FOLDER_NAME/FreeRTOS/Source/portable/MemMang/heap_4.c"
 
    unzip -j "$ZIPFILE" $FILE_LIST -d "$TARGET"
}

# Create the work directory
mkdir -p "$TMP_DIR"

# Get and unzip FreeRTOS
download_if_exists "$FREERTOS_DL_URL" "$TMP_DIR/$ZIP_FILENAME"
mkdir -p "$TARGET_DIR"
unpack "$TMP_DIR/$ZIP_FILENAME" "$TARGET_DIR"