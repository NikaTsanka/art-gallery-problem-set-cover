#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include <fstream>
#include <algorithm>
#include <sstream>

using namespace std;

// global vars
#define PI 3.14159265
#define SAMPLE_POINTS 1000
struct Point {
    int x, y;
};
vector<Point> S, C;
vector<vector<Point> > coordinates, subset_CS;
int vertices;
bool done = false;

// X variables
Display *dis;
int screen;
Window win;
GC gc;

// new methods
void drawing_board(void);
void find_guard_positions(void);
int is_point_in_polygon(Point, int);
bool check_intersection(Point, Point, Point, Point);
double det(int, int, int, int, int, int);

string parse_string(string s);
void split(const string &, char, vector<string> &);
vector<string> split(const string &, char);
bool operator==(const Point &, const Point &);

void draw_line(Colormap, char [], XColor &, int, int, vector<Point>);
void draw_line(Colormap colormap, char color_name[], XColor &color, int, int, int, int);
void fill_orthogonal(Colormap, char [], XColor &, XPoint *, int);

// X routines
void init_x(void);
void close_x(void);

int main(int argc, char *argv[]) {
    string line;
    vector<string> lines;
    vector<Point> obstacle;

    printf("Mode: Command-Line\n");

    if (argc == 2) {
        // format (10,10) (50,10) (50,100) (40,100) (40,20) (10,20)
        ifstream input_file(argv[1]);
        while(getline(input_file, line)) {
            obstacle.clear();
            lines.clear();

            line = parse_string(line);
            lines = split(line, ',');

            for (int i = 0; i < lines.size(); ++i) {
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
        int angle;
        vertices = 0;
        for (int i = 0; i < coordinates.size(); ++i) {
            connect = (coordinates[i][0].x == coordinates[i][coordinates[i].size() - 1].x &&
                       coordinates[i][0].y == coordinates[i][coordinates[i].size() - 1].y);
            vertices +=  coordinates[i].size();
            for (int j = 0; j < coordinates[i].size() - 1; ++j) {
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
        drawing_board();
    }
    if (argc == 1) {
        cout << "No files were passed.\n";
    } else {
        cout << "Can't open the passed file. Please make sure the file ends with \'.txt\' and contains no empty lines.\n";
    }
    return 0;
}

void drawing_board(void) {
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
    char gray[] = "#BEBEBE";
    colormap = DefaultColormap(dis, 0);
    // start the main loop
    cout << "You can use \'q\' or \'Q\' to terminate the program at any time. :)\n";
    cout << "Processing...\n";
    while(1) {
        /*get the next event and stuff it into our event variable.
          Note:  only events we set the mask for are detected!*/
        XNextEvent(dis, &event);
        switch (event.type) {
            case Expose:
                if (event.xexpose.count == 0) {
                    // now put the servers on the screen
                    if (!done) {
                        for (int i = 0; i < coordinates.size(); ++i) {
                            // XPoint points;
                            // if first and the last points are the same then no need for closing the obstacle manually
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
                            }
                            fill_orthogonal(colormap, gray, color, points, (int) coordinates[i].size());
                            for (int j = 0; j < coordinates[i].size() - 1; ++j) {
                                draw_line(colormap, black, color, j, j + 1, coordinates[i]);
                            }
                        }
                        find_guard_positions();
                        XFlush(dis);

                        coordinates.clear();
                        S.clear();
                        C.clear();
                        subset_CS.clear();
                    }
                }
                break;
            case KeyPress:
                if (XLookupString(&event.xkey, text, 255, &key, 0) == 1) {
                    /*use the XLookupString routine to convert the invent
                      KeyPress data into regular text.  Weird but necessary...*/
                    if (text[0] == 'q' || text[0] == 'Q') {
                        close_x();
                    }
                    printf("You pressed the %c key!\n", text[0]);
                }
                break;
            default:
                break;
        }
    }
}
void find_guard_positions(void) {
    // init colors
    XColor color;
    Colormap colormap;
    char black[] = "#000000";
    char red[] = "#FF0000";
    char royal_blue[] = "#4169E1";
    char deep_sky_blue[] = "#00BFFF";
    colormap = DefaultColormap(dis, 0);
    // vars
    int guards = 0, point_check = 0;
    // generate sample points 1000
    Point p;
    int hit = 0;
    srand((unsigned int) time(0));
    while (S.size() != SAMPLE_POINTS) {
        int x = rand() % 499; // 0 - 499
        int y = rand() % 499;
        // if rnd is not inside any obstacle, put it in the sample point
        p.x = x, p.y = y;
        hit = 0;
        for (int j = 0; j < coordinates.size(); ++j) {
            // will include polygon sides.
            // if its outside 0 or on the boundary 2
            point_check = is_point_in_polygon(p, j);
            if (0 == point_check || point_check == 2) {
                hit++;
            }
        }
        if (hit == 0) {
            S.push_back(p);
        }
    }
    while (C.size() != SAMPLE_POINTS) {
        int x = rand() % 499; // 0 - 499
        int y = rand() % 499;
        // if rnd is not inside any obstacle, put it in the sample point
        p.x = x, p.y = y;
        hit = 0;
        for (int j = 0; j < coordinates.size(); ++j) {
            point_check = is_point_in_polygon(p, j);
            // will not include polygon sides.
            if (0 == point_check) {
                hit++;
            }
        }
        if (hit == 0) {
            C.push_back(p);
        }
    }

    // now we have both sets
    // for every point generate a subset(points that it sees)
    int init_S_size = (int) S.size();
    int cover = (int) ceil(init_S_size * 0.05);

    while (S.size() >= cover) {
        subset_CS.clear();
        // vars
        vector<Point> subset;
        int intersection;

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
                }
            }
            subset_CS.push_back(subset);
        }

        // find the subset that has the most elements
        int max_size = 0;
        int index = 0;
        for (int k = 0; k < subset_CS.size(); ++k) {
            if (subset_CS[k].size() > max_size) {
                max_size = (int) subset_CS[k].size();
                index = k;
            }
        }
        // draw the visible points
        for (int n = 0; n < subset_CS[index].size(); ++n) {
            draw_line(colormap, deep_sky_blue, color, C[index].x, C[index].y,
                        subset_CS[index][n].x, subset_CS[index][n].y);
            XParseColor(dis, colormap, royal_blue, &color);
            XAllocColor(dis, colormap, &color);
            XSetForeground(dis, gc, color.pixel);
            XFillArc(dis, win, gc, subset_CS[index][n].x - 2, subset_CS[index][n].y - 2, 5, 5, 0, 360 * 64);
            XFlush(dis);
        }
        // draw the guard
        XParseColor(dis, colormap, red, &color);
        XAllocColor(dis, colormap, &color);
        XSetForeground(dis, gc, color.pixel);
        XFillArc(dis, win, gc, C[index].x - 3, C[index].y - 3, 7, 7, 0, 360 * 64);
        XFlush(dis);
        guards++;
        // remove covered sample points
        for (int n = 0; n < subset_CS[index].size(); ++n) {
            S.erase(remove(S.begin(), S.end(), subset_CS[index][n]), S.end());
        }
        // remove the guard position
        C.erase(C.begin() + index);
    }

    // draw sample points that haven't been covered.
    for (int i1 = 0; i1 < S.size(); ++i1) {
        XParseColor(dis, colormap, black, &color);
        XAllocColor(dis, colormap, &color);
        XSetForeground(dis, gc, color.pixel);
        XFillArc(dis, win, gc, S[i1].x - 2, S[i1].y - 2, 5, 5, 0, 360 * 64);
        XFlush(dis);
    }

    int covered = (int) ((100 * S.size()) / init_S_size);
    cout << 100 - covered << "% of total area is covered by the guards. The paintings are safe. :)\n";
    cout << "Number of vertices: " << vertices << " and "<< guards << " guards.\n";
    cout << "Done.\n";
    done = true;
}
int is_point_in_polygon(Point p, int j) {
    // vars
    int t = 0, cy = 0;
    int n = (int) coordinates[j].size();
    int crossings = 0;

    for (int i = 0; i < n - 1; i++) {
        if ((coordinates[j][i].x < p.x && p.x < coordinates[j][i + 1].x) ||
            (coordinates[j][i].x > p.x && p.x > coordinates[j][i + 1].x)) {
            t = (p.x - coordinates[j][i + 1].x) / (coordinates[j][i].x - coordinates[j][i + 1].x);
            cy = t * coordinates[j][i].y + (1 - t) * coordinates[j][i + 1].y;
            if (p.y == cy) {
                return (2); // on boundary
            } else if (p.y > cy) {
                crossings++;
            }
        }
        if ((coordinates[j][i].x == p.x && coordinates[j][i].y <= p.y)) {
            if (coordinates[j][i].y == p.y) {
                return (2); // on boundary
            }
            if (coordinates[j][i + 1].x == p.x) {
                if ((coordinates[j][i].y <= p.y && p.y <= coordinates[j][i + 1].y) ||
                    (coordinates[j][i].y >= p.y && p.y >= coordinates[j][i + 1].y)) {
                    return (2); // on boundary
                }
            } else if (coordinates[j][i + 1].x > p.x) {
                crossings++;
            }
            if (coordinates[j][i - 1].x > p.x) {
                crossings++;
            }
        }
    }
    if ((crossings & 1) == 0) {
        return 1; // inside
    } else {
        return 0; // outside
    }
}
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
        return true;
    } else {
        return false;
    }
}
double det(int px, int py, int qx, int qy, int rx, int ry) {
    return (px*qy)+(py*rx)+(qx*ry)-(qy*rx)-(px*ry)-(py*qx);
};

string parse_string(string str) {
    // erase the front of the string
    str.erase(0,1);
    str.erase(str.length() - 1, 1);
    str.erase(remove(str.begin(), str.end(), ' '), str.end());

    replace(str.begin(), str.end(), '(', ',');
    replace(str.begin(), str.end(), ')', ',');

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
bool operator==(const Point & lhs, const Point & rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

void draw_line(Colormap colormap, char color_name[], XColor &color, int i, int j, vector<Point> current_points) {
    XParseColor(dis, colormap, color_name, &color);
    XAllocColor(dis, colormap, &color);
    XSetForeground(dis, gc, color.pixel);
    XDrawLine(dis,win,gc,current_points[i].x,current_points[i].y,
              current_points[j].x,current_points[j].y);
}
void draw_line(Colormap colormap, char color_name[], XColor &color, int ix, int iy, int jx, int jy) {
    XParseColor(dis, colormap, color_name, &color);
    XAllocColor(dis, colormap, &color);
    XSetForeground(dis, gc, color.pixel);
    XDrawLine(dis,win,gc,ix,iy,jx,jy);
}
void fill_orthogonal(Colormap colormap, char color_name[], XColor &color, XPoint *points, int size) {
    XParseColor(dis, colormap, color_name, &color);
    XAllocColor(dis, colormap, &color);
    XSetForeground(dis, gc, color.pixel);
    XFillPolygon(dis, win, gc, points, size, Complex, CoordModeOrigin);
}

void init_x(void) {
    // get the colors black and white (see section for details)
    unsigned long black,white;

    dis=XOpenDisplay((char *)0);
    screen=DefaultScreen(dis);
    black=BlackPixel(dis,screen), white=WhitePixel(dis, screen);

    win=XCreateSimpleWindow(dis, DefaultRootWindow(dis), 0, 0, 500, 500, 0, black, white);

    XSetStandardProperties(dis, win, "Art Gallery Simulation - Set Cover Approximation", NULL, None, NULL, 0, NULL);
    XSelectInput(dis, win, ExposureMask|KeyPressMask);

    gc=XCreateGC(dis, win, 0,0);

    XSetBackground(dis,gc,white);
    XSetForeground(dis,gc,black);

    XClearWindow(dis, win);
    XMapRaised(dis, win);
};
void close_x(void) {
    XFreeGC(dis, gc);
    XDestroyWindow(dis,win);
    XCloseDisplay(dis);
    cout << "Program was terminated successfully\n";
    exit(1);
};