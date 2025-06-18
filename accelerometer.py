####################################### Acceleromter Code #####################################
import machine
import time
import ustruct
import config
import gc
import storage as stor

#Documentation: https://www.analog.com/media/en/technical-documentation/data-sheets/adxl343.pdf
#Notes:
#To read data first bit W should be 1, to write it should be 0
#To read/write multiple bytes in one call second bit MB is set to 1, else 0 for a single byte
#Chip Select (CS) needs to be set to 0 whenever doing read/write, and set to 1 afterwards

REG_DEVID = 0x00 #Used to test that we are reading from correct spot. Check doc pg. 23
DEVID = 0xE5 #Should recieve this default value when reading the Device ID Register (REG_DEVID)
REG_POWER_CTL = 0x2D #Power Control Register, used to enable/disable data output
REG_RATE = 0x2C #Rate of output data
RATE = 0b1101 #0b1101 is 800RATE Data Output Rate, see doc pg. 12 for more ranges
REG_DATAX0 = 0x32 #0x32 to 0x37 contain the data for each axis, each is 2 bytes. Order is x, y, and z
REG_INT_ENABLE = 0x2E #Interrupt Enable Register
INT_ENABLE = 0x10 #Enables Activity mode
REG_ACT_CTL = 0x27 #Enables which axis to be monitored for the interrupt modes. Plus an unused bonus feature, see doc pg. 23
ACT_CTL = 0x10 #Sets just the z axis on for now. TODO: Change to least noisy axis in production
REG_INT_MAP = 0x2F #Chooses which pin(s) to use for interrupts
INT_MAP = 0xEF #Sets activity mode interrupt to pin INT1
REG_THRESH_ACT = 0x24 #Sets threshold for interrupt to occur. Single unsigned byte. threshold = 62.5mg * THRESH_ACT.
THRESH_ACT = 0x16 #0x20 = 32, 32 * 62.5mg = 2g 
REG_FIFO_MODE = 0x38 #Register controlling FIFO modes
FIFO_MODE = 0x80 #Sets FIFO mode to stream
REG_INT_SOURCE = 0x30 #Read-only register, shows which interrupts were activated. Reading this resets the interrupt states 
G = 9.80665     

##################### TODO: CHANGE PIN INTERRUPT TO DEFAULT TO LOW CURRENT AND HAVE HIGH CURRENT BE INTERRUPT EVENT

# Assign pins and create SPI instance
cs = machine.Pin(17, machine.Pin.OUT)
intr = machine.Pin(20, machine.Pin.IN) #interrupt
spi = machine.SPI(0,
                  baudrate=2000000, #see doc pg. 13 for info on appropriate ranges 
                  polarity=1, #requirement, see doc pg. 
                  phase=1, #requirement, see doc pg.
                  bits=8,
                  firstbit=machine.SPI.MSB,
                  sck=machine.Pin(18), 
                  mosi=machine.Pin(19), 
                  miso=machine.Pin(16))

#CS pin needs to be high voltage at start, see doc pg.
cs.value(1)
#SCK also needs to idle at high, can be done via a useless read
#This can be found below the read function

def reg_write(spi, cs, reg, data): #upgrade to support multiple byte writes
    msg = bytearray() #creates bytearray to store message. Represents hex as valid ASCII when printed
    msg.append(reg & ~(0x03 << 6)) #inserts register of the message. set first 2 bits W and MB to 0, see doc pg. 14
    msg.append(data) #inserts the data of the message
    
    cs.value(0) #Sets chip select to low voltage to write. See doc pg. 13 & 14
    spi.write(msg)
    cs.value(1) #Sets it back to high voltage
    
def reg_read(spi, cs, reg, nbytes=1):
    
    #Checks if one or more bytes are being read
    if nbytes < 1:
        return "Invalid range"
    elif nbytes == 1:
        mb = 0
    else:
        mb = 1
    
    msg = bytearray()
    msg.append(0x80 | (mb << 6) | reg) #"0x80 |" sets W to 1, "(mb << 6) |" sets MB to 1 or 0
    
    # Send out SPI message and read
    cs.value(0) #Sets chip select to low voltage to write. See doc pg. 13 & 14
    spi.write(msg) #Only contains register, no data. W is 1 so read instruction is initiated
    data = spi.read(nbytes) 
    cs.value(1) #Sets it back to high voltage
    
    return data

#Ensures SCK is high
reg_read(spi, cs, REG_DEVID)


# Read device ID to make sure that we can communicate with the ADXL343
def AccTest():
    data = reg_read(spi, cs, REG_DEVID)
    if (data != bytearray((DEVID,))):
        stor.ErrorLog(1)
        machine.soft_reset()


def ReadState(state):
    if state == 0:
        PowerControl = reg_read(spi, cs, REG_POWER_CTL) #Obtain current Power Control setting
        PowerControl = int.from_bytes(PowerControl, "big") & ~(0x01 << 3) #Change Power Control Measure bit to 0 to end data collecition, see pg. 
        reg_write(spi, cs, REG_POWER_CTL, PowerControl) #Write change
 
    
    elif state == 1:
        reg_write(spi, cs, REG_RATE, RATE) #Sets Data Output Rate 
        PowerControl = reg_read(spi, cs, REG_POWER_CTL) #Obtain current Power Control setting
        PowerControl = int.from_bytes(PowerControl, "big") | (0x01 << 3) #Change Power Control Measure bit to 1 to begin data collecition, see pg. 
        reg_write(spi, cs, REG_POWER_CTL, PowerControl) #Write change
 
    else:
        print("Invalid Input. Please choose one of the following power modes:\n Off: 0\n On: 1")
        
def IntrState(state): #Add functionality to better undo this later!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    if state == 0:
        reg_write(spi, cs, REG_INT_ENABLE, 0x00) #A value of 0x00 disables all interrupts
    elif state == 1:
        reg_write(spi, cs, REG_INT_MAP, INT_MAP) #Sets INT pin
        reg_write(spi, cs, REG_THRESH_ACT, THRESH_ACT) #Sets threshold
        reg_write(spi, cs, REG_FIFO_MODE, FIFO_MODE) #Sets FIFO mode
        reg_write(spi, cs, REG_ACT_CTL, ACT_CTL) #Sets axes for monitoring

        reg_write(spi, cs, REG_INT_ENABLE, INT_ENABLE) #Enables interrupts. Must go last, see doc pg. 18
    else:
        print("Invalid Input. Please choose one of the following bin modes:\n Off: 0\n On: 1")
    
def ResetIntrState():
    reg_read(spi, cs, REG_INT_SOURCE)
    
def Stream(Samples=1): #Have it check to see if everything is initialized, instead of doing some prior and some in the function!!
    ReadState(1)
    data = []
    prev = [0,0,0]
    i = 0
    while i < Samples:
        cur = reg_read(spi, cs, REG_DATAX0, 6) #Get data
        #Format data
        curXYZ = ustruct.unpack_from("<3h", cur)
        
        if curXYZ != prev: #Check if reading same sample
            prev = curXYZ
            
            #Apply calibration
            caliXYZ = (config.ScaleX * (curXYZ[0] - config.OffsetX),
                       config.ScaleY * (curXYZ[1] - config.OffsetY),
                       config.ScaleZ * (curXYZ[2] - config.OffsetZ))
            data.append(caliXYZ)
            i = i + 1
            
        prev = curXYZ
    
    return data
  
def FastStream(): #Optimized for speed, data needs further handling, used in main loop
    buffer_size = const(1024*3)
    bps = const(6) #bytes per sample
    
    data = bytearray(buffer_size*bps)
    for i in range(0,buffer_size):
        data[bps*i:bps*i+bps] = reg_read(spi, cs, REG_DATAX0, bps)
        
        if i % 64 == 0: #might not be needed, bytearray seems to be very cleverly done when compiled!
            gc.collect()
    
    gc.collect()
    return data
    
def Calibrate():
    axis = int(input("Choose an axis for calibration: \n X = 0 \n Y = 1 \n Z = 2 \n"))
    input("Please stabilize the device to have the + side face UPWARDS. Press ENTER to continue") #work on wording!!!
    UpperBound = Stream(100)
    gc.collect()
    input("Please stabilize the device to have the - side face DOWNWARDS. Press ENTER to continue")
    LowerBound = Stream(100)
    gc.collect()
    AvgT = 0
    AvgB = 0
    N = len(UpperBound)
    for i in range(0,N):
        AvgT = AvgT + (UpperBound[i][axis]/N) #Don't know if this is dumb or not, but div before add to increase float percision?
        AvgB = AvgB + (LowerBound[i][axis]/N)
            
    Offset = (AvgB + AvgT)/2
    Scale = G/(AvgT - Offset)
    
    verboseAxis = ("X","Y","Z")
    config.SaveConfig("Offset"+verboseAxis[axis],Offset)
    config.SaveConfig("Scale"+verboseAxis[axis],Scale)
    print("Calibration completed! Resetting system in 3 seconds")
    time.sleep(3)
    machine.soft_reset()

