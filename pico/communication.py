####################################### Networking Code #######################################
from network import WLAN, STA_IF
from urequests import put
from time import time, sleep, gmtime
from ntptime import time as ntptime
import config
import storage as stor
from binascii import b2a_base64
from gc import collect
from machine import RTC, reset

class ConnectionFailure(Exception):
    """Raised when device fails to connect to internet"""
    def __init__(self,issue="Unspecified"):
        self.issue = issue
    
    def __str__(self):
        return f'Device failed to connect to network, Issue: {self.issue}'

#Initialize some WLAN elements
sta_if = WLAN(WLAN.IF_STA)
ap_if = WLAN(WLAN.IF_AP)
wlan = WLAN(STA_IF)

def Connect(): #Make new error checker later
    #Rather insane implementation, meant to match index with status ID, including negative ID's. help(network) to see them all
    status = ["STAT_IDLE","STAT_CONNECTING",None,"STAT_GOT_IP","STAT_WRONG_PASSWORD","STAT_NO_AP_FOUND","STAT_CONNECT_FAIL"]
    
    if wlan.isconnected() == True:
        print("Already connected!")
        
    else:
        wlan.active(True)
        print('Waiting for connection...')
        wlan.connect(config.ssid, config.password)

        failures = 0
        while wlan.isconnected() == False:
            sleep(5)
            ID = wlan.status()
            print(f'Connecting... Status = {status[ID]}')
            failures = failures + 1
            
            if failures > config.ComFailVal:
                raise ConnectionFailure(status[ID])
           
        print("Connected!")

def Disconnect():
    wlan.disconnect()
    wlan.active(False)
    wlan.deinit()

def SendData(FileName,gettime=True):
    f = open(FileName, "rb")
    content = f.read()
    f.close()
    content = b2a_base64(content, newline=False)
    
    ### BODY PARAMETERS ###
    contents = bytearray(len(content)+100) #look at precomputing this in the future
    contents[:98] = b'{"message":"New Strike Log","committer":{"name":"no1","email":"odysseus@fakemail.com"},"content":"' #len() = 98
    contents[-2:] = b'"}' #len() = 2
    contents[98:-2] = content
    del content

    ### HEADERS ###
    head = {
        "Accept": "application/vnd.github+json",
        "Authorization": f'Bearer {config.GithubAuth}',
        "X-GitHub-Api-Version": "2022-11-28",
        "User-Agent": f'{config.GithubAcc}'}#,
        #"Connection": "close"}

    if gettime == True:
        #Assign a time before attempting to get a more accurate one
        UTC = time()
        stor.RenameFile(FileName,f'{FileName[:-9]}{UTC}.bin') #Updates file name with timestamp
        FileName = f'{FileName[:-9]}{UTC}.bin' #Update variable name for further references
        
        for i in range(0,config.ComFailVal): #TODO: maybe make while loop, for is kinda messy here
            Connect()
            
            #TODO: Delete later
            f = open("log.txt","a+") 
            f.write(f'Connection = {wlan.isconnected()}. NTP attempt # = {i}\n')
            f.close()
            
            try:    
                UTC = ntptime()
                tm = gmtime(UTC)
                RTC().datetime((tm[0], tm[1], tm[2], tm[6] + 1, tm[3], tm[4], tm[5], 0)) #Sets current time
                stor.RenameFile(FileName,f'{FileName[:-14]}{UTC}.bin')
                FileName = f'{FileName[:-14]}{UTC}.bin'
                break
            except OSError as err:
                if i == config.ComFailVal - 1: #Checks if enough failures occured, exists process if so
                    stor.Log(f'Failed to get NTP datetime. Using Crystal Oscillator instead. Error message:\n{err}\n')
                    #We don't reset since error may be on NTP side rather than our own
                    break
                sleep(5)    

    ### PATH PARAMETERS ###
    owner = config.GithubAcc
    repo = config.Repository
    path = FileName
    
    if wlan.isconnected() == False:
        Connect()
    
    try:
        collect() #requests is bad, this is needed because C cannot be trusted
        res = put(f'https://api.github.com/repos/{owner}/{repo}/contents/{path}', headers = head, data = contents, timeout = 10)
        if path in res.text: #Basic check to ensure the data was sent properly. TODO: Improve if needed
            print(f'Successful data transfer to {repo}!')
            stor.DeleteFile(path)
        else:
            stor.Log(f'Failed to send data to server. Logs will be stored locally until next attempt. Error message:\n{res.text}\n')
    except Exception as err:
        stor.Log(f'Failed to send data to server. Logs will be stored locally until next attempt. Error message:\n{err}\n')
    
    return FileName

