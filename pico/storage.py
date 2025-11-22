####################################### Data Management Code ##################################
import config
import random
import ustruct
import machine
import os

def NameGen(): #Generate unique name for file
    
    rng = random.randint(10000,99999)
    mac = config.mac.replace(":","-")
    name = f'{config.MicroNum}_{mac}_{rng}.bin' #Make sure these are all lower-case. Some OS's have case sensitive files
    
    '''
    files = os.listdir()
    temp = []
    for i in range(0,len(files)):
        if files[i][-4:] == ".bin":
            temp.append(files[i])
    files = temp
    del temp
    name = str(len(files))
    name = f'{name}.bin'
    '''
    
    return name

def CreateBin(data): #Creates binary file of data. First 24 bytes are calibration settings
    while True:
        name = NameGen()
        
        #creates prepend to data containing accelerometer calibration
        #first scale with order of XYZ then offset with order of XYZ
        #ustruct.pack('6f',value) to turn to 4 byte hex, then ustruct.unpack('6f',value) to return to float
        prepend = bytearray(ustruct.pack('6f',config.ScaleX,config.ScaleY,config.ScaleZ,config.OffsetX,config.OffsetY,config.OffsetZ)) 
        
        
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
        os.remove(FileName)
        print(f'{FileName} deleted successfully')
    except OSError as err:
        LogError(5,err)
        print(err)
     
def RenameFile(FileName,NewName):
    try:
        os.rename(FileName,NewName)
        print(f'{FileName} changed to {NewName} successfully')
    except OSError as err:
        LogError(5,err)
        print(err)
     
def LogError(error_dtype,msg=""): #work on this more
    Errors = [f'Generic Error Message:\n{msg}\n',
              "Failed to connect to network. Please recheck credentials!\n",
              "Failed to communicate with accelerometer. Please ensure your cables are connected to the correct pins!\n",
              f'Failed to send data to server. Logs will be stored locally until next attempt. Error message:\n{msg}\n',
              f'{msg} Strike(s) successfully logged and sent!\n',
              f'Failed to modify file. Error message:\n{msg}\n',
              f'Main loop failed at unspecific point. Error message:\n{msg}\n']
    f = open("log.txt","a+")
    if error_type in range(0,len(Errors)):
        f.write(Errors[error_type])
        print(Errors[error_type])
        f.close()
    else:    
        f.write(f'Unknown error of type {machine.reset_cause()}/n')
        print(f'Unknown error of type {machine.reset_cause()}/n')
        f.close()
    if error_type != 4:
        machine.reset()
