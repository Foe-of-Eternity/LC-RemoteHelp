// Copyright (C) 2008,2009,2010,2011,2012 GlavSoft LLC.
// All rights reserved.
//
//-------------------------------------------------------------------------
// This file is part of the TightVNC software.  Please visit our Web site:
//
//                       http://www.tightvnc.com/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//-------------------------------------------------------------------------
//

#include "win-system/Environment.h"
#include "wsconfig-lib/TvnLogFilename.h"
#include "config-lib/IniFileSettingsManager.h"

#include "win-system/Registry.h"
#include "win-system/RegistryKey.h"

#include "Configurator.h"
#include "tvnserver-app/NamingDefs.h"

Configurator *Configurator::s_instance = NULL;
LocalMutex Configurator::m_instanceMutex;

Configurator::Configurator(bool isConfiguringService)
: m_isConfiguringService(isConfiguringService), m_isConfigLoadedPartly(false),
  m_isFirstLoad(true), m_regSA(0)
{
  AutoLock al(&m_instanceMutex);
  if (s_instance != 0) {
    throw Exception(_T("Configurator instance already exists"));
  }
  s_instance = this;
  try {
    m_regSA = new RegistrySecurityAttributes();
  } catch (...) {
    // TODO: Place exception handler here.
  }
}

Configurator::~Configurator()
{
  if (m_regSA != 0) delete m_regSA;
}

Configurator *Configurator::getInstance()
{
  AutoLock al(&m_instanceMutex);
  _ASSERT(s_instance != NULL);
  return s_instance;
}

void Configurator::setInstance(Configurator *conf)
{
  s_instance = conf;
}

void Configurator::notifyReload()
{
  AutoLock l(&m_listeners);

  for (size_t i = 0; i < m_listeners.size(); i++) {
    m_listeners.at(i)->onConfigReload(getServerConfig());
  }
}

bool Configurator::load()
{
  return load(m_isConfiguringService);
}

bool Configurator::save()
{
  return save(m_isConfiguringService);
}

bool Configurator::load(bool forService)
{
  bool isOk = false;

  IniFileSettingsManager sm(IniFilePaths::SERVER_PATH, IniFileKeys::SERVER_CONFIG);

  isOk = load(&sm);

  notifyReload();

  return isOk;
}

bool Configurator::save(bool forService)
{
  bool isOk = false;

  IniFileSettingsManager sm(IniFilePaths::SERVER_PATH, IniFileKeys::SERVER_CONFIG);

  isOk = save(&sm);

  return isOk;
}

bool Configurator::save(SettingsManager *sm)
{
  bool saveResult = true;
  if (!saveInputHandlingConfig(sm)) {
    saveResult = false;
  }
  if (!saveServerConfig(sm)) {
    saveResult = false;
  }
  if (!saveVideoRegionConfig(sm)) {
    saveResult = false;
  }
  return saveResult;
}

bool Configurator::load(SettingsManager *sm)
{
  bool loadResult = true;

  if (!loadInputHandlingConfig(sm, &m_serverConfig)) {
    loadResult = false;
  }

  if (!loadServerConfig(sm, &m_serverConfig)) {
    loadResult = false;
  }
  if (!loadVideoRegionConfig(sm, &m_serverConfig)) {
    loadResult = false;
  }

  m_isFirstLoad = false;

  return loadResult;
}

bool Configurator::saveInputHandlingConfig(SettingsManager *sm)
{
  bool saveResult = true;
  if (!sm->setUINT(_T("LocalInputPriorityTimeout"), m_serverConfig.getLocalInputPriorityTimeout())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("LocalInputPriority"), m_serverConfig.isLocalInputPriorityEnabled())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("BlockRemoteInput"), m_serverConfig.isBlockingRemoteInput())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("BlockLocalInput"), m_serverConfig.isBlockingLocalInput())) {
    saveResult = false;
  }
  return saveResult;
}

bool Configurator::loadInputHandlingConfig(SettingsManager *sm, ServerConfig *config)
{
  bool loadResult = true;

  //
  // Temporary variables
  //

  bool boolVal = false;
  UINT uintVal = 0;

  if (!sm->getUINT(_T("LocalInputPriorityTimeout"), &uintVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    config->setLocalInputPriorityTimeout(uintVal);
  }
  if (!sm->getBoolean(_T("LocalInputPriority"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    config->setLocalInputPriority(boolVal);
  }
  if (!sm->getBoolean(_T("BlockRemoteInput"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    config->blockRemoteInput(boolVal);
  }
  if (!sm->getBoolean(_T("BlockLocalInput"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    config->blockLocalInput(boolVal);
  }

  return loadResult;
}

bool Configurator::saveVideoRegionConfig(SettingsManager *sm)
{
  bool saveResult = true;

  StringStorage buffer;
  StringVector *videoClasses = m_serverConfig.getVideoClassNames();
  size_t size = videoClasses->size();

  AutoLock al(&m_serverConfig);
  buffer.setString(_T(""));
  for (size_t i = 0; i < size; i++) {
    buffer.appendString(videoClasses->at(i).getString());
    if (i != size - 1) {
      buffer.appendString(_T("\n"));
    }
  }
  if (!sm->setString(_T("VideoClasses"), buffer.getString())) {
    saveResult = false;
  }
  return saveResult;
}

bool Configurator::loadVideoRegionConfig(SettingsManager *sm, ServerConfig *config)
{
  bool loadResult = true;

  StringVector *videoClasses = m_serverConfig.getVideoClassNames();

  // Lock configuration
  AutoLock al(&m_serverConfig);

  //
  // Delete old video classes entries
  //

  videoClasses->clear();

  //
  // Try to load.
  //

  StringStorage storage;

  if (!sm->getString(_T("VideoClasses"), &storage)) {
    loadResult = false;
  }

  //
  // Split.
  //

  size_t count = 0;

  storage.split(_T("\n"), NULL, &count);
  if (count != 0) {
    std::vector<StringStorage> chunks(count);
    storage.split(_T("\n"), &chunks.front(), &count);

    for (size_t i = 0; i < count; i++) {
      if (!chunks[i].isEmpty()) {
        videoClasses->push_back(chunks[i]);
      }
    }
  }

  return loadResult;
}

bool Configurator::saveServerConfig(SettingsManager *sm)
{
  bool saveResult = true;
  if (!sm->setUINT(_T("DisconnectAction"), (UINT)m_serverConfig.getDisconnectAction())) {
    saveResult = false;
  }
  if (!sm->setUINT(_T("LogLevel"), (UINT)m_serverConfig.getLogLevel())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("EnableFileTransfers"), m_serverConfig.isFileTransfersEnabled())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("RemoveWallpaper"), m_serverConfig.isRemovingDesktopWallpaperEnabled())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("UseMirrorDriver"), m_serverConfig.getMirrorIsAllowed())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("AlwaysShared"), m_serverConfig.isAlwaysShared())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("NeverShared"), m_serverConfig.isNeverShared())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("DisconnectClients"), m_serverConfig.isDisconnectingExistingClients())) {
    saveResult = false;
  }
  if (!sm->setUINT(_T("PollingInterval"), m_serverConfig.getPollingInterval())) {
    saveResult = false;
  }
  if (!sm->setUINT(_T("VideoRecognitionInterval"), m_serverConfig.getVideoRecognitionInterval())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("GrabTransparentWindows"), m_serverConfig.getGrabTransparentWindowsFlag())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("SaveLogToAllUsersPath"), m_serverConfig.isSaveLogToAllUsersPathFlagEnabled())) {
    saveResult = false;
  }
  if (!sm->setBoolean(_T("RunControlInterface"), m_serverConfig.getShowTrayIconFlag())) {
    saveResult = false;
  }
  return saveResult;
}

bool Configurator::loadServerConfig(SettingsManager *sm, ServerConfig *config)
{
  bool loadResult = true;

  //
  // Temporary variables
  //

  bool boolVal;
  UINT uintVal;

  if (!sm->getUINT(_T("DisconnectAction"), &uintVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.setDisconnectAction((ServerConfig::DisconnectAction)uintVal);
  }
  if (!sm->getUINT(_T("LogLevel"), &uintVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.setLogLevel(uintVal);
  }
  if (!sm->getBoolean(_T("EnableFileTransfers"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.enableFileTransfers(boolVal);
  }
  if (!sm->getBoolean(_T("RemoveWallpaper"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.enableRemovingDesktopWallpaper(boolVal);
  }
  if (!sm->getBoolean(_T("UseMirrorDriver"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.setMirrorAllowing(boolVal);
  }
  if (!sm->getBoolean(_T("AlwaysShared"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.setAlwaysShared(boolVal);
  }
  if (!sm->getBoolean(_T("NeverShared"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.setNeverShared(boolVal);
  }
  if (!sm->getBoolean(_T("DisconnectClients"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.disconnectExistingClients(boolVal);
  }
  if (!sm->getUINT(_T("PollingInterval"), &uintVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.setPollingInterval(uintVal);
  }
  if (!sm->getUINT(_T("VideoRecognitionInterval"), &uintVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.setVideoRecognitionInterval(uintVal);
  }
  if (!sm->getBoolean(_T("GrabTransparentWindows"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.setGrabTransparentWindowsFlag(boolVal);
  }
  if (!sm->getBoolean(_T("SaveLogToAllUsersPath"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.saveLogToAllUsersPath(boolVal);
  }
  if (!sm->getBoolean(_T("RunControlInterface"), &boolVal)) {
    loadResult = false;
  } else {
    m_isConfigLoadedPartly = true;
    m_serverConfig.setShowTrayIconFlag(boolVal);
  }
  updateLogDirPath();
  return loadResult;
}

void Configurator::updateLogDirPath()
{
  StringStorage pathToLogDirectory;
  TvnLogFilename::queryLogFileDirectory(m_isConfiguringService,
    m_serverConfig.isSaveLogToAllUsersPathFlagEnabled(),
    &pathToLogDirectory);
  m_serverConfig.setLogFileDir(pathToLogDirectory.getString());
}
