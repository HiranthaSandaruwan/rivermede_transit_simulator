// Rivermede Transit: a smart city where people travel only by bus and train.
//
// The city is a graph. The stops are the vertices and the bus and train routes are the edges.
//
// Part 1: the city map
// Part 2: finding journeys (DFS finds the fastest way, BFS finds the way with the fewest rides)
// Part 3: simulating passengers at different times of the day, and profiling the two algorithms
// Part 4: the menu
//
// Build: gcc -std=c11 -Wall -Wextra -o transit src/transit.c
// Run:   ./transit

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_STOPS 20
#define MAX_PASSENGERS 30
#define BANDS 4
#define TIMING_RUNS 100000    // how many searches we repeat to measure the time of one search

#define BUS 0
#define TRAIN 1

#define WHITE 0
#define GRAY 1
#define BLACK 2

char *modeNames[2] = {"BUS", "TRAIN"};
char *bandNames[BANDS] = {"07:00 Morning Peak", "13:00 Midday", "18:00 Evening Peak", "22:00 Night"};

// Gives "s" for every number except 1, so we can print "1 ride" and "2 rides".
char *plural(int n) {
    if (n == 1) return "";
    return "s";
}


// PART 1: THE CITY MAP

// A route from one stop to another.
struct Route {
    int to;
    int minutes;
    int mode;
};

// A stop and the list of routes that leave it. This is an adjacency list.
struct Stop {
    char name[20];
    struct Route routes[10];
    int routeCount;
};

struct Stop stops[MAX_STOPS];
int stopCount = 0;

// Gives the number of a stop. If the name is new, the stop is added.
int stopNumber(char *name) {
    for (int i = 0; i < stopCount; i++) {
        if (strcmp(stops[i].name, name) == 0) return i;
    }
    strcpy(stops[stopCount].name, name);
    stopCount++;
    return stopCount - 1;
}

// Adds a route. Buses and trains go both ways, so we add it at both stops.
void addRoute(char *a, char *b, int minutes, int mode) {
    int x = stopNumber(a);
    int y = stopNumber(b);
    struct Route there = {y, minutes, mode};
    struct Route back = {x, minutes, mode};
    stops[x].routes[stops[x].routeCount] = there;
    stops[x].routeCount++;
    stops[y].routes[stops[y].routeCount] = back;
    stops[y].routeCount++;
}

// Our made-up city: 12 stops, 6 train routes and 12 bus routes.
void buildCity() {
    addRoute("Central Station", "Airport", 15, TRAIN);
    addRoute("Central Station", "Tech Park", 8, TRAIN);
    addRoute("Tech Park", "North Market", 6, TRAIN);
    addRoute("Tech Park", "East Bridge", 10, TRAIN);
    addRoute("Central Station", "Harbor", 12, TRAIN);
    addRoute("East Bridge", "Harbor", 9, TRAIN);

    addRoute("Central Station", "University", 10, BUS);
    addRoute("Central Station", "Old Town", 7, BUS);
    addRoute("University", "Riverside Mall", 6, BUS);
    addRoute("Old Town", "Riverside Mall", 5, BUS);
    addRoute("Riverside Mall", "Harbor", 8, BUS);
    addRoute("Old Town", "Greenwood", 9, BUS);
    addRoute("Greenwood", "Hilltop", 11, BUS);
    addRoute("Hilltop", "Tech Park", 14, BUS);
    addRoute("Sunset Heights", "University", 13, BUS);
    addRoute("Sunset Heights", "Greenwood", 10, BUS);
    addRoute("Harbor", "North Market", 16, BUS);
    addRoute("Tech Park", "Riverside Mall", 12, BUS);
}

// Prints all bus routes or all train routes.
// Each route is stored at both of its stops, so we print it only once: from the stop with the smaller number.
void printRoutes(int mode) {
    printf("\n%s ROUTES (they all go both ways)\n\n", modeNames[mode]);
    for (int i = 0; i < stopCount; i++) {
        for (int j = 0; j < stops[i].routeCount; j++) {
            struct Route r = stops[i].routes[j];
            if (r.mode == mode && r.to > i) {
                printf("  %-16s to  %-16s %2d min\n", stops[i].name, stops[r.to].name, r.minutes);
            }
        }
    }
}

// Prints the stops with their numbers, 3 on each line.
void printStops() {
    for (int i = 0; i < stopCount; i++) {
        printf("%3d. %-16s", i + 1, stops[i].name);
        if (i % 3 == 2) printf("\n");
    }
    printf("\n");
}


// PART 2: FINDING JOURNEYS

// A journey is a list of stops. A ride is one route between two stops.
int path[MAX_STOPS];
int pathLength;      // how many stops the journey has (0 = no journey)
int pathMinutes;     // how long the journey takes

// How much work the last search did. We use this to profile the algorithms.
long steps;          // 1 step = one stop visited or one route looked at
int journeys;        // how many complete journeys the last DFS found

// Gives the route between two stops that are next to each other.
struct Route routeBetween(int a, int b) {
    for (int j = 0; j < stops[a].routeCount; j++) {
        if (stops[a].routes[j].to == b) return stops[a].routes[j];
    }
    return stops[a].routes[0];
}

// BFS: finds the way with the fewest rides.
// color: WHITE = not seen, GRAY = in the queue, BLACK = done.
// dist = rides from the start. parent = the stop we came from.
int color[MAX_STOPS];
int dist[MAX_STOPS];
int parent[MAX_STOPS];

void bfs(int start, int end) {
    steps = 0;
    for (int i = 0; i < stopCount; i++) {
        color[i] = WHITE;
        dist[i] = -1;
        parent[i] = -1;
    }
    int queue[MAX_STOPS];
    int head = 0;
    int tail = 0;
    color[start] = GRAY;
    dist[start] = 0;
    queue[tail] = start;
    tail++;

    while (head < tail) {
        int u = queue[head];         // take the first stop out of the queue
        head++;
        steps++;                     // one stop visited
        for (int j = 0; j < stops[u].routeCount; j++) {
            steps++;                 // one route looked at
            int v = stops[u].routes[j].to;
            if (color[v] == WHITE) {
                color[v] = GRAY;
                dist[v] = dist[u] + 1;
                parent[v] = u;
                queue[tail] = v;     // add it to the end of the queue
                tail++;
            }
        }
        color[u] = BLACK;
    }

    // Build the journey by going back from the end to the start with parent.
    pathLength = dist[end] + 1;
    int stop = end;
    for (int i = pathLength - 1; i >= 0; i--) {
        path[i] = stop;
        stop = parent[stop];
    }
    pathMinutes = 0;
    for (int i = 1; i < pathLength; i++) {
        pathMinutes = pathMinutes + routeBetween(path[i - 1], path[i]).minutes;
    }
}

// DFS: tries every possible journey and keeps the fastest one.
// onPath stops a journey from visiting the same stop twice.
int onPath[MAX_STOPS];
int tryPath[MAX_STOPS];
int endStop;

void dfs(int stop, int rides, int minutes) {
    steps++;                         // one stop visited
    tryPath[rides] = stop;
    if (stop == endStop) {
        journeys++;                  // one complete journey found
        if (pathLength == 0 || minutes < pathMinutes) {   // faster than the best so far, so keep it
            pathLength = rides + 1;
            pathMinutes = minutes;
            for (int i = 0; i < pathLength; i++) path[i] = tryPath[i];
        }
        return;
    }
    onPath[stop] = 1;
    for (int j = 0; j < stops[stop].routeCount; j++) {
        steps++;                     // one route looked at
        struct Route r = stops[stop].routes[j];
        if (onPath[r.to] == 1) continue;
        dfs(r.to, rides + 1, minutes + r.minutes);
    }
    onPath[stop] = 0;    // backtracking: free the stop so other journeys can use it
}

// Finds the fastest way with DFS and puts it in path.
void fastestWay(int start, int end) {
    steps = 0;
    journeys = 0;
    pathLength = 0;
    endStop = end;
    dfs(start, 0, 0);
}

// Prints the journey, one ride on each line.
void printPath() {
    for (int i = 1; i < pathLength; i++) {
        struct Route r = routeBetween(path[i - 1], path[i]);
        printf("  %d. %-5s  %-16s to  %-16s %2d min\n", i, modeNames[r.mode],
               stops[path[i - 1]].name, stops[path[i]].name, r.minutes);
    }
}

// Prints the journey in a few words, for example "TRAIN to Central Station, then BUS to Old Town".
// It names only the stops where you change between bus and train, and the last stop.
void printShort() {
    for (int i = 1; i < pathLength; i++) {
        struct Route r = routeBetween(path[i - 1], path[i]);
        int last = (i == pathLength - 1);
        if (last || routeBetween(path[i], path[i + 1]).mode != r.mode) {
            printf("%s to %s", modeNames[r.mode], stops[path[i]].name);
            if (!last) printf(", then ");
        }
    }
}


// PART 3: SIMULATING PASSENGERS AND THE RESULTS

// A passenger: the time they travel and where they go.
struct Passenger {
    int band;        // 0 = 07:00, 1 = 13:00, 2 = 18:00, 3 = 22:00
    char *from;
    char *to;
};

// The demo day: 24 passengers. Passenger 4 is the example in the assignment:
// at 7:00 AM from the Airport to Old Town, first by train, then by bus.
struct Passenger demoDay[24] = {
    {0, "Greenwood", "Tech Park"},
    {0, "Sunset Heights", "Central Station"},
    {0, "Hilltop", "University"},
    {0, "Airport", "Old Town"},
    {0, "Old Town", "Tech Park"},
    {0, "Greenwood", "Central Station"},
    {0, "Riverside Mall", "Tech Park"},
    {0, "Sunset Heights", "University"},
    {1, "Old Town", "Riverside Mall"},
    {1, "Harbor", "North Market"},
    {1, "University", "Riverside Mall"},
    {1, "Central Station", "Harbor"},
    {1, "Airport", "Central Station"},
    {2, "Tech Park", "Greenwood"},
    {2, "Central Station", "Sunset Heights"},
    {2, "University", "Hilltop"},
    {2, "Tech Park", "Old Town"},
    {2, "Central Station", "Greenwood"},
    {2, "Tech Park", "Riverside Mall"},
    {2, "University", "Sunset Heights"},
    {2, "Old Town", "Airport"},
    {3, "Harbor", "Central Station"},
    {3, "Riverside Mall", "Old Town"},
    {3, "Central Station", "Tech Park"},
};

// What we count about the two algorithms, for each time of day.
int passengers[BANDS];     // passengers who travelled
long bfsSteps[BANDS];      // steps of their BFS searches
long dfsSteps[BANDS];      // steps of their DFS searches

// What we count for all passengers together.
int dfsJourneys;           // journeys DFS tried
int fastestMinutes;        // minutes of the fastest ways, found by DFS
int fewestMinutes;         // minutes of the ways with the fewest rides, found by BFS
int fewestSlower;          // passengers whose fewest rides way is slower than their fastest way
double bfsMicro;           // average time of one BFS, in microseconds
double dfsMicro;           // average time of one DFS, in microseconds

// Sends one passenger through the city, prints one line and counts the work of both algorithms.
void travel(int number, int from, int to, int band) {
    fastestWay(from, to);             // DFS: the fastest way, the one the passenger takes
    int fastest = pathMinutes;
    printf("  %2d. %s to %s: %d min, ", number, stops[from].name, stops[to].name, fastest);
    printShort();
    printf("\n");

    passengers[band]++;
    dfsSteps[band] = dfsSteps[band] + steps;
    dfsJourneys = dfsJourneys + journeys;
    fastestMinutes = fastestMinutes + fastest;

    bfs(from, to);                    // BFS: the way with the fewest rides, to compare
    bfsSteps[band] = bfsSteps[band] + steps;
    fewestMinutes = fewestMinutes + pathMinutes;
    if (pathMinutes > fastest) fewestSlower++;
}

// Measures the average time of one BFS and of one DFS, in microseconds.
// One search is too fast for the clock, so we repeat the searches of all passengers many times.
void timeSearches(int from[], int to[], int count) {
    int rounds = TIMING_RUNS / count;
    int searches = rounds * count;

    clock_t start = clock();
    for (int r = 0; r < rounds; r++) {
        for (int i = 0; i < count; i++) bfs(from[i], to[i]);
    }
    bfsMicro = 1000000.0 * (clock() - start) / CLOCKS_PER_SEC / searches;

    start = clock();
    for (int r = 0; r < rounds; r++) {
        for (int i = 0; i < count; i++) fastestWay(from[i], to[i]);
    }
    dfsMicro = 1000000.0 * (clock() - start) / CLOCKS_PER_SEC / searches;
}

// Prints the results: how efficient the two algorithms were for these passengers.
void showResults() {
    int all = 0;
    long allBfs = 0, allDfs = 0;

    printf("\nRESULTS: ALGORITHM EFFICIENCY\n\n");
    printf("Work done at each time of day. One step is one stop visited or one route looked at.\n\n");
    printf("  %-20s %10s %11s %11s\n", "Time of day", "Passengers", "BFS steps", "DFS steps");
    for (int b = 0; b < BANDS; b++) {
        printf("  %-20s %10d %11ld %11ld\n", bandNames[b], passengers[b], bfsSteps[b], dfsSteps[b]);
        all = all + passengers[b];
        allBfs = allBfs + bfsSteps[b];
        allDfs = allDfs + dfsSteps[b];
    }
    printf("  %-20s %10d %11ld %11ld\n", "All day", all, allBfs, allDfs);

    printf("\nAverage for one search:\n\n");
    printf("  %-8s %-14s %10s %14s\n", "Search", "Finds", "Steps", "Microseconds");
    printf("  %-8s %-14s %10.1f %14.2f\n", "BFS", "fewest rides", (double)allBfs / all, bfsMicro);
    printf("  %-8s %-14s %10.1f %14.2f\n", "DFS", "fastest way", (double)allDfs / all, dfsMicro);

    int routeChecks = 0;      // every route is stored at both of its stops, so 18 routes give 36 checks
    for (int i = 0; i < stopCount; i++) routeChecks = routeChecks + stops[i].routeCount;

    printf("\nBFS always takes %d stops + %d route checks = %d steps. That is V + E.\n",
           stopCount, routeChecks, stopCount + routeChecks);
    printf("DFS tried %.1f journeys for each passenger, so it takes more steps.\n", (double)dfsJourneys / all);
    printf("Fastest ways average %.1f min. Fewest rides ways average %.1f min, slower for %d of %d passenger%s.\n",
           (double)fastestMinutes / all, (double)fewestMinutes / all, fewestSlower, all, plural(all));
}

// Simulates a list of passengers, grouped by time of day, then shows the results.
void simulate(struct Passenger list[], int count, char *title) {
    // Start every simulation from zero, so different cases never mix.
    memset(passengers, 0, sizeof(passengers));
    memset(bfsSteps, 0, sizeof(bfsSteps));
    memset(dfsSteps, 0, sizeof(dfsSteps));
    dfsJourneys = 0;
    fastestMinutes = 0;
    fewestMinutes = 0;
    fewestSlower = 0;

    // Turn the stop names into stop numbers once.
    int from[MAX_PASSENGERS], to[MAX_PASSENGERS];
    for (int i = 0; i < count; i++) {
        from[i] = stopNumber(list[i].from);
        to[i] = stopNumber(list[i].to);
    }

    printf("\n%s: %d passenger%s\n", title, count, plural(count));
    for (int b = 0; b < BANDS; b++) {
        printf("\n%s\n", bandNames[b]);
        int n = 0;
        for (int i = 0; i < count; i++) {
            if (list[i].band == b) {
                travel(i + 1, from[i], to[i], b);
                n++;
            }
        }
        if (n == 0) printf("  nobody\n");
    }
    timeSearches(from, to, count);
    showResults();
}

// PART 4: THE MENU

// Reads a number from 0 to max. If the input is wrong, it asks again.
int readNumber(int max) {
    char line[100];
    int n;
    while (1) {
        if (fgets(line, sizeof(line), stdin) == NULL) exit(0);
        if (sscanf(line, "%d", &n) == 1 && n >= 0 && n <= max) return n;
        printf("Please type a number from 0 to %d: ", max);
    }
}

// Waits until the user presses Enter.
void waitForEnter() {
    char line[100];
    printf("\nPress Enter to go back to the menu.");
    if (fgets(line, sizeof(line), stdin) == NULL) exit(0);
}

// Asks for the start stop, the end stop and the time. Gives 0 if the user typed 0 to go back.
// The answers are put in tripFrom, tripTo and tripBand.
int tripFrom, tripTo, tripBand;

int askTrip() {
    printf("Start stop (1-%d, 0 = back): ", stopCount);
    int a = readNumber(stopCount);
    if (a == 0) return 0;

    printf("End stop (1-%d, 0 = back): ", stopCount);
    int b = readNumber(stopCount);
    while (b == a) {
        printf("Start and end must be different. End stop (1-%d, 0 = back): ", stopCount);
        b = readNumber(stopCount);
    }
    if (b == 0) return 0;

    printf("Time (1 = 07:00, 2 = 13:00, 3 = 18:00, 4 = 22:00, 0 = back): ");
    int t = readNumber(BANDS);
    if (t == 0) return 0;

    tripFrom = a - 1;    // on the screen we count from 1, in the program from 0
    tripTo = b - 1;
    tripBand = t - 1;
    return 1;
}

// Option 3: plan one journey.
void planJourney() {
    printf("\nPLAN A JOURNEY\n\n");
    printStops();
    if (askTrip() == 0) return;
    int from = tripFrom;
    int to = tripTo;
    printf("\n%s to %s at %s\n", stops[from].name, stops[to].name, bandNames[tripBand]);

    fastestWay(from, to);             // DFS
    int fastestRides = pathLength - 1;
    long dfsWork = steps;             // save it, because the next search counts again
    printf("\nFastest way: %d min, %d ride%s\n", pathMinutes, fastestRides, plural(fastestRides));
    printPath();

    bfs(from, to);                    // BFS
    if (pathLength - 1 < fastestRides) {
        printf("\nWay with fewest rides: %d min, %d ride%s\n", pathMinutes, pathLength - 1, plural(pathLength - 1));
        printPath();
    }

    printf("\nWork done: DFS tried %d journey%s in %ld steps. BFS took %ld steps.\n",
           journeys, plural(journeys), dfsWork, steps);
    waitForEnter();
}

// Option 5: the user types in their own passengers, then we simulate them.
void myPassengers() {
    struct Passenger list[MAX_PASSENGERS];
    printf("\nMY OWN PASSENGERS\n\n");
    printStops();
    printf("How many passengers (1-%d, 0 = back): ", MAX_PASSENGERS);
    int count = readNumber(MAX_PASSENGERS);
    if (count == 0) return;

    for (int i = 0; i < count; i++) {
        printf("\nPassenger %d\n", i + 1);
        if (askTrip() == 0) return;
        list[i].band = tripBand;
        list[i].from = stops[tripFrom].name;
        list[i].to = stops[tripTo].name;
    }
    simulate(list, count, "MY PASSENGERS");
    waitForEnter();
}

int main() {
    buildCity();
    int choice = -1;
    while (choice != 0) {
        printf("\nRIVERMEDE TRANSIT\n\n");
        printf("1. Bus routes\n");
        printf("2. Train routes\n");
        printf("3. Plan a journey\n");
        printf("4. Demo day (24 passengers)\n");
        printf("5. My own passengers\n");
        printf("0. Exit\n\n");
        printf("Choose (0-5): ");
        choice = readNumber(5);

        if (choice == 1) printRoutes(BUS);
        if (choice == 2) printRoutes(TRAIN);
        if (choice == 3) planJourney();
        if (choice == 4) simulate(demoDay, 24, "DEMO DAY");
        if (choice == 5) myPassengers();
        if (choice == 1 || choice == 2 || choice == 4) waitForEnter();
    }
    printf("Goodbye!\n");
    return 0;
}
