#! /usr/bin/env python3
#
# Jay Herrmann
# ECE631 Spring 2016
#MQTT Clients for Publish and Subscript
#
import paho.mqtt.client as mqtt
import time
import types

class MQTTPusher(object):
	Messages = []
	def __init__(self,Host:str,Port:int,**Info):
		# The callback for when the client receives a CONNACK response from the server.
		def on_connect(client, userdata,flag, rc):
			pass
			#~ print("Pusher Connected with result code "+str(rc))
			# Subscribing in on_connect() means that if we lose the connection and
			# reconnect then subscriptions will be renewed.
		#	client.subscribe("$SYS/#")

		# The callback for when a PUBLISH message is received from the server.

		#~ def on_subscribe(client, userdata, mid, granted_qos, properties=None):
			#~ print("userData: %s"%str(userdata))

		#~ def on_log(client, userdata, level, buf):
			#~ print("userData: %s\nLevel: %s\nbuf: %s"%(userdata,level,buf))

		self.client = mqtt.Client()
		if Info.get('username') is not None:
			self.client.username_pw_set(username=Info.get("username"),password=Info.get("password"))

		print("Host: %s\tPort: %s"%(Host,Port))
		self.client.on_connect = on_connect
		#~ self.client.on_subscribe = on_subscribe
		#~ self.client.on_log = on_log
		self.client.connect(Host, Port, 60)
		self.client.loop_start()

	def __del__(self):
		self.client.loop_stop()

	def PushData(self,Topic,Message):
		self.client.publish(Topic, Message, qos=0, retain=False)


class MQTTListener(object):
	AMessages = []
	def __init__(self,Host:str,Port:int,ListenTopic:str=None,**Info):
		# The callback for when the client receives a CONNACK response from the server.
		def on_connect(client, userdata,flags, rc):
			pass
			#~ print("Listener Connected with result code "+str(rc))
			# Subscribing in on_connect() means that if we lose the connection and
			# reconnect then subscriptions will be renewed.
		#	client.subscribe("$SYS/#")

		# The callback for when a PUBLISH message is received from the server.
		def on_message(client, userdata, msg):
			msg.payload = msg.payload.decode('utf-8')
			self.AMessages.append(msg)

		self.TopicList = []
		self.client = mqtt.Client()
		if Info.get('username') is not None:
			self.client.username_pw_set(username=Info.get("username"),password=Info.get("password"))
		self.client.on_connect = on_connect
		self.client.on_message = on_message
		self.client.connect(Host, Port, 60)
		if ListenTopic is not None:
			self.TopicList +=[ListenTopic]
			#~ print("TopicList: %s"%self.TopicList)
			#~ print("Topic: %s"%ListenTopic)
			self.client.subscribe([(ListenTopic,0)])
		self.client.loop_start()

	def subscribe(self,ATopicList: list):
		TupleTopicList = []
		self.client.unsubscribe(self.TopicList)
		self.TopicList = []
		for Topic in ATopicList:
			try:
				TupleTopicList.append((Topic,0))
				self.TopicList.append(Topic)
			except:
				pass
		return self.client.subscribe(TupleTopicList)

	@property
	def Messages(self)-> list:
		return self.AMessages

	def __del__(self):
		self.client.loop_stop()

if __name__ == "__main__":
	MQTTListener = MQTTListener()
	try:
		while (True):
			time.sleep(0.01)
			if len(MQTTListener.Messages)>0:
				msg = MQTTListener.Messages.pop()
				print("Topic: "+msg.topic+" Message: "+str(msg.payload))
	finally:
		del MQTTListener
