####################################### Analysis Code #########################################

#function removes repeats of data caused by reading data registers faster than they are updated
def StripData(data,bps=6): #bps = bytes per sample
    prev = b'0x00'
    new_data = bytearray(0)
    
    for i in range(0,len(data)/bps):
        if prev != data[bps*i:bps*i+bps].hex():
            new_data.extend(data[bps*i:bps*i+bps])

        prev = data[bps*i:bps*i+bps].hex()
                   
    
    del prev
    del data
    return new_data[0:bps*1024]


def Analysis(): #make into real function eventually
    return True

