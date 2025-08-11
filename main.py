import tkinter
import random
import threading
import os
from queue import PriorityQueue
import math

POINT_AMOUNT = 20
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
beachline = set() 
directrix = 0;
for i in range(POINT_AMOUNT):
    point = (random.randint(MARGIN, SCREEN_SIZE - MARGIN), random.randint(MARGIN, SCREEN_SIZE - MARGIN))
    canv.create_oval(point[0], point[1], point[0] + POINT_SIZE, point[1] + POINT_SIZE, fill="black")
    eoi.put((point[1], point, True)) # (y, (x, y), isFoci)
    points.append(point)
eoi.put((SCREEN_SIZE, (0, SCREEN_SIZE), True))

class Arc:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def draw(self, canv, dir):
        # dir is directrix
        prev = [-1e9, -1e9]
        p = (self.y - dir) / 2
        h = self.x
        k = (self.y + dir) / 2
        if (p == 0):
            return # division by 0
        for cx in range(0, SCREEN_SIZE, POINT_DIST):
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
        canv.create_oval(point[0], point[1], point[0] + POINT_SIZE, point[1] + POINT_SIZE, fill="red", outline="red")
    for arc in beachline:
        arc.draw(canv, directrix)

def update_sweep(stop):
    if not stop.is_set():
        update_aux()
        update_sweep_aux()
        threading.Timer(0.001 * WAIT_TIME, lambda:update_sweep(stop)).start()
    else:
        print("stopped")

def update_sweep_aux():
    global eoi
    if (eoi.empty()):
        return
    e = eoi.get()
    cur = e[1]
    if (e[2]):
        # is foci
        canv.create_line(0, cur[1], SCREEN_SIZE, cur[1], fill="red")
        belowArc = None # arc below it
        l = 0
        r = len(beachline)
        while (r - l > 1):
            m = (l + r) / 2
            if True: # placeholder
                l = m
            else:
                r = m
        
        if belowArc:
            # find the intersection point. 
            # resulting line is perpendicular to line between two sites.
            arc = Arc(cur[0], cur[1]) # placeholder
        else:
            arc = Arc(cur[0], cur[1])
            beachline.add(arc)
        global directrix
        directrix = cur[1]
    else:
        # is edge intersection
        canv.create_line(0, cur[1], SCREEN_SIZE, cur[1], fill="blue")

stop = threading.Event()

thread = threading.Thread(target=update_sweep, args = (stop,))
thread.start()

root.mainloop()

stop.set()

os._exit(0)