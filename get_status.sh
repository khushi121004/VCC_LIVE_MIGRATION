#!/bin/bash
OUTPUT_FILE="memory_intensive_post_opt.txt"
while true; do
    # Redirect both stdout and stderr to the output file
    output=$(virsh domjobinfo ubuntu20.04-2 2>&1)
    
    # If the command fails or returns empty output
    if [ -z "$output" ]; then
        exit 1
    else 
        echo "$output" >> "$OUTPUT_FILE"
    fi
done

