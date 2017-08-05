# Artemis Engineering Console by Stugo
# NOTE: This script requires the following Python modules:
#  pygame   - http://www.pygame.org/
#  pySerial - http://pyserial.sourceforge.net/
#
# The below should get you going
# pip install pygame
# pip install pyserial
# pip install pyscreenshot
# pip install image
#
# This script is Beerware
# ----------------------------------------------------------------------------
# "THE BEER-WARE LICENSE":
# As long as you retain this notice you can do whatever you want with this
# stuff. If we meet some day, and you think this stuff is worth it, you can
# buy me a beer in return /Stugo
#          Mod By NiKiZe
# ----------------------------------------------------------------------------

import pygame
import serial
import pyscreenshot as ImageGrab
import ctypes
from pygame.locals import *
user32 = ctypes.windll.user32

def getHeat():      # This bit is written by Davr. It gets the overheat-level /Modded by Stugo
    bbox=(HEATX,HEATTOP,HEATX+HEATSPACE*(8-1)+1,HEATBOT)
    # grab interesting part of screen
    g = ImageGrab.grab(bbox)
    px = g.load()
    if DEBUG:
        for x in range(0, g.width):
            for y in range(0, g.height):
                screen.set_at((x,y), px[x,y])
        pygame.display.flip()
    heatList = [0]*8
    for i in range(0,8): # loop over all 8 heat indicators
        # reset HeatHeight just in case it was calibrated
        HeatHeight = HEATBOT - HEATTOP
        x = HEATSPACE*i # math to calculate the X position of the center of the heat indicator
        total = 0.0
        for y in range(HeatHeight-1, -1, -1): # loop over all the Y positions in the heat indicator
            c = px[x,y]
            if c in ((0,0,0),(0xff, 0xff, 0xff)):
                continue
            hc = "%02x%02x%02x" % c
            if DEBUG:
                print("%ix%02i %s %02.0f rgbsum:%i" % (i,y, hc, total, sum(c)))
            if c in ((0x39,0x43,0x45), (0x57,0x80,0x89), (0x50,0x70,0x78), (0x4e,0x6f,0x77)):
                print("pixel has border color: ", i, (HEATX + x, HEATTOP + y), hc, " Calibrate positions")
                HeatHeight-=1
            elif sum(px[x,y]) > 200: # if the sum of red, blue, and green channels is over 200
                total+=1
        heatList[i] = 0 if total <= 0 else int(5 * total/(HeatHeight+1)) + 1 # calculate total heat for this heat indicator

    if DEBUG:
        print("Heat: " + str(heatList))
    return heatList

def updateLEDs(heatList):
    c = str.encode(str(heatList).strip("[]") + "\n")
    print("Sending to arduino: %s" % c)
    ser.write(c)

if __name__ == '__main__':
    print("Artemis Engineering Console by Stugo, big mod by NiKiZe")
    print("You'll need to run Artemis in Full Screen Windowed")

    # Get the pixel resolution of main screen
    res = (user32.GetSystemMetrics(0), user32.GetSystemMetrics(1))
    width, height = res

    DEBUG = False
    #Sets the coordinates
    COOLX = 94          # X coordinate to the center of the first blue coolant arrow
    SLIDEX = 50         # X coordinate to the center of the first energy-slider
    HEATX = 84          # X coordinate to the center of the first beatbar
    COOLXSPACE = int((width - 24) / 8)    # Spacing in X between coolant systems
    if width == 1024:
        COOLXSPACE = 125
    elif width == 1366:
        COOLXSPACE = 168
    elif width == 1280:
        COOLXSPACE = 157
    elif width == 1600:
        COOLXSPACE = 197
    elif width == 1920:
        COOLXSPACE = 237
    SLIDESPACE = COOLXSPACE    # Spacing in X between energy-sliders
    HEATSPACE = COOLXSPACE     # Spacing in X between heatbars

    COOLYUP = int(724.0 / 1080 * height)      # Y coordinate to the center of a blue coolant up-arrow
    COOLYDOWN = int(1059.0 / 1080 * height)     # Y coordinate to the center of a blue coolant down-arrow
    SLIDETOP = int(692.0 / 1080 * height)    # Y coordinate to the top of a energy-slider
    SLIDEBOT = int(1050.0 / 1080 * height)    # Y coordinate to the bottom of a energy-slider
    HEATTOP = int(652.0 / 1080 * height)       # Y coordinate to the top of a heatbar
    HEATBOT = int(693.0 / 1080 * height)       # Y coordinate to the bottom of a heatbar

    if height == 768:
        COOLYUP = 535       # Y coordinate to the center of a blue coolant up-arrow
        COOLYDOWN = 745     # Y coordinate to the center of a blue coolant down-arrow
        SLIDETOP = 502    # Y coordinate to the top of a energy-slider
        SLIDEBOT = 748    # Y coordinate to the bottom of a energy-slider
        HEATTOP = 464       # Y coordinate to the top of a heatbar
        HEATBOT = 503       # Y coordinate to the bottom of a heatbar
    elif height == 800:
        COOLYUP = 555
        COOLYDOWN = 780
        SLIDETOP = 543
        SLIDEBOT = 755
        HEATTOP = 485
        HEATBOT = 523
    elif height == 900:
        COOLYUP = 648
        COOLYDOWN = 875
        SLIDETOP = 614
        SLIDEBOT = 877
        HEATTOP = 574
        HEATBOT = 616
    elif height == 1080:
        COOLYUP = 724
        COOLYDOWN = 1059
        SLIDETOP = 692
        SLIDEBOT = 1050
        HEATTOP = 652
        HEATBOT = 693

    if res in ((1024, 768),(1366, 768),(1280, 800),(1600, 900),(1920, 1080)):
        print("%ix%i is selected" % res)
    else:
        DEBUG = True
        print("Screen resolution %ix%i is not tested, using scaled values." % res)
    print(COOLX, COOLXSPACE, COOLYUP, COOLYDOWN, SLIDETOP, SLIDEBOT, SLIDEX, SLIDESPACE, HEATTOP, HEATBOT, HEATX, HEATSPACE)


    pygame.init()

    # Set the width and height of the screen [width,height]
    if DEBUG:
        size = [HEATSPACE*(8-1) + 2, 200]
        screen = pygame.display.set_mode(size)
        pygame.display.set_caption("Artemis Engineering Console by Stugo")

    oldHeatList = None
    # open the serial port that your arduino
    # is connected to.
    ser = serial.Serial("COM5", 115200)

    ctr = 0

    #Loop until the user clicks the close button.
    done = False

    # -------- Main Program Loop -----------
    while not done:
        pygame.time.wait(1)

        serbuf = b''
        while ser.in_waiting > 0:
            serbuf += ser.read()
        if len(serbuf) > 0:
            print(serbuf.decode())

        ctr += 1
        if ctr > 100 if DEBUG else 10:
            heatList = getHeat()
            # if old value was 4-5 (HOT!) and new is 0 then it is probably due to flashing so ignore
            if oldHeatList and not heatList == oldHeatList and len(heatList) == len(oldHeatList):
                for i in range(0, len(heatList)):
                    if oldHeatList[i] >= 4 and heatList[i] == 0:
                        heatList[i] = oldHeatList[i]
            if 'ser' in locals() and not heatList == oldHeatList:
                updateLEDs(heatList)
            ctr = 0
            oldHeatList = heatList

        for event in pygame.event.get():
            if event.type == QUIT:
                done = True

    # Close the window and quit.
    # If you forget this line, the program will 'hang'
    # on exit if running from IDLE.
    pygame.quit()
