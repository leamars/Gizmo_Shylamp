# Gizmo: Shylamp
Shylamp is a lamp that is shy. When you leave it alone it's animated, moves about and stays lit up (like a proper lamp),  but when you approach it, it gets shy, stopes moving, and slowly turns off each of its 5 levels.

This code controls a 5-level rotating tower/lamp.

* Levels 1 and 5 rotate the fastest @ 6 rotations/ 15 motor rotations
* Levels 2 and 4 rotate at 1/3 of the speed of levels 1 & 5, @ 3 rotations/ 15 motor rotations
* Level 3 is the slowest and rotates at 1/2 of the speed of levels 1 & 5 @ 2 rotations/ 15 motor rotations
  
When a person is more than 90cm away, Shylamp will happily do its thing.
But when you cross the threshold, it starts to hide. 
First, at < 90cm, Shylamp will rotate all its towers into alignment, to hide the fact that it's "alive",
Then every 15cm closer, Shylamp will turn off the lights on each of the level from bottom to top.
The opposite also holds true.
