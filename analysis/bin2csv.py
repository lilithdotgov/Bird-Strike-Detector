import csv
import os
import struct

import time

path = os.path.dirname(os.path.abspath(__file__)).replace("\\","/")

def BinConvert(path=path):
    os.chdir(path)
    files = os.listdir(path)
    temp = []
    for i in range(0,len(files)):
        if files[i][-4:] == ".bin":
            temp.append(files[i])
    files = temp
    
    for i in range(0,len(files)):
        f = open(files[i],'rb')
        content = f.read()
        
        fcsv = open(f'{files[i][:-4]}'+".csv",'w',newline='')
        fwriter = csv.writer(fcsv)
        
        for i2 in range(0,(len(content)),6):
            fwriter.writerow([9.80665 * struct.unpack('>h',content[i2+0:i2+2])[0] / 256,
                              9.80665 * struct.unpack('>h',content[i2+2:i2+4])[0] / 256,
                              9.80665 * struct.unpack('>h',content[i2+4:i2+6])[0] / 256])
            
        f.close()
        os.remove(files[i])
        fcsv.close()
        print("Successfully converted file!")

while True:
    q1 = input(f'Use current file directory "{path}"? Y/N \n').lower()
    
    if q1 == "y":
        BinConvert()
        
    elif q1 == "n":
        path = input("Please input the correct directory below:\n").replace("\\","/")
        BinConvert(path)
        
    else:
        print("Invalid answer")