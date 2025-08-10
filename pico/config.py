####################################### Configuration Code ####################################
import json

with open("config.json","r") as RFile:
    config = json.load(RFile)
    
MicroNum = config["MicroNum"] #Unique number for each device for easy marking
OffsetX = config["OffsetX"]
OffsetY = config["OffsetY"] 
OffsetZ = config["OffsetZ"]
ScaleX = config["ScaleX"]
ScaleY = config["ScaleY"]
ScaleZ = config["ScaleZ"]
PowerMode = config["PowerMode"] #0 for no power saving, 1 for power saving mode
SendVal = config["SendVal"] #Number of logged strikes before it is uploaded to server
ComFailVal = config["ComFailVal"] #Number of connection attempts until timeout
GithubAuth = config["GithubAuth"] #Auth Token for Github
Repository = config["Repository"]
GithubAcc = config["GithubAcc"]

#0 for NJIT internet, 1 for home
mac = config["mac"] #unique number for each device
ssid = config["ssid"][1]
password = config["password"][1]

def SaveConfig(ObjectName,Value,Index=-1):
    if Index == -1:
        config[ObjectName] = Value
    else:
        config[ObjectName][Index] = Value
    with open("config.json","w") as WFile:
        json.dump(config,WFile)
