/**
 * package:		Part of vpl-jail-system
 * copyright:	Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino. All rights reserved.
 * license:		GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/
#include <sys/syslog.h>
#include <climits>
#include "configuration.h"
#include "configurationFile.h"

#include "jail_limits.h"
#include "util.h"
Configuration *Configuration::singlenton=0;
string Configuration::generateCleanPATH(string jailPath, string dirtyPATH){
	string dir;
	size_t pos=0, found;
	string cleanPATH="";
	while(true){
		if((found=dirtyPATH.find(':',pos)) != string::npos){
			dir=dirtyPATH.substr(pos,found-pos);
		}else{
			dir=dirtyPATH.substr(pos);
		}
		if(Util::dirExistsFollowingSymLink(jailPath+dir)){
			if(cleanPATH.size()>0){
				cleanPATH+=':';
			}
			cleanPATH+=dir;
		} else {
			syslog(LOG_INFO, "Path removed: %s", dir.c_str());
		}
		if(found == string::npos) return cleanPATH;
		pos=found+1;
	}
	return "";
}

void Configuration::checkConfigFile(string fileName, string men){
	struct stat info;
	if(lstat(fileName.c_str(),&info))
		throw men+" not found "+fileName;
	if(info.st_uid !=0 || info.st_gid !=0)
		throw men+" not owned by root "+fileName;
	if(info.st_mode & 077)
		throw men+" with insecure permissions (must be 0600/0700)" +fileName;
}

void Configuration::readConfigFile(){
	ConfigData configDefault;
	//Values used if parmeter not present in config file
	configDefault["MAXTIME"]=Util::itos(1200);
	configDefault["MAXFILESIZE"]=Util::itos(64*1024*1024);
	configDefault["MAXMEMORY"]=Util::itos(2000*1024*1024);
	configDefault["MAXPROCESSES"]=Util::itos(500);
	configDefault["MIN_PRISONER_UGID"]="10000";
	configDefault["MAX_PRISONER_UGID"]="20000";
	configDefault["JAILPATH"]="/jail";
	configDefault["CONTROLPATH"]="/var/vpl-jail-system";
	configDefault["TASK_ONLY_FROM"]="";
	configDefault["INTERFACE"]="";
	configDefault["PORT"]="80";
	configDefault["SECURE_PORT"]="443";
	configDefault["URLPATH"]="/";
	configDefault["ENVPATH"]=Util::getEnv("PATH");
	configDefault["LOGLEVEL"]="0";
	configDefault["FAIL2BAN"]="0";
	ConfigData data=ConfigurationFile::readConfiguration(configPath,configDefault);
	minPrisoner = atoi(data["MIN_PRISONER_UGID"].c_str());
	if(minPrisoner < JAIL_MIN_PRISIONER_UID)
		throw "Incorrect MIN_PRISONER_UGID config value"
		+data["MIN_PRISONER_UGID"];
	maxPrisoner= atoi(data["MAX_PRISONER_UGID"].c_str());
	if(maxPrisoner > JAIL_MAX_PRISIONER_UID)
		throw "Incorrect MAX_PRISONER_UGID config value"
		+data["MAX_PRISONER_UGID"];
	if(minPrisoner>maxPrisoner || minPrisoner<JAIL_MIN_PRISIONER_UID || maxPrisoner>JAIL_MAX_PRISIONER_UID)
		throw "Incorrect config file, prisoner uid inconsistency";
	jailPath = data["JAILPATH"];
	if(!Util::correctPath(jailPath))
		throw "Incorrect Jail"+jailPath;
	jailPath=jailPath[0]=='/'?jailPath:"/"+jailPath; //Add absolute path
	if(jailPath == "/")
		throw "Jail path can NOT be set to /";
	controlPath = data["CONTROLPATH"];
	controlPath=controlPath[0]=='/'?controlPath:"/"+controlPath; //Add absolute path
	if(!Util::correctPath(controlPath))
		throw "Incorrect control path "+controlPath;
	jailLimits.maxtime = atoi(data["MAXTIME"].c_str());
	jailLimits.maxfilesize=atoi(data["MAXFILESIZE"].c_str());
	jailLimits.maxmemory=atoi(data["MAXMEMORY"].c_str());
	jailLimits.maxprocesses=atoi(data["MAXPROCESSES"].c_str());
	string tof=data["TASK_ONLY_FROM"];
	string dir;
	size_t pos=0;
	while((pos=tof.find(" ",pos)) != string::npos){
		dir=tof.substr(0,pos);
		Util::trim(dir);
		if(dir.length()>0)
			taskOnlyFrom.push_back(dir);
		tof.erase(0,pos);
	}
	dir=tof.substr(0,pos);
	Util::trim(dir);
	if(dir.size()>0)
		taskOnlyFrom.push_back(dir);
	interface=data["INTERFACE"];
	port=atoi(data["PORT"].c_str());
	sport=atoi(data["SECURE_PORT"].c_str());
	URLPath = data["URLPATH"];
	cleanPATH = data["ENVPATH"];
	logLevel = atoi(data["LOGLEVEL"].c_str());
	fail2ban = atoi(data["FAIL2BAN"].c_str());
}

Configuration::Configuration(){
	configPath="/etc/vpl/vpl-jail-system.conf";
	checkConfigFile(configPath,"Config file");
	readConfigFile();
//cleanPATH=generateCleanPATH(getJailPath(),cleanPATH);
}


