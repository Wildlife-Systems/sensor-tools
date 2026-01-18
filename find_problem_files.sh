#!/bin/bash
# Find files where --clean gives higher count than without

echo "Checking files for count discrepancies..."
echo ""

for f in /urban-nature-project/sensors/*.out /urban-nature-project/sensors/*/*.out; do
    if [ -f "$f" ]; then
        without=$(sensor-data count "$f" 2>/dev/null)
        with=$(sensor-data count --clean "$f" 2>/dev/null)
        
        if [ "$with" -gt "$without" ]; then
            diff=$((with - without))
            echo "PROBLEM: $f"
            echo "  Without --clean: $without"
            echo "  With --clean:    $with"
            echo "  Difference:      +$diff"
            echo ""
        fi
    fi
done

echo "Done."
