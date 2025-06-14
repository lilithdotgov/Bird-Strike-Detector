####################################### Networking Code #######################################
import network
import socket
import urequests as requests
import time
import config

sta_if = network.WLAN(network.WLAN.IF_STA)
ap_if = network.WLAN(network.WLAN.IF_AP)
wlan = network.WLAN(network.STA_IF)

def Connect(): #Make new error checker later
    wlan.active(True)
    print('Waiting for connection...')
    wlan.connect(config.ssid, config.password)

    while wlan.isconnected() == False:
        time.sleep(3)
        info = wlan.status()
        print(f'Connecting... Status = {info}')
       
    print("Connected!")


def Disconnect(): #work on name,  maybe just make same function as connect?
    wlan.disconnect()
    wlan.active(False)

def SendData(FileName,msg=""):
    msg = msg.replace(" ","_")
    Connect()
    
    print("Sending Data...")
    
    #UPDATE THE GOOGLE SCRIPT! ONLY CARE ABOUT EVENTS! DATA CAN BE OBTAINED THRU SERVER!
    ScriptUrl = "AKfycbwbSY9-Rd6My8EsZ7U6ndwwt3rg3tyRseBj6zZC5X0KTcqDoJuYxTXuhJg24DROzZX34A/exec"
    
    #col1 has value 0 for UTC, and col2 has value 1 for EST
    DataUrl = f"https://script.google.com/macros/s/{ScriptUrl}?col1='0'&col2='1'&col3='{config.MicroNum}'&col4='{FileName}'&col5='{msg}'"


    response = requests.get(url=DataUrl)
    if response.text == "Ok":
        print("Collision Event Successfully Logged!")
    else:
        print("Failure to Send Data. See Log Below:")
        print(response.text)
#print(response.text)
    #with requests.get(url=DataUrl) as response: #REPLACE
        #print(response.text) #DONT NEED THIS, VERY SLOW. REMOVE EVENTUALLY OR ADD CHECK FOR ERROR
    #    print("Data Sent!")
    #adafruit_connection_manager.connection_manager_close_all(None) #REPLACE 
