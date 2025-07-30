import tkinter
import random
from queue import PriorityQueue

POINT_AMOUNT = 20
POINT_SIZE = 5
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
for i in range(POINT_AMOUNT):
    point = (random.randint(MARGIN, SCREEN_SIZE - MARGIN), random.randint(MARGIN, SCREEN_SIZE - MARGIN))
    canv.create_oval(point[0], point[1], point[0] + POINT_SIZE, point[1] + POINT_SIZE, fill="black")
    eoi.put((point[1], point))
    points.append(point)

def update():
    global points
    canv.create_rectangle(0, 0, SCREEN_SIZE, SCREEN_SIZE, fill="white")
    for point in points:
        canv.create_oval(point[0], point[1], point[0] + POINT_SIZE, point[1] + POINT_SIZE, fill="black")

def update_sweep():
    update()
    global eoi
    if (eoi.empty()):
        return
    cur = eoi.get()[1]
    canv.create_line(0, cur[1], SCREEN_SIZE, cur[1], fill="red")

    canv.after(WAIT_TIME, update_sweep)

canv.after(WAIT_TIME, update_sweep)
root.mainloop()