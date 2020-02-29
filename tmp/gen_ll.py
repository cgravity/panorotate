from math import pi

def d2r(deg):
    return pi/180.0 * deg

def r2d(rad):
    return 180.0 / pi * rad

lats = [d2r(x) for x in xrange(-90,91)]
longs = [d2r(x) for x in xrange(0,361)]

for lat in lats:
 for long in longs:
   print lat, long
   
