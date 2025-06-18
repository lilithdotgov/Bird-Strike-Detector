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
            machine.soft_reset()
       
    print("Connected!")


def Disconnect():
    wlan.disconnect()
    wlan.active(False)

#Old code. Should just delete. This was a bad idea from the start
'''
def SendMsg(FileName,msg="test"):
    msg = msg.replace(" ","_")
    Connect()
    
    print("Sending Message to Google Sheet...")
    
    #UPDATE THE GOOGLE SCRIPT! ONLY CARE ABOUT EVENTS! DATA CAN BE OBTAINED THRU SERVER!
    ScriptUrl = "AKfycbwbSY9-Rd6My8EsZ7U6ndwwt3rg3tyRseBj6zZC5X0KTcqDoJuYxTXuhJg24DROzZX34A/exec"
    
    #col1 has value 0 for UTC, and col2 has value 1 for EST
    DataUrl = f"https://script.google.com/macros/s/{ScriptUrl}?col1='0'&col2='1'&col3='{config.MicroNum}'&col4='{FileName}'&col5='{msg}'"

    response = requests.get(url=DataUrl)
    if response.text == "Ok":
        print("Collision Event Successfully Logged!")
    else:
        stor.LogError(4,response.text)
'''      
def SendData(FileName):
    f = open(FileName, "rb")
    content = f.read()
    f.close()
    content = binascii.b2a_base64(content, newline=False)
    
    ### BODY PARAMETERS ###
    contents = bytearray(len(content)+41) #look at precomputing this in the future
    contents[:39] = b'{"message":"New Strike Log","content":"' #len() = 39
    contents[-2:] = b'"}' #len() = 2
    contents[39:-2] = content
    del content

    ### HEADERS ###
    head = {
        "Accept": "application/vnd.github+json",
        "Authorization": f'Bearer {config.GithubAuth}',
        "X-GitHub-Api-Version": "2022-11-28",
        "User-Agent": f'{config.GithubAcc}'}

    Connect()
    UTC = ntp.time()

    ### PATH PARAMETERS ###
    owner = config.GithubAcc
    repo = config.Repository
    path = f'{FileName[:-5]}{UTC}' #is this how it's done?
    
    print("please just work! "+str(gc.mem_free()))
    

    gc.collect() #requests is bad, this is needed because C is the bad language.
    res = requests.put(f'https://api.github.com/repos/{owner}/{repo}/contents/{path}', headers = head, data = contents)
    if path in res.text:
        print(f'Successful data transfer to {repo}!')
        stor.DeleteFile(FileName)
    else:
        stor.LogError(2,res.text)