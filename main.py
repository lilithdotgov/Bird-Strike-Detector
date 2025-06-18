####################################### Main Code Loop ########################################
import communication as comm
import accelerometer as acc
import storage as stor
import analysis as anal
import machine
from picozero import pico_led
import time
import gc

pico_led.off()
acc.AccTest() #Tests accelerometer can be communicated with
acc.ReadState(1) #Turns on data reading
acc.IntrState(1) #Turns on interrupts
acc.ResetIntrState() #Ensures interrupt condition is reset

def test(pin):
    pico_led.on()
    
    print("mem before anything "+str(gc.mem_free()))
    
    a = acc.FastStream()
    
    print("mem after FastStream "+str(gc.mem_free()))
    
    b = anal.StripData(a)
    del a
    
    print("mem after StripData "+str(gc.mem_free()))
    
    name = stor.CreateBin(b)
    del b
    
    print("mem after CreateBin "+str(gc.mem_free()))
    
    comm.SendData(name)
    del name
    
    print("mem after SendData "+str(gc.mem_free())+"\n")
    
    comm.Disconnect()
    acc.ResetIntrState()
    
    gc.collect()
    f = open("log.txt","a+")
    f.write(str(gc.mem_free())+"\n")
    f.close()
    
    pico_led.off()
    time.sleep(0.1)
    pico_led.on()
    time.sleep(0.1)
    pico_led.off()
    
    #Sleep()
    

def test2(pin):
    pico_led.on()
    time.sleep(0.1)
    pico_led.off()
    acc.ResetIntrState()
    Sleep()


acc.intr.irq(trigger=machine.Pin.IRQ_RISING,handler=test)

def Sleep():
    while True:
        machine.deepsleep(1000000)
        
gc.collect()
print("mem before even the loop "+str(gc.mem_free()))