# Design notes

How Rivermede Transit models the city, finds journeys and measures the two algorithms.
Everything lives in [`src/transit.c`](../src/transit.c), in four parts.

| Part | Responsibility | Main functions |
|------|----------------|----------------|
| 1. City map | The graph and the Rivermede data | `stopNumber`, `addRoute`, `buildCity`, `printRoutes`, `printStops` |
| 2. Finding journeys | BFS and DFS | `bfs`, `dfs`, `fastestWay`, `printPath`, `printShort` |
| 3. Simulation and results | Passengers, counters, profiling | `travel`, `timeSearches`, `showResults`, `simulate` |
| 4. Menu | Input checking and the five menu options | `readNumber`, `askTrip`, `planJourney`, `myPassengers`, `main` |

## 1. The graph

The city is an **undirected, weighted graph**.

- a stop is a vertex;
- a bus or train route is an edge;
- the weight of an edge is the ride time in minutes;
- each edge also remembers whether it is a bus or a train route.

Rivermede has V = 12 stops and E = 18 routes (12 bus, 6 train).

The graph is stored as an **adjacency list**:

```c
struct Route { int to; int minutes; int mode; };                         // BUS or TRAIN
struct Stop  { char name[20]; struct Route routes[10]; int routeCount; };
struct Stop stops[MAX_STOPS];
```

Both searches only ever ask "which routes leave this stop?", and the list stores exactly those
routes. Because every route works in both directions, `addRoute` stores it at **both** of its
stops, so the 18 routes give 36 list entries. The space used is O(V + E).

## 2. Finding journeys

### BFS: the fewest rides

`bfs(start, end)` is the textbook breadth-first search.

- `color[]` tracks progress: `WHITE` = not seen, `GRAY` = waiting in the queue, `BLACK` = finished.
- `dist[]` is the number of rides from the start, and `parent[]` is the stop we came from.
- A plain array is used as the queue, with `head` and `tail` indexes.
- When the search ends, the journey is rebuilt by following `parent[]` back from the end stop.

Every stop is taken out of the queue once (V = 12) and every list entry is looked at once
(2E = 36), so the work is **always V + E = 48 steps**, whichever two stops are chosen.

BFS counts **rides, not minutes**. The journey with the fewest rides is therefore not always the
quickest one.

### DFS with backtracking: the fastest

`fastestWay(start, end)` calls `dfs`, which tries **every journey** that never visits a stop twice
and keeps the quickest.

- `tryPath[]` holds the journey being built and `onPath[]` marks the stops already on it.
- When the end stop is reached the journey is complete: `journeys` goes up by one, and if its
  minutes are lower than the best so far, it is copied into `path[]`.
- When all routes of a stop have been tried, `onPath[stop]` is cleared again. This is the
  **backtracking** step: it frees the stop so that other journeys can use it.

The number of possible journeys can grow very quickly on a big map (exponentially in the worst
case). On a 12-stop city it stays small: DFS tried 26.8 journeys per passenger on average on the
demo day, and at most 58 for any pair of stops in Rivermede.

### Why both?

They answer two different questions, and comparing them is the point of the project:

- BFS answers "which journey has the fewest rides?" cheaply and with a guaranteed cost.
- DFS answers "which journey takes the fewest minutes?" because it looks at all of them.

For the 24 passengers of the demo day, the BFS journey is slower than the fastest one for 4 of
them. For example, Greenwood to Tech Park takes 24 minutes in 3 rides on the fastest way, but 25
minutes in 2 rides on the fewest-rides way.

## 3. Simulation and profiling

### Passengers and times of day

A passenger is a start stop, an end stop and a time of day: 07:00 Morning Peak, 13:00 Midday,
18:00 Evening Peak or 22:00 Night. The demo day has 24 passengers. The user can type up to 30
passengers of their own.

The time of day is used to group the passengers and the results. The network and the ride times
are the same at every hour; timetables and crowding are not modelled.

### Steps

Counting steps gives the same result on every machine, so it is the main measure. Both searches
use the same rule:

> 1 step = one stop visited + one route looked at

This makes the BFS and DFS numbers directly comparable.

### Time

One search is too short for the clock to measure. `timeSearches` therefore repeats the searches
of all passengers about 100,000 times (`TIMING_RUNS`) between two `clock()` calls and divides by
the number of searches. The result is the average time of **one** search in microseconds.
Unlike steps, it depends on the machine, the compiler and its optimisation flags.

## 4. Limits

- The map is built into `buildCity()`. The arrays are fixed-size (20 stops, 10 routes per stop,
  names up to 19 characters) and `addRoute` has no bounds checks, because the map never changes.
- The exhaustive DFS does not scale to large maps. For fastest journeys on a bigger network,
  Dijkstra's algorithm would be the better choice.
- Only the user's menu input is validated. It is read with `fgets` and `sscanf`, and a wrong
  entry is asked for again.
