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

#ifndef _SERVER_CONFIG_H_
#define _SERVER_CONFIG_H_

#include "util/StringVector.h"
#include "util/Exception.h"
#include "thread/AutoLock.h"
#include "thread/LocalMutex.h"
#include "io-lib/DataInputStream.h"
#include "io-lib/DataOutputStream.h"
#include "io-lib/IOException.h"

#include <shlobj.h>

class ServerConfig : public Lockable
{
public:
  static const unsigned int MINIMAL_POLLING_INTERVAL = 30;
  static const unsigned int MINIMAL_LOCAL_INPUT_PRIORITY_TIMEOUT = 1;

public:
  ServerConfig();
  virtual ~ServerConfig();

  /**
   * Serializes server config to output stream as byte stream.
   * Thread-safe method.
   * @throws Exception on io error.
   * @fixme stub.
   */
  void serialize(DataOutputStream *output) throw(Exception);

  /**
   * Deserializes server config from input stream.
   * Thread-safe method.
   * @throws Exception on io error.
   * @fixme stub.
   */
  void deserialize(DataInputStream *input) throw(Exception);

  //
  // Inherited from Lockable abstract class.
  //

  virtual void lock() {
    m_objectCS.lock();
  }

  virtual void unlock() {
    m_objectCS.unlock();
  }

  //
  // Other server options access methods
  //

  void enableFileTransfers(bool enabled);
  bool isFileTransfersEnabled();

  void enableRemovingDesktopWallpaper(bool enabled);
  bool isRemovingDesktopWallpaperEnabled();

  bool getMirrorIsAllowed();
  void setMirrorAllowing(bool value);

  //
  // Configurator from Administration tab
  //

  int getLogLevel();

  void setLogLevel(int logLevel);

  //
  // Sharing configuration
  //

  bool isAlwaysShared();

  bool isNeverShared();

  bool isDisconnectingExistingClients();

  void setAlwaysShared(bool enabled);

  void setNeverShared(bool enabled);

  void disconnectExistingClients(bool disconnectExisting);

  void setPollingInterval(unsigned int interval);

  unsigned int getPollingInterval();

  //
  // Input handling config
  //

  void blockRemoteInput(bool blockEnabled);

  bool isBlockingRemoteInput();

  void setLocalInputPriority(bool localPriority);

  bool isLocalInputPriorityEnabled();

  unsigned int getLocalInputPriorityTimeout();

  void setLocalInputPriorityTimeout(unsigned int value);

  void blockLocalInput(bool enabled);

  bool isBlockingLocalInput();

  //
  // Other
  //

  void setAutoConnectHost(const TCHAR *host);
  void getAutoConnectHost(StringStorage *host);

  void setGrabTransparentWindowsFlag(bool grab);
  bool getGrabTransparentWindowsFlag();

  void getLogFileDir(StringStorage *logFileDir);
  void setLogFileDir(const TCHAR *logFileDir);
protected:

  //
  // Other server options members group
  //

  bool m_enableFileTransfers;
  bool m_removeWallpaper;
  bool m_mirrorDriverAllowed;

  //
  // Configurator from Administration tab
  //

  int m_logLevel;

  //
  // Sharing configuration
  //

  bool m_alwaysShared;
  bool m_neverShared;
  bool m_disconnectClients;

  //
  // Polling configuration
  //

  unsigned int m_pollingInterval;

  //
  // When flag is set server always blocks remote input.
  //

  bool m_blockRemoteInput;
  //
  // When flag is set server always blocks local input.
  //

  bool m_blockLocalInput;

  //
  // When flag is set server blocks remote input
  // on local input activity.
  //

  bool m_localInputPriority;

  //
  // Local input invactivity timeout during that
  // we still blocking remote input(when m_localInputPriority
  // is enabled).
  //

  unsigned int m_localInputPriorityTimeout;

  //
  // Other
  //

  StringStorage m_autoConnectHost;
  bool m_grabTransparentWindows;

  StringStorage m_logFilePath;
private:

  //
  // Critical section
  //

  LocalMutex m_objectCS;
};

#endif
