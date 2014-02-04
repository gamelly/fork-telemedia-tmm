/*
 * Project.cpp
 *
 *  Created on: 05/02/2013
 *      Author: Felippe Nagato
 */

#include "project/Project.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace tool {

Project::Project() {
	projectList = new map<int, ProjectInfo*>;
	(*projectList)[0] = new PPat();
	stcBegin = SYSTEM_CLOCK_FREQUENCY * 10;
	isLoop = false;
	vbvBuffer = 1.0;
	ttl = 16;
	packetsInBuffer = 40;
	useTot = false;
	useSdt = false;
	useNit = false;
	packetSize = 188;
	iip = NULL;
	isPipe = false;
}

Project::~Project() {
	map<int, ProjectInfo*>::iterator itProj;

	if (projectList) {
		itProj = projectList->begin();
		while (itProj != projectList->end()) {
			delete (itProj->second);
			++itProj;
		}
		delete projectList;
	}

	if (iip) delete iip;
}

bool Project::changeToProjectDir() {
	char* currDir = getcwd(NULL, 0);
	if (currDir) {
		string currDirString;
		currDirString.assign(currDir);
		tmmPath.assign(currDir);
		unsigned found = filename.find_last_of("/\\");
		currDirString += getUriSlash() + filename.substr(0,found);
		chdir(currDirString.c_str());
		return true;
	}
	return false;
}

unsigned char Project::toLayer(string layer) {
	if (layer == "none") {
		return NULL_TSP;
	} else if (tolower(layer.c_str()[0]) == 'a') {
		return HIERARCHY_A;
	} else if (tolower(layer.c_str()[0]) == 'b') {
		return HIERARCHY_B;
	} else if (tolower(layer.c_str()[0]) == 'c') {
		return HIERARCHY_C;
	} else {
		return 0xFF;
	}
}

void Project::setPacketSize(unsigned char size) {
	packetSize = size;
}

unsigned char Project::getPacketSize() {
	return packetSize;
}

bool Project::mountCarousel(PCarousel* pcar) {
	char number[11];
	string path, tempPath;

	pcar->setSectionEncapsulationMode(true);
	pcar->setBlockSize(4066);
	snprintf(number, 11, "%d", pcar->getServiceDomain());

	path = "." + getUriSlash() + "carousel" + getUriSlash();
	makeDir(path.c_str(), 0777);
	path.insert(path.size(), number);
	makeDir(path.c_str(), 0777);
	tempPath.assign(path);
	path.insert(path.size(), getUriSlash() + "output" + getUriSlash());
	makeDir(path.c_str(), 0777);
	tempPath.insert(tempPath.size(), getUriSlash() + "temp" + getUriSlash());
	makeDir(tempPath.c_str(), 0);
	path.insert(path.size(), number);
	path.insert(path.size(), ".sec");
	pcar->setOutputFile(path);
	pcar->setTempFolder(tempPath);
	pcar->createCarousel(path, tempPath);

	return true;
}

bool Project::mountCarousels() {
	map<int, ProjectInfo*>::iterator itProj;

	if (projectList) {
		itProj = projectList->begin();
		while (itProj != projectList->end()) {
			if (itProj->second->getProjectType() != PT_CAROUSEL) {
				++itProj;
				continue;
			}
			mountCarousel((PCarousel*) itProj->second);
			++itProj;
		}
	}
	return true;
}

bool Project::createStreamEvent(PStreamEvent* pse) {
	char privateStr[252];
	StreamEvent* se;
	vector<StreamEvent*>::iterator itSe;
	vector<StreamEvent*>* sel;
	map<string, InternalIor*>* fi;
	map<string, InternalIor*>::iterator it;
	InternalIor* iior = NULL;
	int ret;

	if (!pse) {
		return false;
	}

	if (pse->getCarouselProj() && pse->getEntryPoint().size()) {
		fi = ((PCarousel*)pse->getCarouselProj())->getFilesIor();
		it = fi->find(pse->getEntryPoint());
		if (it == fi->end()) {
			string fn1, fn2;
			unsigned int found;
			it = fi->begin();
			while (it != fi->end()) {
				fn1.assign(it->first);
				found = fn1.find_last_of("/\\");
				if (found != std::string::npos) {
					fn1 = fn1.substr(found + 1);
					found = pse->getEntryPoint().find_last_of("/\\");
					if (found != std::string::npos) {
						fn2 = pse->getEntryPoint().substr(found + 1);
						if (fn2 == fn1) {
							if (!pse->getDocumentId().size())
								pse->setDocumentId(LocalLibrary::extractBaseId(it->first));
							cout << "Project::createStreamEvent - Selected entrypoint: "
								 << it->first << endl;
							break;
						}
					}
				}
				++it;
			}
		}
		if (it != fi->end()) {
			iior = it->second;
		} else {
			cout << "Project::createStreamEvent - The " << pse->getEntryPoint()
				 << " file doesn't exists." << endl;
			return -8;
		}
	}
	if (!pse->getBaseId().size()) {
		pse->setBaseId(tsid);
	}
	sel = pse->getStreamEventList();
	for (itSe = sel->begin(); itSe != sel->end(); ++itSe) {
		se = *itSe;
		if (!se->getPrivateDataPayloadLength()) {
			ret = 0;
			switch (se->getCommandTag()) {
			case SE_ADD_DOCUMENT:
				ret = sprintf(privateStr,
						"%s,x-sbtvd://%s,%d,%d,%d",
						pse->getBaseId().c_str(), pse->getEntryPoint().c_str(),
						iior->carousel, iior->moduleId,
						iior->key);
				break;
			case SE_START_DOCUMENT:
				ret = sprintf(privateStr,
						"%s,%s,,,,",
						pse->getBaseId().c_str(),
						pse->getDocumentId().c_str());
				break;
			}
			if (ret > 0) se->setPrivateDataPayload(privateStr, ret);
		}
	}

	return true;
}

bool Project::createStreamEvents() {
	map<int, ProjectInfo*>::iterator itProj;

	if (projectList) {
		itProj = projectList->begin();
		while (itProj != projectList->end()) {
			if (itProj->second->getProjectType() != PT_STREAMEVENT) {
				++itProj;
				continue;
			}
			createStreamEvent((PStreamEvent*) itProj->second);
			++itProj;
		}
	}
	return true;
}

int Project::configAit(PAit* ait, unsigned int ctag, string aName, string lang,
			  string baseDir, string initClass, unsigned int orgId,
			  unsigned short appId, unsigned int appcode) {
	vector<MpegDescriptor*>* dlist = new vector<MpegDescriptor*>;

	TransportProtocol* tp = new TransportProtocol();
	tp->setProtocolId(0x01);
	tp->setTransportProtocolLabel(0x00);
	ObjectCarouselProtocol* ocp = new ObjectCarouselProtocol;
	ocp->remoteConnection = false;
	ocp->componentTag = ctag;
	ocp->originalNetworkId = originalNetworkId;
	ocp->transportStreamId = tsid;
	//ocp->serviceId: use configAitService() to define this field,
	//because it depends on the service that it's broadcasted.
	tp->setOcProtocol(ocp);
	dlist->push_back(tp);

	Application* appDesc = new Application();
	//appDesc->addAppProfile(0x8001, 0x01, 0x00, 0x00); //GLOBO
	appDesc->addAppProfile(0x00, 0x00, 0x00, 0x00); //SBT
	appDesc->setServiceBoundFlag(true);
	appDesc->setVisibility(0x03);
	appDesc->setApplicationPriority(0x01);
	char pl[1];
	pl[0] = 0x00;
	appDesc->setTransportProtocolLabel(pl, 1);
	dlist->push_back(appDesc);

	ApplicationName* appName = new ApplicationName();
	appName->setAppName(lang, aName);
	dlist->push_back(appName);

	GingaApplication* ga = NULL;
	GingaApplicationLocation* gal = NULL;
	if (ait->getTableIdExtension() == AT_GINGA_NCL) {
		ga = new GingaApplication(0x06);
		gal = new GingaApplicationLocation(0x07);
	} else if (ait->getTableIdExtension() == AT_GINGA_J) {
		ga = new GingaApplication(0x03);
		gal = new GingaApplicationLocation(0x04);
	}
	if (ga) dlist->push_back(ga);
	if (gal) {
		gal->setBaseDirectory(baseDir);
		gal->setInitialClass(initClass);
		dlist->push_back(gal);
	}

	ait->addApplicationInfo(orgId, appId, appcode, dlist);

	return 0;
}

int Project::configAitService(ProjectInfo* ait, unsigned short serviceId,
		unsigned char ctag) {
	vector<MpegDescriptor*>::iterator itDesc;
	vector<AppInformation*>::iterator itApp;
	PAit* pAit = (PAit*)ait;
	vector<AppInformation*>* appList = pAit->getAppInformationList();

	for (itApp = appList->begin(); itApp != appList->end(); ++itApp) {
		for (itDesc = (*itApp)->appDescriptorList->begin();
				itDesc != (*itApp)->appDescriptorList->end(); ++itDesc) {
			if ((*itDesc)->getDescriptorTag() == 0x02) {
				TransportProtocol* tp = (TransportProtocol*)(*itDesc);
				if (tp) {
					ObjectCarouselProtocol* ocp = tp->getOcProtocol();
					if (ocp) {
						ocp->serviceId = serviceId;
						ocp->componentTag = ctag;
					}
				}
				return 0;
			}
		}
	}

	return 0;
}

int Project::configSdt(vector<pmtViewInfo*>* newTimeline, ProjectInfo* sdt) {
	map<unsigned short, PMTView*> orderedPmtList;
	map<unsigned short, PMTView*>::iterator itOrd;
	vector<pmtViewInfo*>::iterator itPmt;

	PSdt* pSdt = (PSdt*)sdt;

	if (sdt == NULL) return -1;
	pSdt->releaseAllServiceInformation();
	pSdt->setCurrentNextIndicator(1);
	pSdt->setOriginalNetworkId(originalNetworkId);
	pSdt->setTableIdExtension(tsid);
	itPmt = newTimeline->begin();
	while (itPmt != newTimeline->end()) {
		orderedPmtList[(*itPmt)->pv->getPid()] = (*itPmt)->pv;
		++itPmt;
	}
	itOrd = orderedPmtList.begin();
	while (itOrd != orderedPmtList.end()) {
		ServiceInformation* si = new ServiceInformation();
		Service* service = new Service();
		service->setServiceName(itOrd->second->getServiceName());
		service->setProviderName(providerName);
		if (itOrd->second->getEitProj()) {
			si->eitPresentFollowingFlag = true;
		} else {
			si->eitPresentFollowingFlag = false;
		}
		si->eitScheduleFlag = false;
		si->freeCaMode = 0;
		si->runningStatus = 4; //Running
		si->serviceId = itOrd->second->getProgramNumber();
		si->descriptorList.push_back(service);
		pSdt->addServiceInformation(si);
		++itOrd;
	}

	return 0;
}

int Project::configNit(vector<pmtViewInfo*>* newTimeline, ProjectInfo* nit) {
	map<unsigned short, PMTView*> orderedPmtList;
	map<unsigned short, PMTView*>::iterator itOrd;
	vector<pmtViewInfo*>::iterator itPmt;
	PNit* pNit = (PNit*)nit;

	if (pNit == NULL) return -1;

	itPmt = newTimeline->begin();
	while (itPmt != newTimeline->end()) {
		orderedPmtList[(*itPmt)->pv->getPid()] = (*itPmt)->pv;
		++itPmt;
	}

	pNit->releaseAllDescriptors();
	pNit->releaseAllTransportInformation();
	pNit->setCurrentNextIndicator(1);
	pNit->setTableIdExtension(originalNetworkId);
	NetworkName* netName = new NetworkName();
	netName->setNetworkName(providerName);
	pNit->addDescriptor(netName);
	SystemManagement* sysMan = new SystemManagement();
	pNit->addDescriptor(sysMan);
	TransportInformation* ti = new TransportInformation();
	ti->transportStreamId = tsid;
	ti->originalNetworkId = pNit->getTableIdExtension();
	ServiceList* sl = new ServiceList();
	itOrd = orderedPmtList.begin();
	while (itOrd != orderedPmtList.end()) {
		sl->addService(itOrd->second->getProgramNumber(), DIGITAL_TELEVISION_SERVICE);
		++itOrd;
	}
	ti->descriptorList.push_back(sl);
	TerrestrialDeliverySystem* tds = new TerrestrialDeliverySystem();
	tds->setGuardInterval(guardInterval);
	tds->setTransmissionMode(transmissionMode);
	tds->addFrequency(broadcastFrequency);
	ti->descriptorList.push_back(tds);

	if (orderedPmtList.size()) {
		PartialReception* pr = new PartialReception();
		pr->addServiceId(orderedPmtList.begin()->second->getProgramNumber());
		ti->descriptorList.push_back(pr);
	}

	TSInformation* tsinfo = new TSInformation();
	tsinfo->setRemoteControlKeyId(virtualChannel);
	tsinfo->setTsName(tsName);
	itPmt = newTimeline->begin();
	while (itPmt != newTimeline->end()) {
		TransmissionType* tt = new TransmissionType();
		tt->transmissionTypeInfo = 0x0F; // ???
		tt->serviceIdList.insert((*itPmt)->pv->getProgramNumber());
		tsinfo->addTransmissionTypeList(tt);
		++itPmt;
	}
	ti->descriptorList.push_back(tsinfo);
	pNit->addTransportInformation(ti);

	return 0;
}

void Project::setFilename(string filename) {
	this->filename = filename;
}

string Project::getFilename() {
	return filename;
}

string Project::getProjectName() {
	return projectName;
}

string Project::getProjectDescription() {
	return projectDescription;
}

void Project::setDestination(string dest) {
	destination = dest;
}

string Project::getDestination() {
	return destination;
}

bool Project::getIsPipe() {
	return isPipe;
}

string Project::getExternalApp() {
	return externalApp;
}

string Project::getAppParams() {
	return appParams;
}

void Project::setProviderName(string name) {
	providerName = name;
}

string Project::getProviderName() {
	return providerName;
}

void Project::setTsid(int id) {
	tsid = id;
}

int Project::getTsid() {
	return tsid;
}

IIP* Project::getIip() {
	return iip;
}

void Project::setTsBitrate(int rate) {
	tsBitrate = rate;
}

int Project::getTsBitrate() {
	return tsBitrate;
}

void Project::setStcBegin(int64_t stc) {
	stcBegin = stc;
}

int64_t Project::getStcBegin() {
	return stcBegin;
}

map<int, ProjectInfo*>* Project::getProjectList() {
	return projectList;
}

double Project::getVbvBuffer() {
	return vbvBuffer;
}

bool Project::getIsLoop(){
	return isLoop;
}

unsigned char Project::getTTL() {
	return ttl;
}

unsigned short Project::getOriginalNetworkId() {
	return originalNetworkId;
}

string Project::getTsName() {
	return tsName;
}

unsigned short Project::getBroadcastFrequency() {
	return broadcastFrequency;
}

unsigned char Project::getVirtualChannel() {
	return virtualChannel;
}

unsigned char Project::getGuardInterval() {
	return guardInterval;
}

unsigned char Project::getTransmissionMode() {
	return transmissionMode;
}

unsigned short Project::getPacketsInBuffer() {
	return packetsInBuffer;
}

ProjectInfo* Project::findProject(int id) {
	map<int, ProjectInfo*>::iterator itProj;

	if (projectList) {
		itProj = projectList->find(id);
		if (itProj != projectList->end()) {
			return itProj->second;
		}
	}

	return NULL;
}

void Project::setUseTot(bool use) {
	useTot = use;
}

bool Project::getUseTot() {
	return useTot;
}

void Project::setUseSdt(bool use) {
	useSdt = use;
}

bool Project::getUseSdt() {
	return useSdt;
}

void Project::setUseNit(bool use) {
	useNit = use;
}

bool Project::getUseNit() {
	return useNit;
}

}
}
}
}


