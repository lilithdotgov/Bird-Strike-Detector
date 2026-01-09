####################################### Main Code Loop ########################################
import communication as comm
import accelerometer as acc
import storage as stor
import analysis as anal
import machine
from picozero import pico_led
import time
import os
import sys
import gc

#Functions:
def ISR(pin): #Handles the interrupt
    global strike_flag
    strike_flag = True
    if sleep_flag:
        acc.ResetIntrState() #Reset interrupt state to allow for future interrupts
        
def Strike(): #Main function that collects, stores, and sends the strike data
    try:
        pico_led.on()
        
        data = acc.FastStream() #Gathers data
        
        name = stor.CreateBin(data) #returns name of the newly made file
        del data

        name = comm.SendData(name) #Sends data to Github repository, returns new name of file
        
        nfiles = 0 #Number of files, used to keep track of how many were successfully sent
        if name not in os.listdir():
            nfiles = nfiles + 1
        del name
        
        #Checks to see if there exist past strikes that weren't sent
        files = os.listdir()
        temp = []
        for i in range(0,len(files)):
            if files[i][-4:] == ".bin":
                temp.append(files[i])
                nfiles = nfiles + 1
        files = temp
        del temp
        
        #Sends data to Github repository
        for i in range(0,len(files)):
            name = files[i]
            #print(f'mem free before calling function {i+1} '+str(gc.mem_free()))
            state = False
            if name.index(".") - name.index("_",2) < 10: #Checks whether we already gave the file a timestamp. TODO: Add extra flag to indicate that file had no timestamp originally
                state = True
            comm.SendData(name,gettime=state)
            if name in os.listdir(): #Checks whether we actually sent the file, if still in os then decriment nfiles
                nfiles = nfiles - 1
        del files

        stor.LogError(f'{nfiles} Strike(s) successfully logged and sent!\n',reset=False)
        del nfiles
        
        #Double blink
        pico_led.off()
        time.sleep(0.1)
        pico_led.on()
        time.sleep(0.1)
        pico_led.off()
        time.sleep(0.1)
        pico_led.on()
        time.sleep(0.1)
        pico_led.off()
        
        comm.Disconnect()
        global strike_flag
        strike_flag = False
        
    except Exception as err:
        print(err)
        stor.LogError(f'Main loop failed at unspecific point. Error message:\n{err}\n')

def Sleep():
    global sleep_flag
    sleep_flag = True 
    
    #There is a bug present in the current micropython port of the deep/lightsleep function to the rp2
    #in which the system fails to wake up after recieving an interrupt. You can see this via the code
    #on github which only accepts interrupts from the CYW43 wifi chip:
    #https://github.com/micropython/micropython/blob/2ad1d29747df1f35c638b32477684c6e141d0f81/ports/rp2/modmachine.c#L234
    
    #Based on the RP2350 datasheet, we can set the GPIO pin we desire to be able to wake the pico up
    #from dormancy via an interrupt. This is done by accessing the register at offset 0x2e0 from the user
    #bank address 0x40028000, and modifying the 4 bytes to enable our GPIO pin for dormancy interrupts.
    #Link to documentation:
    #https://pip-assets.raspberrypi.com/categories/1214-rp2350/documents/RP-008373-DS-2-rp2350-datasheet.pdf?disposition=inline#reg-io_bank0-DORMANT_WAKE_INTE0
    
    REG_DORMANT_WAKE_INT =  0x40028000 + 0x2e0 #User bank address plus offset
    DORMANT_WAKE_INT = 1 << 19 #bit 19 turns on GPIO20 as a dormant interrupt when rising
    
    machine.mem32[REG_DORMANT_WAKE_INT] = DORMANT_WAKE_INT #Fixes bug present in current micropython code
    
    print("Shutting down to sleep...")
    pico_led.off()
    
    time.sleep(0.2)
    pico_led.on()
    time.sleep(0.2)
    pico_led.off()
    
    acc.ResetIntrState()
    machine.lightsleep()
    
    sleep_flag = False #Occurs after sleep is over
      

#Initialization:
pico_led.off()
comm.Disconnect() #Kills any leftover connections
acc.AccTest() #Tests accelerometer can be communicated with
acc.ReadStateOn() #Turns on data reading
acc.IntrStateOn() #Turns on interrupts
acc.intr.irq(trigger=machine.Pin.IRQ_RISING,handler=ISR) #Create interrupt
strike_flag = False #Global variable that main loop checks to see if a strike occured
sleep_flag = False #Global variable used to check if currently attempting to sleep
time.sleep(5) #Gives time for user to exit main.py if attempting to access code


#Main loop:
while True:
    #Use following code to check for memory leaks each loop when debugging:
    '''
    gc.collect()
    f = open("log.txt","a+")
    f.write(str(gc.mem_free())+"\n")
    f.close()
    '''
    
    if strike_flag:
        Strike()
    
    Sleep()