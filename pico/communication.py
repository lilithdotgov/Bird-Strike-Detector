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
    wlan.active(True)
    print('Waiting for connection...')
    wlan.connect(config.ssid, config.password)

    failures = 0
    while wlan.isconnected() == False:
        time.sleep(3)
        info = wlan.status()
        print(f'Connecting... Status = {info}')
        failures = failures + 1
        
        if failures > config.ComFailVal:
            stor.LogError(0)
       
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

    Connect()
    if gettime == True:
        for i in range(0,config.SendVal): #make while loop, for is messy here
            try:
                UTC = ntp.time()
                tm = time.gmtime(UTC)
                machine.RTC().datetime((tm[0], tm[1], tm[2], tm[6] + 1, tm[3], tm[4], tm[5], 0))
            except OSError:
                if i == config.SendVal - 1:
                    stor.LogError(2,"Failed to get datetime")
                time.sleep(3)    
            else:
                stor.RenameFile(FileName,f'{FileName[:-9]}{UTC}.bin')
                break

    ### PATH PARAMETERS ###
    owner = config.GithubAcc
    repo = config.Repository
    if gettime == True:
        path = f'{FileName[:-9]}{UTC}.bin'
    else:
        path = FileName
    
    try:
        print("attempting to send "+str(gc.mem_free()))
        gc.collect() #requests is bad, this is needed because C cannot be trusted
        res = requests.put(f'https://api.github.com/repos/{owner}/{repo}/contents/{path}', headers = head, data = contents)
        if path in res.text:
            print(f'Successful data transfer to {repo}!')
            stor.DeleteFile(path)
            #break
        else:
            stor.LogError(2,res.text)
    except Exception as exc:
        stor.LogError(2,exc)
    

