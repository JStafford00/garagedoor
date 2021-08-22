#! /usr/bin/env python3
# ECE 631 Spring 2021
# Author: 
# Demo of NFC UID get.
#
"""
This example shows connecting to the PN532 with I2C (requires clock
stretching support), SPI, or UART. SPI is best, it uses the most pins but
is the most reliable and universally supported.
After initialization, try waving various 13.56MHz RFID cards over it!
"""

import time
import datetime
import binascii
import RPi.GPIO as GPIO
import pn532
import json
import MQTTClients

MQTTPub = MQTTClients.MQTTPusher('127.0.0.1', 1883)
curTime = time.time()
actionTime = time.time()

if __name__ == '__main__':
	try:
		PN532 = pn532.PN532_SPI(debug=False, reset=22, cs=8)

		ic, ver, rev, support = PN532.get_firmware_version()
		print('Found PN532 with firmware version: {0}.{1}'.format(ver, rev))

		# Configure PN532 to communicate with MiFare cards
		PN532.SAM_configuration()

		print('Waiting for RFID/NFC card...')
		while True: #Main Loop
			try:
				# Check if a card is available to read with 100ms timeout
				uid = PN532.read_passive_target(timeout=0.1)
				elapTime = time.time()

				if elapTime >= curTime + 1:
					curTime = time.time()
					if uid is None:
						pass
					elif curTime >= actionTime + 5:
						topic2 = "/ece631/Final/Input"
						payload2 = {"UID":binascii.hexlify(uid).decode('utf-8')}
						MQTTPub.PushData(topic2, json.dumps(payload2))
						actionTime = time.time()
					else:
						pass

				# No card is available.
				if uid is None:
					continue

				print('Found card with UID: %s'%(binascii.hexlify(uid).decode('utf-8')))

			except Exception as e:
				print("Loop Exception: %s"%e)
	except Exception as e:
		print("Main Exception: %s"%e)
	finally:
		GPIO.cleanup()
