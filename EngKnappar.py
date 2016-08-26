# Artemis Engineering Console by Stugo
# NOTE: This script requires the following Python modules:
#  pygame   - http://www.pygame.org/
#  pySerial - http://pyserial.sourceforge.net/
#  PIL - http://www.pythonware.com/products/pil/
# Win32 users may also need:
#  pywin32  - http://sourceforge.net/projects/pywin32/
#
# pip install pygame
# pip install pyserial
# pip install pypiwin32
# pip install pyscreenshot
#
# This script is based on a screen resolution of 1366x768 or 1280x800
#
# This script is Beerware
# ----------------------------------------------------------------------------
# "THE BEER-WARE LICENSE":
# As long as you retain this notice you can do whatever you want with this
# stuff. If we meet some day, and you think this stuff is worth it, you can
# buy me a beer in return /Stugo
# ----------------------------------------------------------------------------

import pygame
import time
import serial
import sys
import win32api, win32con
import pyscreenshot as ImageGrab
from win32api import GetSystemMetrics
from pygame.locals import *

# Buttons
# 0-7 is Coolant Up
# 8-15 is Coolant Down
# 16-25 is Presets 1-0
# 26 is Shift
# 27 is Spacebar
# 28 is Enter

# Some numbers dependent on resolution 1366x768
# Slider location:
# Y: 525 - 725
# X: 50 + 168*n
# Coolant:
# Y: 535, 745
# X: 94 + 168*n
# Heat:
# Y: 464, 503
# X: 84 + 168*n

# Some numbers dependent on resolution 1280x800
# Slider location:
# Y: 543 - 755
# X: 50 + 157*n
# Coolant:
# Y: 555, 780
# X: 94 + 157*n
# Heat:
# Y: 485, 523
# X: 84 + 157*n

def click(x,y):         # Make the cursor move and click
    print("click ", x, y)
    win32api.SetCursorPos((x,y))
    win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN,x,y,0,0)
    win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP,x,y,0,0)

def clickset(x,y):         # Make the cursor move and click, sliders
    print("clickset ", x, y)
    win32api.SetCursorPos((x,y))
    win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN,x,y,0,0)
    time.sleep(1)
    win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP,x,y,0,0)

def press(VK_CODE):     # Press a key on the keyboard
    win32api.keybd_event(VK_CODE, 0,0,0)
    pygame.time.wait(5)
    win32api.keybd_event(VK_CODE,0 ,win32con.KEYEVENTF_KEYUP ,0)

def coolantup(nbr):     # Make the coolant go up
    x = COOLX + COOLXSPACE * nbr
    y = COOLYUP
    click(x,y)

def coolantdown(nbr):   # Make the coolant go down
    x = COOLX + COOLXSPACE * nbr
    y = COOLYDOWN
    click(x,y)

def setslider(nbr, value):      # Move the slider
    x = SLIDEX + SLIDESPACE * nbr
    y = (SLIDEBOT - SLIDETOP) * ((value + 1) / 2) + SLIDETOP
    y = int(y)
    clickset(x,y)
    #print(x, y)

def getHeat():      # This bit is written by Davr. It gets the overheat-level /Modded by Stugo
    HeatHeight = HEATBOT - HEATTOP
    bbox=(HEATX,HEATTOP,HEATX+HEATSPACE*(8-1)+1,HEATBOT)
    # grab interesting part of screen
    g = ImageGrab.grab(bbox)
    px = g.load()
    for x in range(0, g.width):
        for y in range(0, g.height):
            screen.set_at((x,y), px[x,y])
    pygame.display.flip()
    heatList = [0]*8
    for i in range(0,8): # loop over all 8 heat indicators
        x = HEATSPACE*i # math to calculate the X position of the center of the heat indicator
        total = 0.0
        for y in range(HeatHeight-1, -1, -1): # loop over all the Y positions in the heat indicator
            c = px[x,y]
            #print(i,y, c, total, sum(c))
            if c == (57, 67, 69):
                print ("pixel has border color: ", i, (x, y), c, " Calibrate positions")
            if sum(px[x,y]) > 200: # if the sum of red, blue, and green channels is over 200
                total+=1
        heatList[i] = int(100 * total/HeatHeight) # calculate total heat for this heat indicator

    print("Heat: " + str(heatList))
    return heatList

def updateLEDs(heatList):
    c = str.encode(str(heatList).strip("[]") + " /n")
    ser.write(c)

if __name__ == '__main__':
    print("Artemis Engineering Console by Stugo, big mod by NiKiZe")
    print("You'll need to run Artemis in Full Screen Windowed")

    # Get the pixel resolution of main screen
    res = (GetSystemMetrics (0), GetSystemMetrics (1))
    width, height = res

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
    elif height == 1080:
        COOLYUP = 724
        COOLYDOWN = 1059
        SLIDETOP = 692
        SLIDEBOT = 1050
        HEATTOP = 652
        HEATBOT = 693

    if res in ((1024, 768),(1366, 768),(1280, 800),(1920, 1080)):
        print("%ix%i is selected" % res)
    else:
        print("Screen resolution %ix%i is not tested, using scaled values." % res)
    print(COOLX, COOLXSPACE, COOLYUP, COOLYDOWN, SLIDETOP, SLIDEBOT, SLIDEX, SLIDESPACE, HEATTOP, HEATBOT, HEATX, HEATSPACE)


    pygame.init()

    # Set the width and height of the screen [width,height]
    size = [HEATSPACE*(8-1) + 2, 200]
    screen = pygame.display.set_mode(size)
    pygame.display.set_caption("Artemis Engineering Console by Stugo")

    # Used to manage how fast the screen updates
    clock = pygame.time.Clock()

    # Initialize the joysticks
    pygame.joystick.init()

    # Count the joysticks:
    joystickCount = pygame.joystick.get_count()

    print("Joystick count: %i" % joystickCount)
    if joystickCount > 0:
        myJoy = pygame.joystick.Joystick(0)
        myJoy.init()
        print("Joystick name: %s" % myJoy.get_name())
        print("Joystick id: %i" % myJoy.get_id())
        print("Joystick axis: %i" % myJoy.get_numaxes())
        print("Number of buttons: %i" % myJoy.get_numbuttons())

    # open the serial port that your arduino
    # is connected to.
    #ser = serial.Serial("COM9", 9600)
    #time.sleep(2)

    ctr = 0

    #Loop until the user clicks the close button.
    done = False

    # -------- Main Program Loop -----------
    while not done:
        pygame.time.wait(1)

        ctr += 1
        if ctr > 200:
            heatList = getHeat()
            if 'ser' in locals():
                updateLEDs(heatList)
            ctr = 0

        for event in pygame.event.get():
            if event.type == QUIT:
                pygame.quit()
                sys.exit()


            elif event.type == pygame.JOYBUTTONDOWN:
                if 0 <= event.button and event.button <=7:
                    coolantup(event.button)

                elif 8 <= event.button and event.button <= 15:
                    coolantdown(event.button - 8)

                elif 16 <= event.button and event.button <= 24:
                    num = event.button - 15
                    press (0x30 + num)
                    print(num)
                elif event.button == 25:
                    press (0x30)
                    print("0")
                elif event.button == 26:
                    press (0xA0)
                    print("Shift")
                elif event.button == 27:
                    press (0x20)
                    print("Spacebar")
                elif event.button == 28:
                    press (0x0D)
                    print("Enter")

            elif event.type == pygame.JOYAXISMOTION:
                if event.value < 1.0:
                    setslider(event.axis, event.value)


    # Close the window and quit.
    # If you forget this line, the program will 'hang'
    # on exit if running from IDLE.
    pygame.quit ()
