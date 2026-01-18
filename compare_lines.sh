#!/bin/bash
# Compare count per line with and without --clean

FILE="${1:-/urban-nature-project/sensors/unp-pi-09.nhm.ac.uk_sensors.out}"
LINENUM=0

while IFS= read -r line; do
    LINENUM=$((LINENUM + 1))
    
    # Skip empty lines
    if [ -z "$line" ]; then
        continue
    fi
    
    without=$(echo "$line" | sensor-data count 2>/dev/null)
    with=$(echo "$line" | sensor-data count --clean 2>/dev/null)
    
    if [ "$with" != "$without" ]; then
        echo "Line $LINENUM: without=$without, with --clean=$with"
        echo "  Content: ${line:0:100}..."
        echo ""
        
        # Stop after 10 differences
        if [ $LINENUM -gt 1000 ] && [ "$with" -gt "$without" ]; then
            echo "Found line where --clean gives MORE. Stopping."
            echo "Full line:"
            echo "$line" | cat -v
            break
        fi
    fi
done < "$FILE"

echo "Done processing $LINENUM lines"
