#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include <fstream>
#include <map>
#include <algorithm>
#include <list>
#include <sstream>
#include <ctime>

// https://www.youtube.com/watch?v=3sWaIlXX018
// http://www.sciencedirect.com/science/article/pii/S0166218X09004855
// https://www.youtube.com/watch?v=uqoAb-zEEMQ
// http://www.algorithmic-solutions.info/leda_guide/GeometryAlgorithms.html
// http://www.cs.umd.edu/~mount/
// https://channel9.msdn.com/Events/GoingNative/GoingNative-2012/Keynote-Bjarne-Stroustrup-Cpp11-Style
// http://alienryderflex.com/polygon/
// #http://www.geeksforgeeks.org/how-to-check-if-a-given-point-lies-inside-a-polygon/

using namespace std;

// global vars
#define PI 3.14159265
#define SAMPLE_POINTS 20
#define INF 10000
struct Point {
    int x, y;
};
vector<vector<Point> > coordinates;
vector<Point> S;
vector<Point> C;
vector<vector<Point> > subset_CS;
int vertices;

// here are our X variables
Display *dis;
int screen;
Window win;
GC gc;

// here are our X routines declared
void init_x();
void close_x();
void redraw();

// new methods
void drawing_board();
void find_guard_positions();
//bool is_point_in_polygon(int, int);
void draw_line(Colormap, char [], XColor &, int, int, vector<Point>);
void fill_orthogonal(Colormap, char [], XColor &, XPoint *, int);
string parse_string(string s);
void split(const string &, char, vector<string> &);
vector<string> split(const string &, char);

void draw_line(Colormap colormap, char color_name[], XColor &color, int ix, int iy, int jx, int jy) {
    XParseColor(dis, colormap, color_name, &color);
    XAllocColor(dis, colormap, &color);
    XSetForeground(dis, gc, color.pixel);
    XDrawLine(dis,win,gc,ix - 5,iy- 5,jx- 5,jy- 5);
}

bool onSegment(Point p, Point q, Point r) {
    return q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) &&
           q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y);
}

int orientation(Point p, Point q, Point r) {
    int val = (q.y - p.y) * (r.x - q.x) -
              (q.x - p.x) * (r.y - q.y);

    if (val == 0) return 0;  // colinear
    return (val > 0)? 1: 2; // clock or counterclock wise
}

// The function that returns true if line segment 'p1q1'
// and 'p2q2' intersect.
bool doIntersect(Point p1, Point q1, Point p2, Point q2) {
    // Find the four orientations needed for general and
    // special cases
    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);

    // General case
    if (o1 != o2 && o3 != o4) {
        if (!o4) {
            return false;
        }
        if (!o3) {
            if (o4 != 1) {
                return false;
            }
        }
        return true;
    }

    // Special Cases
    // p1, q1 and p2 are colinear and p2 lies on segment p1q1
    if (o1 == 0 && onSegment(p1, p2, q1)) return true;

    // p1, q1 and p2 are colinear and q2 lies on segment p1q1
    if (o2 == 0 && onSegment(p1, q2, q1)) return true;

    // p2, q2 and p1 are colinear and p1 lies on segment p2q2
    if (o3 == 0 && onSegment(p2, p1, q2)) return true;

    // p2, q2 and q1 are colinear and q1 lies on segment p2q2
    if (o4 == 0 && onSegment(p2, q1, q2)) return true;

    return false; // Doesn't fall in any of the above cases
}

bool is_point_in_polygon(Point p, int j, bool side) {
    /*// http://alienryderflex.com/polygon/
    bool  oddNodes= false;
    for (int k = 0; k < coordinates.size(); ++k) {

        int   i, j= (int) (coordinates[k].size() - 1);

        for (i = 0; i < coordinates[k].size(); i++) {
            if ((coordinates[k][i].second < y && coordinates[k][j].second >= y ||
                 coordinates[k][j].second < y && coordinates[k][i].second >= y) &&
                (coordinates[k][i].first <= x || coordinates[k][j].first <= x)) {
                oddNodes ^= (coordinates[k][i].first +
                             (y - coordinates[k][i].second) /
                             (coordinates[k][j].second - coordinates[k][i].second) *
                             (coordinates[k][j].first - coordinates[k][i].first) < x);
            }
            j = i;
        }
    }
    return oddNodes;*/

    //int count = 0, i = 0;

    //for (int j = 0; j < coordinates.size(); ++j) {
    int n = (int) coordinates[j].size();


    // There must be at least 3 vertices in polygon[]
    if (n < 3)  return false;

    // Create a point for line segment from p to infinite
    Point extreme = {INF, p.y};

    // Count intersections of the above line with sides of polygon
    int count = 0, i = 0;
    do {
        int next = (i+1)%n;

        // Check if the line segment from 'p' to 'extreme' intersects
        // with the line segment from 'coordinates[j][i]' to 'coordinates[j][next]'
        if (doIntersect(coordinates[j][i], coordinates[j][next], p, extreme)) {
            // If the point 'p' is colinear with line segment 'i-next',
            // then check if it lies on segment. If it lies, return true,
            // otherwise false
            if (orientation(coordinates[j][i], p, coordinates[j][next]) == 0 && side) {
                return onSegment(coordinates[j][i], p, coordinates[j][next]);
            }
            count++;
        }
        i = next;
    } while (i != 0);


    // Return true if count is odd, false otherwise
    return (bool) (count & 1);  // Same as (count%2 == 1)

}

double det(int px, int py, int qx, int qy, int rx, int ry) {
    return (px*qy)+(py*rx)+(qx*ry)-(qy*rx)-(px*ry)-(py*qx);
};

bool check_intersection(Point p1, Point q1, Point p2, Point q2) {
    /*Determinant formula
      If (((det (p, q , r) * det(p q s) ) < 0)
      And ((Det (s r p) * det ( s r q))< 0)
      Then the edge pq intersects sr
     */
    double pqr = 0, pqs = 0, srp = 0, srq = 0;
    // p1 = p, q1 = q, r = p2, s = q2

    pqr = det(p1.x, p1.y, // p
              q1.x, q1.y, // q
              p2.x, p2.y); // r

    pqs = det(p1.x, p1.y, // p
              q1.x, q1.y, // q
              q2.x, q2.y); // s

    srp = det(q2.x, q2.y, //s
              p2.x, p2.y, //r
              p1.x, p1.y); // p

    srq = det(q2.x, q2.y, //s
              p2.x, p2.y, //r
              q1.x, q1.y); //q

    if (((pqr * pqs) < 0) && ((srp * srq) < 0)) {
        //cout << "Intersection\n";
        //cout << pqr << " * " << pqs << " " << srp << " * " << srq << endl;
        return true;
    } else {
        //cout << "Nope\n";
        //cout << pqr << " * " << pqs << " " << srp << " * " << srq << endl;
        return false;
    }
}

void find_guard_positions() {
    cout << "Number of vertices: " << vertices << endl;
    // init colors
    XColor color;
    Colormap colormap;
    char black[] = "#000000";
    char crimson[] = "#DC143C";
    char navy_blue[] = "#000080";
    char blue[] = "#0000FF";
    char gray[] = "#BEBEBE";
    colormap = DefaultColormap(dis, 0);

    // generate sample points S. 100
    Point p;
    int hit = 0;
    srand((unsigned int) time(0));
    for (int i = 0; i < SAMPLE_POINTS; ++i) {
        int x = rand() % 499; // 0 - 499
        int y = rand() % 499;
        //cout << x << " , " << y << endl;
        // if rnd is not inside any obstacle, put it in the sample point
        p.x = x, p.y = y;
        hit = 0;
        for (int j = 0; j < coordinates.size(); ++j) {
            // will include polygon sides.
            if (is_point_in_polygon(p, j, false)) {
                //cout << j << " inside\n";
                hit++;
            }
        }
        if (hit == 0) {
            XParseColor(dis, colormap, blue, &color);
            XAllocColor(dis, colormap, &color);
            XSetForeground(dis, gc, color.pixel);
            XFillArc(dis, win, gc, (x - (15 / 2)), (y - (15 / 2)), 5, 5, 0, 360 * 64);
            XFlush(dis);
            S.push_back(p);
        }
    }

    for (int i = 0; i < SAMPLE_POINTS; ++i) {
        int x = rand() % 499; // 0 - 499
        int y = rand() % 499;
        //cout << x << " , " << y << endl;
        // if rnd is not inside any obstacle, put it in the sample point
        p.x = x, p.y = y;
        hit = 0;
        for (int j = 0; j < coordinates.size(); ++j) {
            // will not include polygon sides.
            if (is_point_in_polygon(p, j, true)) {
                //cout << j << " inside\n";
                hit++;
            }
        }
        if (hit == 0) {
            XParseColor(dis, colormap, crimson, &color);
            XAllocColor(dis, colormap, &color);
            XSetForeground(dis, gc, color.pixel);
            XFillArc(dis, win, gc, (x - (15 / 2)), (y - (15 / 2)), 5, 5, 0, 360 * 64);
            XFlush(dis);
                C.push_back(p);
        }
    }

    /*for (int j = 0; j < C.size(); ++j) {
            printf("S[%d] (x = %d, y = %d) \n", j, C[j].x, C[j].y);
        cout << endl;
    }*/

    // now we have both sets
    // for every point generate a subset(points that it sees)
    vector<Point> subset;
    int intersection;
    //for (int k = 0; k < SAMPLE_POINTS; ++k) {
        for (int i = 0; i < C.size(); ++i) {
            subset.clear();
            Point pc;
            // pick a candidate point
            pc.x = C[i].x, pc.y = C[i].y;
            // create edges to every S point.
            for (int j = 0; j < S.size(); ++j) {
                Point ps;
                ps.x = S[j].x, ps.y = S[j].y;
                // now check if the segment cs does not intersect any obstacle
                // edge pc - ps
                // total number of edges of the obstacles
                intersection = 0;
                for (int l = 0; l < coordinates.size(); ++l) {
                    for (int m = 0; m < coordinates[l].size() - 1; ++m) {
                        Point p1, q1;
                        p1.x = coordinates[l][m].x, p1.y = coordinates[l][m].y;
                        q1.x = coordinates[l][m + 1].x, q1.y = coordinates[l][m + 1].y;
                        // now check intersection between pc - ps and p1 and p2
                        if (check_intersection(pc, ps, p1, q1)) {
                            // intersection
                            intersection++;
                        }
                    }
                }
                // if there were no intersections. then it should be 0
                // and we can add ps to subset.
                if (intersection == 0) {
                    subset.push_back(ps);
                    draw_line(colormap, black, color, pc.x, pc.y,  ps.x, ps.y);
                }
            }
            subset_CS.push_back(subset);
        }
    //}

    // god help us: LOL

    /*for (int j = 0; j < subset_CS.size(); ++j) {
        printf("subset_CS[%d] ", j);
        for (int i = 0; i < subset_CS[j].size(); ++i) {
            printf(" (x = %d, y = %d)", subset_CS[j][i].x, subset_CS[j][i].y);
        }
        cout << endl;
    }*/

}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void drawing_board() {
    // the XEvent declaration
    XEvent event;
    // to handle KeyPress Events
    KeySym key;
    // for KeyPress Events
    char text[255];
    // create and open the window
    init_x();
    // init colors
    XColor color;
    Colormap colormap;
    char black[] = "#000000";
    char crimson[] = "#DC143C";
    char navy_blue[] = "#000080";
    char blue[] = "#0000FF";
    char gray[] = "#BEBEBE";
    colormap = DefaultColormap(dis, 0);

    // vars

    // start the main loop
    cout << "You can use \'q\' or \'Q\' to terminate the program at any time. :)\n";
    while(1) {
        /*get the next event and stuff it into our event variable.
          Note:  only events we set the mask for are detected!*/

        XNextEvent(dis, &event);
        switch (event.type) {
            case Expose:
                if (event.type == Expose && event.xexpose.count == 0) {
                    // the window was exposed redraw it
                    redraw();

                    // now put the servers on the screen
                    for (int i = 0; i < coordinates.size(); ++i) {
                        // XPoint points;
                        // if first and the last points are the same then no need for closing the obstacle manually
                        //cout << "Compare: ";
                        //cout << coordinates[i][0].first << " and " << coordinates[i][coordinates[i].size() - 1].first << endl;
                        //cout << "Compare: ";
                        //cout << coordinates[i][0].second << " and " << coordinates[i][coordinates[i].size() - 1].second << endl;

                        if (coordinates[i][0].x != coordinates[i][coordinates[i].size() - 1].x ||
                            coordinates[i][0].y != coordinates[i][coordinates[i].size() - 1].y) {
                            Point p;
                            p.x = coordinates[i][0].x, p.y = coordinates[i][0].y;
                            coordinates[i].push_back(p);
                            vertices += 1;
                        }
                        XPoint points[coordinates[i].size()];
                        for (int j = 0; j < coordinates[i].size(); ++j) {
                            points[j].x = (short) coordinates[i][j].x;
                            points[j].y = (short) coordinates[i][j].y;
                            //cout << "x: " << points[j].x << " y: " << points[j].y << endl;
                        }
                        //fill_orthogonal(colormap, gray, color, points, (int) coordinates[i].size());
                        for (int j = 0; j < coordinates[i].size() - 1; ++j) {
                            draw_line(colormap, black, color, j, j + 1, coordinates[i]);
                        }
                    }

                    /*for (int j = 0; j < coordinates.size(); ++j) {
                        for (int i = 0; i < coordinates[j].size(); ++i) {
                            printf("coordinates[%d][%d] (x = %d, y = %d) \n", j, i, coordinates[j][i].x, coordinates[j][i].y);
                        }
                        cout << endl;
                    }*/
                    XFlush(dis);
                }
                break;

            case KeyPress:
                if (XLookupString(&event.xkey, text, 255, &key, 0) == 1) {
                    /*use the XLookupString routine to convert the invent
                      KeyPress data into regular text.  Weird but necessary...*/

                    if (text[0] == 'q' || text[0] == 'Q') {
                        close_x();
                    } else {
                        find_guard_positions();
                    }
                    printf("You pressed the %c key!\n", text[0]);
                }
                break;

            default:
                break;
        }
    }
}
#pragma clang diagnostic pop

int main(int argc, char *argv[]) {
    string line;
    vector<string> lines;
    vector<Point> obstacle;

    printf("Mode: Command-Line\n");

    if (argc == 2) {
        // (10,10) (50,10) (50,100) (40,100) (40,20) (10,20)
        ifstream input_file(argv[1]);
        while(getline(input_file, line)) {
            obstacle.clear();
            lines.clear();

            line = parse_string(line);
            lines = split(line, ',');

            for (int i = 0; i < lines.size(); ++i) {
                //cout << i << ": " << lines[i] << " i + 1: " << lines[i + 1] << endl;
                Point p;
                p.x = atoi(lines[i].c_str()), p.y = atoi(lines[i + 1].c_str());
                obstacle.push_back(p);
                i+=2;
            }
            coordinates.push_back(obstacle);
        }
        input_file.close();

        // validate the polygons before drawing them

        bool connect;
        int angle; //number_of_obstacles = (int) coordinates.size();
        vertices = 0;
        /*for (int j = 0; j < coordinates.size(); ++j) {
            for (int i = 0; i < coordinates[j].size(); ++i) {
                printf("coordinates[%d][%d] (x = %d, y = %d) \n", j, i, coordinates[j][i].x, coordinates[j][i].y);
            }
            cout << endl;
        }*/

        for (int i = 0; i < coordinates.size(); ++i) {
            connect = (coordinates[i][0].x == coordinates[i][coordinates[i].size() - 1].x &&
                       coordinates[i][0].y == coordinates[i][coordinates[i].size() - 1].y);

            vertices +=  coordinates[i].size();

            for (int j = 0; j < coordinates[i].size() - 1; ++j) {
                // coordinates[i][j].x = x1 , coordinates[i][j].y = y1
                // coordinates[i][j + 1].x = x2 , coordinates[i][j + 1].y = y2
                angle = (int) (atan2((double) coordinates[i][j + 1].y - coordinates[i][j].y,
                                     (double) coordinates[i][j + 1].x - coordinates[i][j].x) * 180 / PI);
                angle += (angle < 0) ? 360 : 0;
                angle += (angle == 0) ? 180 : 0;

                if (j == coordinates[i].size() - 2 && !connect) {
                    angle = (int) (atan2((double) coordinates[i][0].y - coordinates[i][j + 1].y,
                                         (double) coordinates[i][0].x - coordinates[i][j + 1].x) * 180 / PI);
                    angle += (angle < 0) ? 360 : 0;
                    angle += (angle == 0) ? 180 : 0;
                }
                if ((angle != 180 && angle != 90 && angle != 270)) {
                    cout << "Obstacle on line: " << i + 1 << " is not an orthogonal.\n"
                         << "Would you like to skip it? [Y/n] ";
                    char input = '\0';
                    cin >> input;
                    if (input == 'Y' || input == 'y') {
                        vertices -= coordinates[i].size();
                        coordinates.erase(coordinates.begin() + i);
                        i -= 1;
                        break;
                    } else {
                        cout << "Please fix the obstacle or remove it from the file and try again.\n";
                        coordinates.clear();
                        cout << "Program was terminated successfully\n";
                        return 0;
                    }
                }
            }
        }

        /*for (int j = 0; j < coordinates.size(); ++j) {
            for (int i = 0; i < coordinates[j].size(); ++i) {
                printf("coordinates[%d][%d] (x = %d, y = %d) \n", j, i, coordinates[j][i].x, coordinates[j][i].y);
            }
            cout << endl;
        }*/

        // now let's make sure no edge crosses any other edge
        drawing_board();
    }
    if (argc == 1) {
        cout << "No files were passed.\n";
    }

    return 0;
}

void draw_line(Colormap colormap, char color_name[], XColor &color, int i, int j, vector<Point> current_points) {
    XParseColor(dis, colormap, color_name, &color);
    XAllocColor(dis, colormap, &color);
    XSetForeground(dis, gc, color.pixel);
    // the reason why they are offset by 5 is because when points are drawn with 5
    XDrawLine(dis,win,gc,current_points[i].x - 5,current_points[i].y - 5,
              current_points[j].x - 5,current_points[j].y - 5);
}
void fill_orthogonal(Colormap colormap, char color_name[], XColor &color, XPoint *points, int size) {
    //https://www.itec.suny.edu/scsys/vms/ovmsdoc073/v73/5642/5642pro_008.html#graphics_fillpoly_exam
    XParseColor(dis, colormap, color_name, &color);
    XAllocColor(dis, colormap, &color);
    XSetForeground(dis, gc, color.pixel);
    XFillPolygon(dis, win, gc, points, size, Complex, CoordModeOrigin);
}


string parse_string(string str) {
    // erase the front of the string
    str.erase(0,1);
    // erase the ending
    str.erase(str.length() - 1, 1);

    std::replace(str.begin(), str.end(), ' ', '\0');
    std::replace(str.begin(), str.end(), '(', ',');
    std::replace(str.begin(), str.end(), ')', ',');

    return str;
}
void split(const string &s, char delim, vector<string> &elems) {
    stringstream ss;
    ss.str(s);
    string item;
    string cha = "\0";
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
}
vector<string> split(const string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

void init_x() {
    // get the colors black and white (see section for details)
    unsigned long black,white;

    dis=XOpenDisplay((char *)0);
    screen=DefaultScreen(dis);
    black=BlackPixel(dis,screen), white=WhitePixel(dis, screen);

    win=XCreateSimpleWindow(dis, DefaultRootWindow(dis), 0, 0,
                            500, 500, 5, black, white);

    XSetStandardProperties(dis,win,"Art Gallery - Set Cover Algorithm",NULL,None,NULL,0,NULL);
    XSelectInput(dis, win, ExposureMask|ButtonPressMask|KeyPressMask);

    gc=XCreateGC(dis, win, 0,0);

    XSetBackground(dis,gc,white);
    XSetForeground(dis,gc,black);

    XClearWindow(dis, win);
    XMapRaised(dis, win);
};
void close_x() {
    // clear()
    coordinates.clear();

    XFreeGC(dis, gc);
    XDestroyWindow(dis,win);
    XCloseDisplay(dis);
    cout << "Program was terminated successfully\n";
    exit(1);
};
void redraw() {
    XClearWindow(dis, win);
}