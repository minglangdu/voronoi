import tkinter
import random
import threading
import os
from queue import PriorityQueue
import math
import bisect

POINT_AMOUNT = 5
POINT_SIZE = 5
POINT_DIST = 3
SCREEN_SIZE = 600
MARGIN = 30
WAIT_TIME = 750 # in milliseconds
MANHATTAN = False # false -> euclidean distance

root = tkinter.Tk()
canv = tkinter.Canvas(width=SCREEN_SIZE, height=SCREEN_SIZE)
canv.pack()
root.focus_force()

eoi = PriorityQueue()
points = []
beachline = []
directrix = 0;
for i in range(POINT_AMOUNT):
    point = (random.randint(MARGIN, SCREEN_SIZE - MARGIN), random.randint(MARGIN, SCREEN_SIZE - MARGIN))
    canv.create_oval(point[0], point[ 1], point[0] + POINT_SIZE, point[1] + POINT_SIZE, fill="black")
    eoi.put((point[1], point, True)) # (y, (x, y), isFoci)
    points.append(point)

class HalfEdge: # ray
    def __init__(self, x, y, a1, a2):
        self.x1 = x
        self.y1 = y
        # ray is perpendicular to line between arc foci
        aslope = float(a1.y - a2.y) / float(a1.x - a2.x)
        self.slope = -1.0 / aslope 

    def draw(self, canv, x2, y2):
        canv.create_line(self.x1, self.y1, x2, y2)

    def collide(self, arc, dir): # returns intersection point between arc and edge (or -1e9, -1e9) if out of bounds
        # cy = ((cx - h) ** 2) / (4 * p) + k
        # cy = sx (s = self.slope)
        # (cx - h)^2 = 4p(-k + sx)
        p, h, k = arc.phk(dir)
        


class Edge: # full edge – won't grow
    def __init__(self, he, ex, ey):
        self.x1 = he.x1
        self.y1 = he.y1
        self.x2 = ex
        self.y2 = ey

    def draw(self, canv):
        canv.create_line(self.x1, self.y1, self.x2, self.y2)

    def collide(self, arc, dir): 
        pass

class Arc:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.ed = [] # full edges – edges that will not grow
        self.he = [] # half edges – rays extending

    def phk(self, dir):
        p = (self.y - dir) / 2
        h = self.x
        k = (self.y + dir) / 2
        return [p, h, k]
    
    def collide(self, arc, dir):
        # get collision point between arc and arc or otherwise returns -1e9
        p1, h1, k1 = self.phk(dir)
        p2, h2, k2 = arc.phk(dir)
        # y = ((x - h)^2 / 4p + k)
        
        return -1e9

    def getsides(self, dir): # returns x-coordinates where y = 0
        # p always negative, since dir always > focus
        p, h, k = self.phk(dir)
        c = math.sqrt(-4 * p * k) # always positive, as p always negative
        prev =  [h - c, h + c] # before taking in accound full edges and half edges
        # half edges

        # full edges

        return prev

    def draw(self, canv, dir):
        # dir is directrix
        prev = [-1e9, -1e9]
        p, h, k = self.phk(dir)
        if (p == 0):
            return # division by 0
        s1, s2 = [math.floor(x) for x in self.getsides(dir)]
        for cx in range(s1, s2, POINT_DIST):
            # 4p(y - k) = (x - h)^2
            # y = ((x - h)^2 / 4p + k)
            # (h, k) is vertex
            # (self.x, self.y) is focus
            # k is y value between self.y and directrix
            # p is distance from focus to vertex
            cy = ((cx - h) ** 2) / (4 * p) + k
            if (prev == [-1e9, -1e9]):
                prev = [cx, cy]
            canv.create_line(prev[0], prev[1], cx, cy, fill="black")
            prev = [cx, cy]

def update_aux():
    # print("update")
    global points, directrix
    canv.create_rectangle(0, 0, SCREEN_SIZE, SCREEN_SIZE, fill="white")
    for point in points:
        canv.create_oval(point[0], point[1], point[0] + POINT_SIZE, point[1] + POINT_SIZE, fill="blue", outline="blue")
    for arc in beachline:
        arc.draw(canv, directrix)

def update_sweep(stop):
    if not stop.is_set():
        update_aux()
        update_sweep_aux(stop)
        threading.Timer(0.001 * WAIT_TIME, lambda:update_sweep(stop)).start()
    else:
        print("stopped")

def update_sweep_aux(stop):
    global eoi
    if (eoi.empty()):
        stop.set()
        return
    e = eoi.get()
    cur = e[1]
    global directrix
    directrix = cur[1]
    if (e[2]):
        # is foci
        canv.create_line(0, cur[1], SCREEN_SIZE, cur[1], fill="red")
        belowArc = None # arc below it
        l = 0
        r = len(beachline)
        m = -1
        while (r - l > 1):
            m = (l + r) / 2
            if (True):
                l = m
            else:
                r = m
        
        if belowArc:
            # find the intersection point. 
            # resulting line is perpendicular to line between two sites.
            arc = Arc(cur[0], cur[1]) 
        else:
            arc = Arc(cur[0], cur[1])
            beachline.append(arc)
            eoi.put((cur[1] + 100, (0, cur[1] + 100), False))
    else:
        # is edge intersection
        canv.create_line(0, cur[1], SCREEN_SIZE, cur[1], fill="blue")

stop = threading.Event()

thread = threading.Thread(target=update_sweep, args = (stop,))
thread.start()

root.mainloop()

stop.set()

os._exit(0)