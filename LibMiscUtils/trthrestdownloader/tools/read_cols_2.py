'''
    To parse and format a historical CSV data file with the given fields.
'''
import re
import sys

f = open("/tmp/ibm.csv", 'r')
g = open("/tmp/cols2.csv", 'w')

if len(sys.argv) <= 1:
    stride = 100
elif int(sys.argv[1]) <= 0:
    stride = -1
else:
    stride = int(sys.argv[1])

'''ric,reuters_date,reuters_time,reuters_time_order,toq,exchange_id,price,volume,buyer_id,bid_price,bid_size,seller_id,ask_price,ask_size,exchange_time,quote_time'''
rg = re.compile("^(.*?)(,.*?)(,.*?)(,.*?)((?:,.*?)+)$")

cnt = 0

while True:
    line = f.readline()
    if len(line) == 0:
        break

    cols = rg.sub(r"\5", line)
    cols = cols.lstrip(',')
    cols = cols.split(',')

    g.write("{:<6},{:<13},{:<10},{:<10},{:<10},{:<10},{:<10},{:<10},{:<10},{:<10},{:<20},{:<12}".format(*cols))

    cnt += 1
    cnt %= stride
    if stride > 0 and 0 == cnt:
        print("Continue ? (Y/N): ")
        ch = input()
        if str(ch).lower() != 'y':
            break

g.close()
f.close()
