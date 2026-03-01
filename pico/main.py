####################################### Main Code Loop ########################################
from machine import mem32, lightsleep, reset, Pin
import accelerometer as acc
import storage as stor
import communication as comm
from time import sleep
from os import listdir

#Functions:
def ISR(pin): #Handles the interrupt
    if sleep_flag:
        acc.ResetIntrState() #Reset interrupt state to allow for future interrupts
        
def Strike(): #Main function that collects, stores, and sends the strike data
    try:
        data = acc.FastStream() #Gathers data
        
        name = stor.CreateBin(data) #returns name of the newly made file
        del data
        
        name = comm.SendData(name) #Sends data to Github repository, returns new name of file
        
        nfiles = 0 #Number of files, used to keep track of how many were successfully sent
        if name not in listdir():
            nfiles = nfiles + 1
        del name
        
        #Checks to see if there exist past strikes that weren't sent
        files = listdir()
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
            if name in listdir(): #Checks whether we actually sent the file, if still in os then decriment nfiles
                nfiles = nfiles - 1
        del files

        stor.Log(f'{nfiles} Strike(s) successfully logged and sent!\n')
        del nfiles
        
        #For some reason this doesn't even seem to do anything?
        #I'll keep it on for now though, just in case...
        comm.Disconnect()
        
    except Exception as err:
        print(err)
        stor.Log(f'Main loop failed at unspecific point. Error message:\n{err}\n')
        reset()

def Sleep():
    global sleep_flag
    sleep_flag = True 
    
    #There is a bug present in the current micropython port of the deep/lightsleep function to the rp2
    #in which the system fails to wake up after recieving an interrupt. You can see this via the code
    #on github which only accepts interrupts from the CYW43 wifi chip:
    #https://github.com/micropython/micropython/blob/2ad1d29747df1f35c638b32477684c6e141d0f81/ports/rp2/modmachine.c#L234
    
    #Based on the RP2350 datasheet, we can set the GPIO pin we desire to be able to wake the pico up
    #from DORMANT via an interrupt. This is done by accessing the register at offset 0x2e0 from the user
    #bank address 0x40028000, and modifying the 4 bytes to enable our GPIO pin for dormancy interrupts.
    #Link to documentation:
    #https://pip-assets.raspberrypi.com/categories/1214-rp2350/documents/RP-008373-DS-2-rp2350-datasheet.pdf?disposition=inline#reg-io_bank0-DORMANT_WAKE_INTE0
    
    #WORK ON THIS!!!
    REG_DORMANT_WAKE_INT =  0x40028000 + 0x2e0 #User bank address plus offset
    DORMANT_WAKE_INT = 0b1 << 19 #bit 19 turns on GPIO20 as a dormant interrupt when rising
    
    mem32[REG_DORMANT_WAKE_INT] = DORMANT_WAKE_INT #Fixes bug present in current micropython code
    
    #We want to put RAM into sleep mode (data saved but can't be accessed) to reduce power consumption further.
    #To do this first we need to set up our interrupt as a wake-up condition, or else when going out of
    #DORMANT mode the SRAM banks will still be asleep and can't be accessed
    REG_POWMAN_BASE = 0x40100000 #Registers for power management
    POWMAN_PASSWORD = 0x5AFE << 16 #Offsets from 0x0 to 0xAC require this password be written to the top 16 bits 
    
    REG_PWRUP0_OFFSET = 0x8c #Register for configuring a wake-up event
    PWRUP0 = 0b1111010100 #Sets a wake-up condition on a rising edge on GPIO 20
    
    mem32[REG_POWMAN_BASE + REG_PWRUP0_OFFSET] = PWRUP0 | POWMAN_PASSWORD #Writes the change
    
    REG_STATE_OFFSET = 0x38 #Controls the power state of the 4 power domains: SWCORE, XIP, SRAM0, and SRAM1
    STATE = 0b1111 << 4 #Turns off SRAM0 and SRAM1 and XIP and SWCORE
    
    mem32[REG_POWMAN_BASE + REG_STATE_OFFSET] = STATE | POWMAN_PASSWORD #Writes the change
    
    #Finally reset the interrupt and go to dormant mode
    acc.ResetIntrState()
    lightsleep()
    reset() #Might not be called in the first place
      

#Initialization:
sleep_flag = False #Global variable used to check if currently attempting to sleep

acc.ReadStateOn() #Turns on data reading
acc.IntrStateOn() #Turns on interrupts
acc.intr.irq(trigger=Pin.IRQ_RISING,handler=ISR) #Create interrupt

if Pin(20).value(): #Checks to see if a strike occured
    Strike()
    acc.ResetIntrState()
    reset()
else: #This is first time device is being run or right after a strike
    sleep(5) #Gives time for user to exit main.py if attempting to access code
stor.Log("Sleep!")
Sleep()
