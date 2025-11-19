####################################### Configuration Code ####################################
import json
import communication as comm
import ubinascii

with open("config.json","r") as RFile:
    config = json.load(RFile)
    
MicroNum = config["MicroNum"] #Unique number for each device for easy marking
OffsetX = config["OffsetX"]
OffsetY = config["OffsetY"] 
OffsetZ = config["OffsetZ"]
ScaleX = config["ScaleX"]
ScaleY = config["ScaleY"]
ScaleZ = config["ScaleZ"]
SendVal = config["SendVal"] #Number of logged strikes before it is uploaded to server
ComFailVal = config["ComFailVal"] #Number of connection attempts until timeout
GithubAuth = config["GithubAuth"] #Auth Token for Github
Repository = config["Repository"]
GithubAcc = config["GithubAcc"]

#0 for NJIT internet, 1 for home
mac = config["mac"] #unique number for each device
ssid = config["ssid"][0]
password = config["password"][0]

def SaveConfig(ObjectName,Value,Index=-1):
    if Index == -1:
        config[ObjectName] = Value
    else:
        config[ObjectName][Index] = Value
    with open("config.json","w") as WFile:
        json.dump(config,WFile)

def Default(MicroNum_=0):
    comm.wlan.active(True)
    mac = ubinascii.hexlify(comm.wlan.config('mac'),':').decode()
    SaveConfig("mac",mac)
    print(mac)
    comm.Disconnect()
    
    SaveConfig("ComFailVal",10)
    SaveConfig("SendVal",0)
    
    SaveConfig("OffsetX",0)
    SaveConfig("OffsetY",0)
    SaveConfig("OffsetZ",0)
    
    Scale = (9.80665*8)/(2**11) #TODO: Have this change with user-selected ranges and percision
    SaveConfig("ScaleX",Scale)
    SaveConfig("ScaleY",Scale)
    SaveConfig("ScaleZ",Scale)
    
    SaveConfig("MicroNum",MicroNum_)