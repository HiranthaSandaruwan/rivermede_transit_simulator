#!/bin/sh
# Smoke test for Rivermede Transit.
#
# Drives the menu with scripted input and checks the lines that never change.
# The microsecond timings depend on the machine, so they are not checked.
#
# Usage: sh tests/smoke.sh [path-to-program]     (default: ./transit)

bin=${1:-./transit}
failed=0

# Feeds the answers to the program and squeezes repeated spaces, so the
# checks do not depend on the column padding.
run() {
    printf '%b' "$1" | "$bin" | tr -s ' '
}

# expect <test name> <program output> <text that must appear in it>
expect() {
    if printf '%s\n' "$2" | grep -qF -- "$3"; then
        echo "PASS  $1"
    else
        echo "FAIL  $1 (missing: $3)"
        failed=1
    fi
}

# With no input the program must stop cleanly. This also catches a missing build.
if ! "$bin" < /dev/null > /dev/null 2>&1; then
    echo "Cannot run $bin (build it first with: make)"
    exit 2
fi

# Menu 1 and 2: the route lists.
out=$(run '1\n\n2\n\n0\n')
expect "bus routes are listed"   "$out" "Central Station to University 10 min"
expect "train routes are listed" "$out" "East Bridge to Harbor 9 min"

# Menu 3: Airport (stop 2) to Old Town (stop 8) at 07:00. Train, then bus.
out=$(run '3\n2\n8\n1\n\n0\n')
expect "fastest way is 22 minutes"  "$out" "Fastest way: 22 min, 2 rides"
expect "first ride is the train"    "$out" "1. TRAIN Airport to Central Station 15 min"
expect "second ride is the bus"     "$out" "2. BUS Central Station to Old Town 7 min"
expect "step counts of both searches" "$out" "Work done: DFS tried 28 journeys in 484 steps. BFS took 48 steps."

# Menu 3: Greenwood (10) to Tech Park (3). DFS and BFS give different journeys.
out=$(run '3\n10\n3\n1\n\n0\n')
expect "DFS finds the fastest way"        "$out" "Fastest way: 24 min, 3 rides"
expect "BFS finds the fewest rides way"   "$out" "Way with fewest rides: 25 min, 2 rides"

# Menu 4: the demo day.
out=$(run '4\n\n0\n')
expect "24 passengers travel"       "$out" "DEMO DAY: 24 passengers"
expect "step totals for the day"    "$out" "All day 24 1152 11166"
expect "BFS always takes 48 steps"  "$out" "BFS always takes 12 stops + 36 route checks = 48 steps."
expect "journeys DFS tried"         "$out" "DFS tried 26.8 journeys for each passenger"
expect "BFS is slower for 4 passengers" "$out" "Fewest rides ways average 16.5 min, slower for 4 of 24 passengers."

# Menu 5: two passengers typed in by the user.
out=$(run '5\n2\n8\n1\n1\n9\n3\n3\n\n0\n')
expect "own passengers are simulated" "$out" "MY PASSENGERS: 2 passengers"
expect "step totals for own passengers" "$out" "All day 2 96 560"

# Wrong input is rejected and the program still exits cleanly.
out=$(run '9\nabc\n0\n')
expect "wrong menu input is rejected" "$out" "Please type a number from 0 to 5"
expect "program says goodbye"         "$out" "Goodbye!"

if [ "$failed" -ne 0 ]; then
    echo "Some checks failed."
    exit 1
fi
echo "All checks passed."
