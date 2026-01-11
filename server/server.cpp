//Name: Shijie Bu
//SID: 1720680
//CCID: sbu1
//COURSE: CMPUT 275 Winter 2022
//PROJECT NAME: Assign 1 Part 2 Client/Server Application
#include <iostream>
#include <cassert>
#include <fstream>
#include <string>
#include <list>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstdio>
#include <unordered_set>
#include <unordered_map>
#include <utility>

#include "wdigraph.h"
#include "dijkstra.h"

struct Point {
    long long lat, lon;
};

// returns the manhattan distance between two points
long long manhattan(const Point& pt1, const Point& pt2) {
  long long dLat = pt1.lat - pt2.lat, dLon = pt1.lon - pt2.lon;
  return abs(dLat) + abs(dLon);
}

// finds the id of the point that is closest to the given point "pt"
int findClosest(const Point& pt, const unordered_map<int, Point>& points) {
  pair<int, Point> best = *points.begin();

  for (const auto& check : points) {
    if (manhattan(pt, check.second) < manhattan(pt, best.second)) {
      best = check;
    }
  }
  return best.first;
}

// read the graph from the file that has the same format as the "Edmonton graph" file
void readGraph(const string& filename, WDigraph& g, unordered_map<int, Point>& points) {
  ifstream fin(filename);
  string line;

  while (getline(fin, line)) {
    // split the string around the commas, there will be 4 substrings either way
    string p[4];
    int at = 0;
    for (auto c : line) {
      if (c == ',') {
        // start new string
        ++at;
      }
      else {
        // append character to the string we are building
        p[at] += c;
      }
    }

    if (at != 3) {
      // empty line
      break;
    }

    if (p[0] == "V") {
      // new Point
      int id = stoi(p[1]);
      assert(id == stoll(p[1])); // sanity check: asserts if some id is not 32-bit
      points[id].lat = static_cast<long long>(stod(p[2])*100000);
      points[id].lon = static_cast<long long>(stod(p[3])*100000);
      g.addVertex(id);
    }
    else {
      // new directed edge
      int u = stoi(p[1]), v = stoi(p[2]);
      g.addEdge(u, v, manhattan(points[u], points[v]));
    }
  }
}

int create_and_open_fifo(const char * pname, int mode) {
  // creating a fifo special file in the current working directory
  // with read-write permissions for communication with the plotter
  // both proecsses must open the fifo before they can perform
  // read and write operations on it
  if (mkfifo(pname, 0666) == -1 && errno != EEXIST) {
    cout << "Unable to make a fifo. Ensure that this pipe does not exist already!" << endl;
    exit(-1);
  }

  // opening the fifo for read-only or write-only access
  // a file descriptor that refers to the open file description is
  // returned
  int fd = open(pname, mode);

  if (fd == -1) {
    cout << "Error: failed on opening named pipe." << endl;
    exit(-1);
  }

  return fd;
}

// keep in mind that in part 1, the program should only handle 1 request
// in part 2, you need to listen for a new request the moment you are done
// handling one request
int main() {
  WDigraph graph;
  unordered_map<int, Point> points;

  const char *inpipe = "inpipe";
  const char *outpipe = "outpipe";

  // Open the two pipes
  int in = create_and_open_fifo(inpipe, O_RDONLY);
  cout << "inpipe opened..." << endl;
  int out = create_and_open_fifo(outpipe, O_WRONLY);
  cout << "outpipe opened..." << endl;  

  // build the graph
  readGraph("server/edmonton-roads-2.0.1.txt", graph, points);

  // read a request
  FILE* fin = fdopen(in, "r");
  if (!fin) {
    cout << "Error: fdopen(in) failed." << endl;
    exit(-1);
  }

  char line1[128], line2[128];
  Point sPoint, ePoint;

  while (true) {
    // first line: start point (or 'Q')
    if (!fgets(line1, sizeof(line1), fin)) break;
    if (line1[0] == 'Q') break;

    // second line: end point
    if (!fgets(line2, sizeof(line2), fin)) break;

    double slat, slon, elat, elon;
    if (sscanf(line1, "%lf %lf", &slat, &slon) != 2) continue;
    if (sscanf(line2, "%lf %lf", &elat, &elon) != 2) continue;

    sPoint.lat = (long long)(slat * 100000);
    sPoint.lon = (long long)(slon * 100000);
    ePoint.lat = (long long)(elat * 100000);
    ePoint.lon = (long long)(elon * 100000);

    int start = findClosest(sPoint, points);
    int goal  = findClosest(ePoint, points);

    unordered_map<int, PIL> tree;
    dijkstra(graph, start, tree);

    // path reconstruction
    list<int> path;
    int cur = goal;
    unordered_set<int> seen;

    while (cur != start) {
      if (!seen.insert(cur).second) {
        break;
      }
      auto it = tree.find(cur);
      if (it == tree.end()) {
        write(out, "E\n", 2);
        goto NEXT_REQUEST;
      }
      path.push_front(cur);
      cur = it->second.first;
    }
    path.push_front(start);

    // safe output formatting
    char buf[64];
    for (int v : path) {
      int n = snprintf(buf, sizeof(buf), "%.5f %.5f\n",
                       points[v].lat / 100000.0,
                       points[v].lon / 100000.0);
      if (n > 0) write(out, buf, (size_t)n);
    }
    write(out, "E\n", 2);

  NEXT_REQUEST:
    continue;
  }
    //close pipes and flush them
    fclose(fin); close(out);
    unlink(inpipe); unlink(outpipe);
    return 0;
}
