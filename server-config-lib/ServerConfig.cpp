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

#include "ServerConfig.h"

#include "win-system/Environment.h"

#include "file-lib/File.h"

ServerConfig::ServerConfig()
: m_logLevel(0),
  m_enableFileTransfers(true),
  m_mirrorDriverAllowed(true),
  m_removeWallpaper(true),
  m_alwaysShared(false), m_neverShared(true),
  m_disconnectClients(true), m_pollingInterval(1000), m_localInputPriorityTimeout(3),
  m_blockLocalInput(false), m_blockRemoteInput(false), m_localInputPriority(false),
  m_grabTransparentWindows(true),
  m_logFilePath(_T(".\\"))
{
}

ServerConfig::~ServerConfig()
{
}

void ServerConfig::serialize(DataOutputStream *output)
{
  AutoLock l(this);

  output->writeInt8(m_enableFileTransfers ? 1 : 0);
  output->writeInt8(m_removeWallpaper ? 1 : 0);
  output->writeInt8(m_mirrorDriverAllowed ? 1 : 0);
  output->writeInt32(m_logLevel);
  output->writeInt8(m_alwaysShared ? 1 : 0);
  output->writeInt8(m_neverShared ? 1 : 0);
  output->writeInt8(m_disconnectClients ? 1 : 0);
  output->writeUInt32(m_pollingInterval);
  output->writeInt8(m_blockRemoteInput ? 1 : 0);
  output->writeInt8(m_blockLocalInput ? 1 : 0);
  output->writeInt8(m_localInputPriority ? 1 : 0);
  output->writeUInt32(m_localInputPriorityTimeout);

  output->writeInt8(m_grabTransparentWindows ? 1 : 0);

  output->writeUTF8(m_autoConnectHost.getString());
  output->writeUTF8(m_logFilePath.getString());
}

void ServerConfig::deserialize(DataInputStream *input)
{
  AutoLock l(this);

  m_enableFileTransfers = input->readInt8() == 1;
  m_removeWallpaper = input->readInt8() == 1;
  m_mirrorDriverAllowed = input->readInt8() != 0;
  m_logLevel = input->readInt32();
  m_alwaysShared = input->readInt8() == 1;
  m_neverShared = input->readInt8() == 1;
  m_disconnectClients = input->readInt8() == 1;
  m_pollingInterval = input->readUInt32();
  m_blockRemoteInput = input->readInt8() == 1;
  m_blockLocalInput = input->readInt8() == 1;
  m_localInputPriority = input->readInt8() == 1;
  m_localInputPriorityTimeout = input->readUInt32();

  m_grabTransparentWindows = input->readInt8() == 1;

  input->readUTF8(&m_autoConnectHost);
  input->readUTF8(&m_logFilePath);
}

void ServerConfig::getLogFileDir(StringStorage *logFilePath)
{
  AutoLock l(this);

  *logFilePath = m_logFilePath;
}

void ServerConfig::setLogFileDir(const TCHAR *logFilePath)
{
  AutoLock l(this);

  m_logFilePath.setString(logFilePath);
}

void ServerConfig::enableFileTransfers(bool enabled)
{
  AutoLock lock(&m_objectCS);
  m_enableFileTransfers = enabled;
}

bool ServerConfig::isFileTransfersEnabled()
{
  AutoLock lock(&m_objectCS);
  return m_enableFileTransfers;
}

void ServerConfig::enableRemovingDesktopWallpaper(bool enabled)
{
  AutoLock lock(&m_objectCS);
  m_removeWallpaper = enabled;
}

bool ServerConfig::isRemovingDesktopWallpaperEnabled()
{
  AutoLock lock(&m_objectCS);
  return m_removeWallpaper;
}

bool ServerConfig::getMirrorIsAllowed()
{
  AutoLock lock(&m_objectCS);
  return m_mirrorDriverAllowed;
}

void ServerConfig::setMirrorAllowing(bool value)
{
  AutoLock lock(&m_objectCS);
  m_mirrorDriverAllowed = value;
}

int ServerConfig::getLogLevel()
{
  AutoLock lock(&m_objectCS);
  return m_logLevel;
}

void ServerConfig::setLogLevel(int logLevel)
{
  AutoLock lock(&m_objectCS);
  if (logLevel < 0) {
    m_logLevel = 0;
  } else if (logLevel > 9) {
    m_logLevel = 9;
  } else {
    m_logLevel = logLevel;
  }
}

bool ServerConfig::isAlwaysShared()
{
  AutoLock lock(&m_objectCS);
  return m_alwaysShared;
}

bool ServerConfig::isNeverShared()
{
  AutoLock lock(&m_objectCS);
  return m_neverShared;
}

bool ServerConfig::isDisconnectingExistingClients()
{
  AutoLock lock(&m_objectCS);
  return m_disconnectClients;
}

void ServerConfig::setAlwaysShared(bool enabled)
{
  AutoLock lock(&m_objectCS);
  m_alwaysShared = enabled;
}

void ServerConfig::setNeverShared(bool enabled)
{
  AutoLock lock(&m_objectCS);
  m_neverShared = enabled;
}

void ServerConfig::disconnectExistingClients(bool disconnectExisting)
{
  AutoLock lock(&m_objectCS);
  m_disconnectClients = disconnectExisting;
}

void ServerConfig::setPollingInterval(unsigned int interval)
{
  AutoLock lock(&m_objectCS);
  if (interval < MINIMAL_POLLING_INTERVAL) {
    m_pollingInterval = MINIMAL_POLLING_INTERVAL;
  } else {
    m_pollingInterval = interval;
  }
}

unsigned int ServerConfig::getPollingInterval()
{
  AutoLock lock(&m_objectCS);
  return m_pollingInterval;
}

void ServerConfig::blockRemoteInput(bool blockEnabled)
{
  AutoLock lock(&m_objectCS);
  m_blockRemoteInput = blockEnabled;
}

bool ServerConfig::isBlockingRemoteInput()
{
  AutoLock lock(&m_objectCS);
  return m_blockRemoteInput;
}

void ServerConfig::setLocalInputPriority(bool localPriority)
{
  AutoLock lock(&m_objectCS);
  m_localInputPriority = localPriority;
}

bool ServerConfig::isLocalInputPriorityEnabled()
{
  AutoLock lock(&m_objectCS);
  return m_localInputPriority;
}

unsigned int ServerConfig::getLocalInputPriorityTimeout()
{
  AutoLock lock(&m_objectCS);
  return m_localInputPriorityTimeout;
}

void ServerConfig::setLocalInputPriorityTimeout(unsigned int value)
{
  AutoLock lock(&m_objectCS);
  if (value < MINIMAL_LOCAL_INPUT_PRIORITY_TIMEOUT) {
    m_localInputPriorityTimeout = MINIMAL_LOCAL_INPUT_PRIORITY_TIMEOUT;
  } else {
    m_localInputPriorityTimeout = value;
  }
}

void ServerConfig::blockLocalInput(bool enabled)
{
  AutoLock lock(&m_objectCS);
  m_blockLocalInput = enabled;
}

bool ServerConfig::isBlockingLocalInput()
{
  AutoLock lock(&m_objectCS);
  return m_blockLocalInput;
}

void ServerConfig::setGrabTransparentWindowsFlag(bool grab)
{
  AutoLock lock(&m_objectCS);
  m_grabTransparentWindows = grab;
}

bool ServerConfig::getGrabTransparentWindowsFlag()
{
  AutoLock lock(&m_objectCS);
  return m_grabTransparentWindows;
}

void ServerConfig::getAutoConnectHost(StringStorage *host)
{
  AutoLock l(this);

  *host = m_autoConnectHost;
}

void ServerConfig::setAutoConnectHost(const TCHAR *host)
{
  AutoLock l(this);

  m_autoConnectHost.setString(host);
}
