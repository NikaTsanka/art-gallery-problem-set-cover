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


using namespace std;

// global vars
vector<vector<pair<int, int> > > coordinates;

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
string parse_string(string s);
void split(const string &, char, vector<string> &);
vector<string> split(const string &, char);

int main(int argc, char *argv[]) {
    string line;
    vector<string> lines;
    vector<pair<int, int> > obstacle;

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
                obstacle.push_back(make_pair(atoi(lines[i].c_str()), atoi(lines[i + 1].c_str())));
                i+=2;
            }
            coordinates.push_back(obstacle);
        }
        input_file.close();

        for (int j = 0; j < coordinates.size(); ++j) {
            for (int i = 0; i < coordinates[j].size(); ++i) {
                printf("coordinates[%d][%d] (x = %d, y = %d) \n", j, i, coordinates[j][i].first, coordinates[j][i].second);
            }
            cout << endl;
        }
    }
    if (argc == 1) {
        cout << "No files were passed.\n";
    }


    return 0;
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

    XSetStandardProperties(dis,win,"TSP Simulation - Held-Karp Algorithm",NULL,None,NULL,0,NULL);
    XSelectInput(dis, win, ExposureMask|ButtonPressMask|KeyPressMask);

    gc=XCreateGC(dis, win, 0,0);

    XSetBackground(dis,gc,white);
    XSetForeground(dis,gc,black);

    XClearWindow(dis, win);
    XMapRaised(dis, win);
};
void close_x() {
    // clear()
    XFreeGC(dis, gc);
    XDestroyWindow(dis,win);
    XCloseDisplay(dis);
    cout << "Program was terminated successfully\n";
    exit(1);
};
void redraw() {
    XClearWindow(dis, win);
}