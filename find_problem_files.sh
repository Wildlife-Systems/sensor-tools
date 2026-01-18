#!/bin/bash
# Find files where --clean gives higher count than without

echo "Checking files for count discrepancies..."
echo ""

total_without=0
total_with=0
file_count=0

for f in /urban-nature-project/sensors/*.out /urban-nature-project/sensors/*/*.out; do
    if [ -f "$f" ]; then
        file_count=$((file_count + 1))
        without=$(sensor-data count "$f" 2>/dev/null)
        with=$(sensor-data count --clean "$f" 2>/dev/null)
        
        total_without=$((total_without + without))
        total_with=$((total_with + with))
        
        if [ "$with" -gt "$without" ]; then
            diff=$((with - without))
            echo "PROBLEM (higher with --clean): $f"
            echo "  Without --clean: $without"
            echo "  With --clean:    $with"
            echo "  Difference:      +$diff"
            echo ""
        elif [ "$with" -ne "$without" ]; then
            diff=$((without - with))
            echo "OK (lower with --clean): $(basename $f): $without -> $with (-$diff)"
        fi
    fi
done

echo ""
echo "Files processed: $file_count"
echo "Total without --clean: $total_without"
echo "Total with --clean:    $total_with"
echo "Difference: $((total_with - total_without))"
echo "Done."
