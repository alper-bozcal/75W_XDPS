from ast import arg
from asyncio.windows_events import NULL
import csv
import sys
from sys import argv

#Check for arguments if it is used from CLI
#First argument is file to parse, second argument is frequency for parsing.
if len(argv) < 2:
    print("Usage: FFTBot.py [input file path] [First harmonic Frequency]")
    sys.exit()

#use 50 as freq if it is used as drag and drop program or arg2 is not an int
try:
    _freq = int(argv[2])
except:
    _freq = 50
    print("Using frequency as 50")

#exit If file arg can not be found
try:
    file = open(argv[1])
except:
    print("Input File Error")
    sys.exit()

#
outFileName = argv[1].split(".")[0] + "output.csv"
output = open(outFileName, "w")

csvreader = csv.reader(file)
next (csvreader)

ClassALimitsInmA = [0, 1080, 2300, 430, 1140, 300, 770, 230, 400, 184, 330, 153, 210, 131, 150, 115, 132, 102, 118, 92, 107, 83, 97, 76, 90, 70, 83, 65, 77, 61, 72, 57, 68, 54, 64, 51, 60, 48, 57, 46]

print("Base Frequency[Hz],","Frequency[Hz],","Current[mA],","Class A Limit[mA],","Harmonic Number", file = output)

i = 1
for row in csvreader:
    splitteddata = row[0].split('E')
    splitteddata2 = row[1].split('E')

    freq = int(float(splitteddata[0]) * (10 ** int(splitteddata[1].split('+')[-1])) )
    amper = (1000 * float(splitteddata2[0]) * (10 ** int(splitteddata2[1].split('+')[-1])))
    if ( 0.99 < (freq % _freq) < 1.1  or freq % _freq == 0 ) and i < 26:
        if freq < _freq:
            continue
        print( _freq*i, ",", freq, ",", "%.5f," %amper, end = '', file = output)
        print(ClassALimitsInmA[i], ",%i" %(i+1), file = output)
        i += 1

file.close()
output.close()
sys.exit()
