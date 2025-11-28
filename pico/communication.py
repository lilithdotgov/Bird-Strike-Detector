####################################### Networking Code #######################################
import network
import socket
import urequests as requests
import time
import ntptime as ntp
import config
import storage as stor
import json
import binascii
import gc
import machine

sta_if = network.WLAN(network.WLAN.IF_STA)
ap_if = network.WLAN(network.WLAN.IF_AP)
wlan = network.WLAN(network.STA_IF)

def Connect(): #Make new error checker later
    if wlan.isconnected() == True:
        print("Already connected!")
        
    else:
        wlan.active(True)
        print('Waiting for connection...')
        wlan.connect(config.ssid, config.password)

        '''
        f = open("log.txt","a+")
        f.write("Starting Connection"+"\n")
        f.close()
        '''
        failures = 0
        while wlan.isconnected() == False:
            time.sleep(5)
            info = wlan.status()
            print(f'Connecting... Status = {info}')
            failures = failures + 1
            
            if failures > config.ComFailVal:
                stor.LogError("Failed to connect to network. Please recheck credentials!\n")
           
        print("Connected!")

def Disconnect():
    wlan.disconnect()
    wlan.active(False)

def SendData(FileName,gettime=True):
    f = open(FileName, "rb")
    content = f.read()
    f.close()
    content = binascii.b2a_base64(content, newline=False)
    
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
        UTC = time.time()
        stor.RenameFile(FileName,f'{FileName[:-9]}{UTC}.bin') #Updates name with timestamp
        FileName = f'{FileName[:-9]}{UTC}.bin' #Update name for further references
        
        for i in range(0,config.ComFailVal): #make while loop, for is messy here
            Connect()
            try:    
                UTC = ntp.time()
                tm = time.gmtime(UTC)
                machine.RTC().datetime((tm[0], tm[1], tm[2], tm[6] + 1, tm[3], tm[4], tm[5], 0)) #Sets current time
                stor.RenameFile(FileName,f'{FileName[:-14]}{UTC}.bin')
                FileName = f'{FileName[:-14]}{UTC}.bin'
                break
            except OSError as err:
                if i == config.ComFailVal - 1: #Checks if enough failures occured, exists process if so
                    stor.LogError(f'Failed to get NTP datetime. Using Crystal Oscillator instead. Error message:\n{err}\n',reset=False)
                    #We don't reset since error may be on NTP side rather than our own
                    break
                time.sleep(5)    

    ### PATH PARAMETERS ###
    owner = config.GithubAcc
    repo = config.Repository
    path = FileName
    
    try:
        print("attempting to send "+str(gc.mem_free()))
        gc.collect() #requests is bad, this is needed because C cannot be trusted
        res = requests.put(f'https://api.github.com/repos/{owner}/{repo}/contents/{path}', headers = head, data = contents)
        if path in res.text: #Basic check to ensure the data was sent properly. TODO: Improve if needed
            print(f'Successful data transfer to {repo}!')
            stor.DeleteFile(path)
        else:
            stor.LogError(f'Failed to send data to server. Logs will be stored locally until next attempt. Error message:\n{res.text}\n')
    except Exception as err:
        stor.LogError(f'Failed to send data to server. Logs will be stored locally until next attempt. Error message:\n{err}\n')
    

