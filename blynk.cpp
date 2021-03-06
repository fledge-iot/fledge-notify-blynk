/*
 * Fledge Blynk class
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
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <sstream>
#include <unistd.h>
#include <client_https.hpp>
#include <client_http.hpp>
#include "blynk.h"

using namespace std;
using namespace rapidjson;
using HttpsClient = SimpleWeb::Client<SimpleWeb::HTTPS>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

/**
 * Construct for Blynk class
 *
 * @param category	The configuration of the plugin
 */
Blynk::Blynk(ConfigCategory *category)
{
	// Configuration set is protected by a lock
	lock_guard<mutex> guard(m_configMutex);
	m_token = category->getValue("token");
	m_pin = category->getValue("pin");
	m_url = category->getValue("api_url");

	if (m_token.empty() ||
	    m_pin.empty() ||
	    m_url.empty())
	{
		m_enable = false;
	}
	else
	{
        	m_enable = category->getValue("enable").compare("true") == 0 ||
			   category->getValue("enable").compare("True") == 0;
	}
}

/**
 * The destructure for the Blynk class
 */
Blynk::~Blynk()
{
}

/**
 * Send a notification via the Blynk REST API
 *
 * @param notificationName 	The name of this notification
 * @param triggerReason		Why the notification is being sent
 * @param message		The message to send
 */
bool Blynk::notify(const string& notificationName,
		      const string& triggerReason,
		      const string& customMessage)
{
	// Configuration fetch is protected by a mutex
	m_configMutex.lock();
	if (!m_enable)
	{
		// Release lock	
		m_configMutex.unlock();
		return false;
	}
		
	/**
	* Extract host, port, path from URL
	*/
	size_t findProtocol = m_url.find_first_of(":"); 
	string protocol = m_url.substr(0,findProtocol);

	string tmpUrl = m_url.substr(findProtocol + 3);
	size_t findPort = tmpUrl.find_first_of(":");
	size_t findPath = tmpUrl.find_first_of("/");
	if (findPath == string::npos)
	{
		findPath = tmpUrl.length();
	}
	string port;
	string hostAndPort;
	if (findPort == string::npos)
	{
		hostAndPort = tmpUrl.substr(0, findPath);
	}
	else
	{
		string hostName = tmpUrl.substr(0, findPort);
		port = tmpUrl.substr(findPort + 1 , findPath - findPort -1);
		hostAndPort = hostName + ":" + port;
	}

	string path = tmpUrl.substr(findPath);
	if (path.empty())
	{
		path = "/";
	}

	// Prepare GET request appending TOKEN and sendmessage?.... to URL prefix
	string finalPath = path + m_token + \
			   "/update/" + m_pin + "?value=";
        
	// Send message
	// Release lock	
	m_configMutex.unlock();

	// Parse JSON trigger reason string
	Document doc;
	doc.Parse(triggerReason.c_str());
	if (doc.HasParseError())
	{
		Logger::getLogger()->error("Blynk delivery: failure parsing "
					   "JSON trigger reason '%s'",
					   triggerReason.c_str());
		return false;
	}

	string reason = doc["reason"].GetString();

	// We assume the configured Virtual Pin
	// takes value 1 for switching on the alerting device
	// and 0 for switching off
	if (reason.compare("triggered") == 0)
	{
		finalPath += "1";
	}
	if (reason.compare("cleared") == 0)
	{
		finalPath += "0";
	}

	finalPath += "&reason="+ reason + "&notification=" + notificationName;

	Logger::getLogger()->debug("Delivering Blynk message, URL='%s'",
				   (hostAndPort + finalPath).c_str());

	// Send message
	try
	{
		int httpCode;
		string retCode;
		// Send HTTS GET
		if (protocol.compare("https") == 0)
		{
			HttpsClient sClient(hostAndPort);
			sClient.config.timeout = (time_t)10;
			sClient.config.timeout_connect = (time_t)10;
			auto res = sClient.request("GET", finalPath);
			retCode = res->status_code;
			httpCode = atoi(retCode.c_str());
		}
		else
		{
			HttpClient client(hostAndPort);
			client.config.timeout = (time_t)10;
			client.config.timeout_connect = (time_t)10;
			auto res = client.request("GET", finalPath);
			retCode = res->status_code;
			httpCode = atoi(retCode.c_str());
		}

		if (httpCode != 200)
		{
			Logger::getLogger()->error("Failed to send notification via Blynk REST API, "
						   "URL '%s', httpCode %d",
						   (hostAndPort + finalPath).c_str(),
						   httpCode);
			return false;
		}
	}
	catch (exception &ex)
	{
		Logger::getLogger()->error("Failed to send notification via Blynk REST API, "
					   "URL '%s', exception '%s'",
					   (hostAndPort + finalPath).c_str(),
					   ex.what());
		return false;
	}
	catch (...)
	{
		std::exception_ptr p = std::current_exception();
		string name = (p ? p.__cxa_exception_type()->name() : "null");
		Logger::getLogger()->error("Failed to send notification via Blynk REST API, "
					   "URL '%s', generic exception '%s'",
					   finalPath.c_str(),
					   name.c_str());
		return false;
	}

	return true;
}

/**
 * Reconfigure the delivery plugin
 *
 * @param newConfig	The new configuration
 */
void Blynk::reconfigure(const string& newConfig)
{
	ConfigCategory category("new", newConfig);

	// Configuration change is protected by a lock
	lock_guard<mutex> guard(m_configMutex);

	m_token = category.getValue("token");
	m_url = category.getValue("api_url");
	m_pin = category.getValue("pin");

	if (m_token.empty() ||
	    m_pin.empty() ||
	    m_url.empty())
	{
		m_enable = false;
	}
	else
	{
        	m_enable = category.getValue("enable").compare("true") == 0 ||
			   category.getValue("enable").compare("True") == 0;
	}
}
