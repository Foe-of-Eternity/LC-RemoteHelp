// Copyright (C) 2009,2010,2011,2012 GlavSoft LLC.
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

#include "TvnServer.h"
#include "win-system/CurrentConsoleProcess.h"
#include "win-system/Environment.h"

#include "server-config-lib/Configurator.h"

#include "thread/GlobalMutex.h"

#include "tvnserver/resource.h"

#include "network/socket/WindowsSocket.h"

#include "util/StringTable.h"
#include "util/AnsiStringStorage.h"
#include "tvnserver-app/NamingDefs.h"

#include "file-lib/File.h"

#include "rfb/HostPath.h"
#include "tvnserver-app/OutgoingRfbConnectionThread.h"

#include "tvnserver/BuildTime.h"

#include <crtdbg.h>
#include <time.h>

TvnServer::TvnServer(LogInitListener *logInitListener,
                     Logger *logger)
: Singleton<TvnServer>(),
ListenerContainer<TvnServerListener *>(),
m_logInitListener(logInitListener),
m_rfbClientManager(0),
m_config(false),
m_log(logger)
{
  m_log.message(_T("%s Build on %s"),
    ProductNames::SERVER_PRODUCT_NAME,
    BuildTime::DATE);

  // Initialize configuration.
  // FIXME: It looks like configurator may be created as a member object.
  Configurator *configurator = Configurator::getInstance();
  configurator->load();
  m_srvConfig = Configurator::getInstance()->getServerConfig();

  try {
    StringStorage logDir;
    m_srvConfig->getLogFileDir(&logDir);
    unsigned char logLevel = m_srvConfig->getLogLevel();
    // FIXME: Use correct log name.
    m_logInitListener->onLogInit(logDir.getString(), LogNames::SERVER_LOG_FILE_STUB_NAME, logLevel);

  }
  catch (...) {
    // A log error must not be a reason that stop the server.
  }

  // Initialize windows sockets.

  m_log.info(_T("Initialize WinSock"));

  try {
    WindowsSocket::startup(2, 1);
  }
  catch (Exception &ex) {
    m_log.interror(_T("%s"), ex.getMessage());
  }

  DesktopFactory *desktopFactory = &m_applicationDesktopFactory;

  // Instanize zombie killer singleton.
  // FIXME: may be need to do it in another place or use "lazy" initialization.
  m_rfbClientManager = new RfbClientManager(0, &m_log, desktopFactory);

  m_rfbClientManager->addListener(this);

  // FIXME: No good to act as a listener before completing the object
  //        construction.
  Configurator::getInstance()->addListener(this);
}

TvnServer::~TvnServer()
{
  Configurator::getInstance()->removeListener(this);

  ZombieKiller *zombieKiller = ZombieKiller::getInstance();

  // Disconnect all zombies rfb, control clients though killing
  // their threads.
  zombieKiller->killAllZombies();

  m_rfbClientManager->removeListener(this);

  delete m_rfbClientManager;

  m_log.info(_T("Shutdown WinSock"));

  try {
    WindowsSocket::cleanup();
  } catch (Exception &ex) {
    m_log.error(_T("%s"), ex.getMessage());
  }
}

// Remark: this method can be called from other threads.
void TvnServer::onConfigReload(ServerConfig *serverConfig)
{
  changeLogProps();
}

void TvnServer::generateExternalShutdownSignal()
{
  AutoLock l(&m_listeners);

  vector<TvnServerListener *>::iterator it;
  for (it = m_listeners.begin(); it != m_listeners.end(); it++) {
    TvnServerListener *each = *it;

    each->onTvnServerShutdown();
  } // for all listeners.
}

void TvnServer::afterFirstClientConnect()
{
}

void TvnServer::afterLastClientDisconnect()
{
  generateExternalShutdownSignal();
}

void TvnServer::changeLogProps()
{
  StringStorage logDir;
  unsigned char logLevel;
  {
    AutoLock al(&m_mutex);
    m_srvConfig->getLogFileDir(&logDir);
    logLevel = m_srvConfig->getLogLevel();
  }
  m_logInitListener->onChangeLogProps(logDir.getString(), logLevel);
}

void TvnServer::doConnect()
{
  StringStorage autoConnectHost;
  m_srvConfig->getAutoConnectHost(&autoConnectHost);

  if (autoConnectHost.getLength() == 0) {
    throw Exception(_T("AutoConnectHost not specified."));
  }

  //
  // Parse host and port from connection string.
  //
  AnsiStringStorage connectStringAnsi(&autoConnectHost);
  HostPath hp(connectStringAnsi.getString(), 5500);

  if (!hp.isValid()) {
    throw Exception(_T("Invalid AutoConnectHost specified."));
  }

  StringStorage host;
  AnsiStringStorage ansiHost(hp.getVncHost());
  ansiHost.toStringStorage(&host);

  //
  // Make outgoing connection in separate thread.
  //
  OutgoingRfbConnectionThread *newConnectionThread =
    new OutgoingRfbConnectionThread(host.getString(),
    hp.getVncPort(), false,
    m_rfbClientManager, &m_log,
    this);

  newConnectionThread->resume();

  ZombieKiller::getInstance()->addZombie(newConnectionThread);
}