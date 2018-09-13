'''
    To parse and format a historical CSV data file with the given fields.
'''
import re
import sys

f = open("raw/data.csv", 'r')
g = open("raw/cols.csv", 'w')

if len(sys.argv) <= 1:
    stride = 100
elif int(sys.argv[1]) <= 0:
    stride = -1
else:
    stride = int(sys.argv[1])

'''#RIC(,Domain),Date-Time(,GMT Offset),Type,Ex/Cntrb.ID,Price,Volume,Buyer ID,Bid Price,Bid Size,Seller ID,Ask Price,Ask Size,Exch Time,Date'''
rg = re.compile("^(.*?)(,.*?)(,.*?)(,.*?)((?:,.*?)+)$")

cnt = 0

while True:
    line = f.readline()
    if len(line) == 0:
        break

    cols = rg.sub(r"\5", line)
    cols = cols.lstrip(',')
    cols = cols.split(',')

    # g.write("{:<8},{:<32},{:<6},{:<13},{:<10},{:<10},{:<10},{:<10},{:<10},{:<10},{:<10},{:<10},{:<20},{:<12}".format(*cols))
    g.write("{:<6},{:<13},{:<10},{:<10},{:<10},{:<10},{:<10},{:<10},{:<10},{:<10},{:<20}".format(*cols))

    cnt += 1
    cnt %= stride
    if stride > 0 and 0 == cnt:
        print("Continue ? (Y/N): ")
        ch = input()
        if str(ch).lower() != 'y':
            break

g.close()
f.close()
