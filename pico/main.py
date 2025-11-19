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




pico_led.off()
acc.AccTest() #Tests accelerometer can be communicated with
acc.ReadState(1) #Turns on data reading
acc.IntrState(1) #Turns on interrupts
acc.ResetIntrState() #Ensures interrupt condition is reset

def Strike(pin):
    pico_led.on()
    
    a = acc.FastStream() #Gathers data
    
    name = stor.CreateBin(a) 
    del a

    comm.SendData(name) #Sends data to Github repository
    del name
    
    #Checks to see if other binary files weren't sent
    files = os.listdir()
    temp = []
    for i in range(0,len(files)):
        if files[i][-4:] == ".bin":
            temp.append(files[i])
    files = temp
    del temp
    
    #Sends data to Github repository
    for i in range(0,len(files)):
        name = files[i]
        #print(f'mem free before calling function {i+1} '+str(gc.mem_free()))
        state = False
        if name.index(".") - name.index("_",2) < 10:
            state = True
        comm.SendData(name,gettime=state)
    del files
    
    comm.Disconnect()
    acc.ResetIntrState()
   
    '''
    gc.collect()
    f = open("log.txt","a+")
    f.write(str(gc.mem_free())+"\n")
    f.close()
    '''
    
    stor.LogError(4,len(files)+1)
    
    pico_led.off()
    time.sleep(0.1)
    pico_led.on()
    time.sleep(0.1)
    pico_led.off()

    Sleep()        

def test2(pin):
    pico_led.on()
    time.sleep(0.1)
    pico_led.off()
    acc.ResetIntrState()
    Sleep()


acc.intr.irq(trigger=machine.Pin.IRQ_RISING,handler=Strike)

def Sleep():
    while True:
        machine.deepsleep(1000000)

gc.collect()

def Sleep():
    pico_led.off()
    while True:
        machine.deepsleep()
        stor.LogError(0,"You should not see this message!")
        
