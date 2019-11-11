#!/usr/bin/python3
# Written by David L Sprague Nov 2019
# This code reads data coming from a set of sensors via an external LoRa reciever
# implemented using an Adafruit Feather M0 connected to a USB port.  It averages
# the sensor data and uploads it to a Google spreadsheet.
# Code for uploading data to Google sheets is from 
# http://www.whatimade.today/log-sensor-data-straight-to-google-sheets-from-a-raspberry-pi-zero-all-the-python-code/                                                                                                                                

from __future__ import print_function
from googleapiclient.discovery import build
from httplib2 import Http
from oauth2client import file, client, tools
from oauth2client.service_account import ServiceAccountCredentials
import datetime
import random
import serial
import re

ser = serial.Serial('/dev/ttyACM0', 9600, timeout = 5) # 5 seconds

# My Spreadsheet ID ... See google documentation on how to derive this                
MY_SPREADSHEET_ID = '1QbVq9pnV5ikaNo_CnsuOAhTP0T22q0XOIAcirisPpts'

def update_sheet(sheetname, dtemp, dpres, stemp,spres, vbat , missedPackets):
    """update_sheet method:                                                           
       appends a row of a sheet in the spreadsheet with the                           
       the latest temperature, pressure and humidity sensor data                      
    """
    # authentication, authorization step                                              
    SCOPES = 'https://www.googleapis.com/auth/spreadsheets'
    creds = ServiceAccountCredentials.from_json_keyfile_name(
            'credentials.json', SCOPES)
    service = build('sheets', 'v4', http=creds.authorize(Http()))

    # Call the Sheets API, append the next row of sensor data                         
    # values is the array of rows we are updating, its a single row                   
    values = [ [ str(datetime.datetime.now()),
	            'DepthTemp,C', dtemp, 'DepthPress,hPa', dpres,
                'SurfaceTemp,C', stemp, 'SurfacePress,hPa', spres,
                'BattVoltage', vbat, 'MissedPackets', missedPackets] ]
    body = { 'values': values }

        # call the append API to perform the operation                                    
    result = service.spreadsheets().values().append(
                spreadsheetId=MY_SPREADSHEET_ID,
                range=sheetname + '!A1:N1',
                valueInputOption='USER_ENTERED',
                insertDataOption='INSERT_ROWS',
                body=body).execute()

def main():
    prevSeqNum = None
    while(1):
        depthTempList = []
        depthPressList = []
        surfaceTempList = []
        surfacePressList = []
        vBatList = []
        
        missedPackets = 0
        while len(depthTempList) < 60:
            message = ser.read_until().decode("ascii").strip()
            print("Received: " + message)
            print(':'.join(hex(ord(x))[2:] for x in message))
            if len(message) > 5:
                fields = list(filter(None, re.split("[, \-!?:]+", message)))
                print(fields)
                seqNum = int(fields[0][1:])
                if prevSeqNum != None:
                    diffSeq = seqNum - prevSeqNum
                    if diffSeq < 0:
                        diffSeq += 1000 # handle wraparound
                    missedPackets += diffSeq - 1
                else:
                    missedPackets = 0
              
                depthTempList.append(float(fields[1]))
                depthPressList.append(float(fields[2]))
                surfaceTempList.append(float(fields[3]))
                surfacePressList.append(float(fields[4]))
                vBatList.append(float(fields[5]))
                print('Depth Temperature: %f °C' % float(fields[1]))
                print ('Depth Pressure: %f hPa' % float(fields[2]))
                print('Sequence Number: %i' % seqNum)
                if prevSeqNum != None:
                     print('prev Seq Number: %i' % prevSeqNum)
                print('sample count: %i' % len(depthTempList))
                print('Missed Packet Count: %i' % missedPackets)

                prevSeqNum = seqNum
                
        depthTemp = sum(depthTempList)/len(depthTempList)
        depthPress = sum(depthPressList)/len(depthPressList)
        surfaceTemp = sum(surfaceTempList)/len(surfaceTempList)
        surfacePress = sum(surfacePressList)/len(surfacePressList)
        vBat = sum(vBatList)/len(vBatList)
        print("Uploading to spreadsheet")
        update_sheet("Data", depthTemp, depthPress,
                     surfaceTemp, surfacePress, vBat , missedPackets)


if __name__ == '__main__':
    main() 