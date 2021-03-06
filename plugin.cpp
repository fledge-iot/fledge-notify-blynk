/*
 * Fledge Blynk notification delivery plugin.
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Massimiliano Pinto
 */
#include <plugin_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <logger.h>
#include <plugin_exception.h>
#include <iostream>
#include <config_category.h>
#include <sstream>
#include <unistd.h>
#include "version.h"
#include "blynk.h"

using namespace std;
using namespace rapidjson;

#define PLUGIN_NAME "Blynk"

/**
 * Plugin specific default configuration
 */
#define PLUGIN_DEFAULT_CONFIG \
			"\"token\": { " \
				"\"description\": \"The Blynk REST API token\", " \
				"\"type\": \"string\", " \
				"\"default\": \"\", " \
				"\"order\": \"1\", \"displayName\" : \"REST API token\"}, " \
			"\"pin\": { " \
				"\"description\": \"The Blynk Pin for device or Virtual Pin\", " \
				"\"type\": \"string\", " \
				"\"default\": \"\", " \
				"\"displayName\" : \"Device Pin/Vpin\", " \
				"\"order\": \"2\" }, " \
			"\"api_url\": { " \
				"\"displayName\": \"REST API url prefix\", " \
				"\"type\": \"string\", " \
				"\"default\": \"http://blynk-cloud.com\", " \
				"\"order\": \"3\", " \
				"\"description\" : \"Blynk REST API url prefix.\" }," \
			 "\"enable\": {\"description\": \"A switch that can be used to enable or disable execution of " \
					 "the Blynk notification plugin.\", " \
				"\"type\": \"boolean\", " \
				"\"default\": \"false\", " \
				"\"displayName\" : \"Enabled\" }"

#define BLYNK_PLUGIN_DESC "\"plugin\": {\"description\": \"Blynk notification delivery C plugin\", \"type\": \"string\", \"default\": \"" PLUGIN_NAME "\", \"readonly\": \"true\"}"

#define PLUGIN_DEFAULT_CONFIG_INFO "{" BLYNK_PLUGIN_DESC ", " PLUGIN_DEFAULT_CONFIG "}"

/**
 * The Blynk plugin interface
 */
extern "C" {

/**
 * The C API plugin information structure
 */
static PLUGIN_INFORMATION info = {
	PLUGIN_NAME,				// Name
	VERSION,				// Version
	0,					// Flags
	PLUGIN_TYPE_NOTIFICATION_DELIVERY,	// Type
	"1.0.0",				// Interface version
	PLUGIN_DEFAULT_CONFIG_INFO		// Configuration
};

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info()
{
	return &info;
}

/**
 * Initialise the plugin with configuration.
 *
 * This funcion is called to get the plugin handle.
 */
PLUGIN_HANDLE plugin_init(ConfigCategory* configData)
{
	Blynk* handler = new Blynk(configData);
	return (PLUGIN_HANDLE)handler;
}

/**
 * Shutdown the plugin
 */
void plugin_shutdown(PLUGIN_HANDLE handle)
{
	Blynk* data = (Blynk *)handle;
	// Delete plugin handle
	delete data;
}

/**
 * Deliver received notification data
 *
 * @param handle		The plugin handle returned from plugin_init
 * @param deliveryName		The delivery category name
 * @param notificationName	The notification name
 * @param triggerReason		The trigger reason for notification
 * @param customMessage		The message from notification
 */
bool plugin_deliver(PLUGIN_HANDLE handle,
		    const std::string& deliveryName,
		    const std::string& notificationName,
		    const std::string& triggerReason,
		    const std::string& customMessage)
{

	Logger::getLogger()->debug("Blynk notification plugin_deliver(): "
				   "deliveryName=%s, notificationName=%s, "
				   "triggerReason=%s, message=%s",
				   deliveryName.c_str(),
				   notificationName.c_str(),
				   triggerReason.c_str(),
				   customMessage.c_str());

	Blynk* blynk = (Blynk *)handle;

	// Send Blynk message
	return blynk->notify(notificationName,
				triggerReason,
				customMessage);
}

/**
 * Reconfigure the plugin
 */
void plugin_reconfigure(PLUGIN_HANDLE *handle,
			string& newConfig)
{
	Logger::getLogger()->debug("Blynk notification plugin: plugin_reconfigure()");
	Blynk* blynk = (Blynk *)handle;
	
	blynk->reconfigure(newConfig);

	return;
}

// End of extern "C"
};
