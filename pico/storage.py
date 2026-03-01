################################ Internal Data Management Code ################################
import config
from random import randint
from ustruct import pack
from machine import reset
from os import remove, rename

def NameGen(): #Generate unique name for file
    
    rng = randint(10000,99999)
    mac = config.mac.replace(":","-")
    name = f'{config.MicroNum}_{mac}_{rng}.bin' 
    
    return name

#TODO: Do some basic compression on the file when you can
#TODO: Maybe include the error log after the data?
def CreateBin(data): #Creates binary file of data. First 24 bytes are calibration settings
    while True:
        name = NameGen()
        
        #creates prepend to data containing accelerometer calibration
        #first scale with order of XYZ then offset with order of XYZ
        #ustruct.pack('6f',value) to turn to 4 byte hex, then ustruct.unpack('6f',value) to return to float
        prepend = bytearray(pack('6f',config.ScaleX,config.ScaleY,config.ScaleZ,config.OffsetX,config.OffsetY,config.OffsetZ)) 
        
        
        try:
            with open(name,'xb') as f:
                f.write(prepend)
                f.write(data)
                print(f'Data logged as {name}')
                f.close()
                break
        except OSError:
            print("Filename already in use. Attempting to create new filename...")
            pass
     
    return name
     
def DeleteFile(FileName):
    try:
        remove(FileName)
        print(f'{FileName} deleted successfully')
    except OSError as err:
        Log(f'Failed to modify file. Error message:\n{err}\n')
        reset()
     
def RenameFile(FileName,NewName):
    try:
        rename(FileName,NewName)
        print(f'{FileName} changed to {NewName} successfully')
    except OSError as err:
        Log(f'Failed to modify file. Error message:\n{err}\n')
        reset()
    
#Logs a message in log.txt for debugging purposes
def Log(msg=""): 
    f = open("log.txt","a+")
    f.write(msg)
    f.close()