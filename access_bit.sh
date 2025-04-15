#!/bin/bash

VM_NAME="ubuntu20.04-2"
DEST_HOST="manya@172.31.106.57"
DEST_URI="qemu+ssh://$DEST_HOST/system"
TMP_HOT_PAGES="/tmp/hot_pages.bin"
DEST_HOT_PAGES="/tmp/${VM_NAME}-hot-pages.bin"

echo "============= VM Migration with Pre-Generated Hot Pages ============="

# Step 1: Confirm hot_pages.bin exists
if [ ! -f "$TMP_HOT_PAGES" ]; then
    echo "ERROR: Hot pages file not found at $TMP_HOT_PAGES"
    exit 1
fi

# Step 2: Transfer hot pages to destination
echo "Transferring hot pages file to destination host..."
scp "$TMP_HOT_PAGES" "$DEST_HOST:$DEST_HOT_PAGES"

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to copy hot pages file to destination."
    exit 1
fi
echo "Hot pages file successfully copied to $DEST_HOST:$DEST_HOT_PAGES"

# Step 3: Perform post-copy live migration
echo "Initiating live post-copy migration of VM: $VM_NAME"
virsh migrate --live --postcopy --verbose "$VM_NAME" "$DEST_URI"

if [ $? -eq 0 ]; then
    echo "Migration completed successfully."
else
    echo "Migration failed."
    exit 1
fi

