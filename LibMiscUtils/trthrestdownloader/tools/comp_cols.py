import pandas

'''
Difference found:
(1) Two redundant Exch Time:
    cols.csv:854: Trade ,             ,          ,          ,          ,          ,          ,          ,          ,          ,09:28:14.397653000
    cols.csv:858: Trade ,             ,          ,          ,          ,          ,          ,          ,          ,          ,09:34:41.617495000
(2)
    cols.csv:Price column may have 4-digits fractions, while no such in cols2.csv.
'''

DIFF_THRESHOLD = 0.0005

def to_val_str_list(dfCol):
    return [dfCol[i+1].strip() for i in range(len(dfCol))]

def diff_prices(price1, price2, out_file):
    if len(price1) * len(price2) == 0 or len(price1) != len(price2):
        print("ERROR: Please provide non-empty data files with the same number of lines.")
        return

    with open(out_file, 'w') as of:
        for i, (p1, p2) in enumerate(zip(price1, price2)):
            if len(p1) + len(p2) == 0:
                continue
            # if abs(float(p1) - float(p2)) > 0.0005:
            #     print("float1 = {}, float2 = {}, abs = {}".format(float(p1), float(p2), abs(float(p1) - float(p2))))
            if (len(p1) * len(p2) > 0) and (round(abs(float(p1) - float(p2)), 4) <= DIFF_THRESHOLD):
                continue

            csv_line = i + 2
            of.write("{:>8}:  L={:<12},  R={:<12}\n".format(csv_line, p1, p2))


def diff_str(str1, str2, out_file):
    if len(str1) * len(str2) == 0 or len(str1) != len(str2):
        print("ERROR: Please provide non-empty data files with the same number of lines.")
        return

    with open(out_file, 'w') as of:
        for i, (s1, s2) in enumerate(zip(str1, str2)):
            if s1 == s2:
                continue

            csv_line = i + 2
            of.write("{:>8}:  L= {:<15},  R= {:<15}\n".format(csv_line, s1, s2))

#------------------------------------------------------------
dfColsREST = pandas.read_csv('./raw/cols.csv')
dfColsPSQL = pandas.read_csv('./raw/cols2.csv')

print('Diff: Prices...')
prices1 = to_val_str_list(dfColsREST['Price     '][1:])
prices2 = to_val_str_list(dfColsPSQL['price     '][1:])
diff_prices(prices1, prices2, './raw/diff_price.txt')

prices1 = to_val_str_list(dfColsREST['Bid Price '][1:])
prices2 = to_val_str_list(dfColsPSQL['bid_price '][1:])
diff_prices(prices1, prices2, './raw/diff_bp.csv')

prices1 = to_val_str_list(dfColsREST['Ask Price '][1:])
prices2 = to_val_str_list(dfColsPSQL['ask_price '][1:])
diff_prices(prices1, prices2, './raw/diff_ap.csv')
print('\tDone.')

#------------------------------------------------------------

print('Diff: Others...')
str1 = to_val_str_list(dfColsREST['Type  '][1:])
str2 = to_val_str_list(dfColsPSQL['toq   '][1:])
diff_str(str1, str2, './raw/diff_toq.txt')

str1 = to_val_str_list(dfColsREST['Volume    '][1:])
str2 = to_val_str_list(dfColsPSQL['volume    '][1:])
diff_str(str1, str2, './raw/diff_vol.txt')

str1 = to_val_str_list(dfColsREST['Ex/Cntrb.ID  '][1:])
str2 = to_val_str_list(dfColsPSQL['exchange_id  '][1:])
diff_str(str1, str2, './raw/diff_exid.txt')
print('\tDone.')

print('Please refer to folder ./raw/')