/* cproxy - Copyright (C) 2012 Aaron Riekenberg (aaron.riekenberg@gmail.com).

   cproxy is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   cproxy is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with cproxy.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "bufferpool.h"
#include "errutil.h"
#include "fdutil.h"
#include "linkedlist.h"
#include "log.h"
#include "memutil.h"
#include "pollutil.h"
#include "socketutil.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define DEFAULT_BUFFER_SIZE (16 * 1024)
#define DEFAULT_NO_DELAY_SETTING (false)
#define DEFAULT_NUM_IO_THREADS (1)
#define MAX_OPERATIONS_FOR_ONE_FD (100)
#define INITIAL_CONNECTION_SOCKET_INFO_POOL_SIZE (16)

static void printUsageAndExit()
{
  printf("Usage:\n");
  printf("  cproxy -l <local addr>:<local port>\n");
  printf("         [-l <local addr>:<local port>...]\n");
  printf("         -r <remote addr>:<remote port>\n");
  printf("         [-b <buf size>] [-n] [-t <num io threads>]\n");
  printf("Arguments:\n");
  printf("  -l <local addr>:<local port>: specify listen address and port\n");
  printf("  -r <remote addr>:<remote port>: specify remote address and port\n");
  printf("  -b <buf size>: specify session buffer size in bytes\n");
  printf("  -n: enable TCP no delay\n");
  printf("  -t: <num io threads>: specify number of I/O threads\n");
  exit(1);
}

static int parseBufferSize(
  const char* optarg)
{
  int bufferSize = atoi(optarg);
  if (bufferSize <= 0)
  {
    proxyLog("invalid buffer size %s", optarg);
    exit(1);
  }
  return bufferSize;
}

static int parseNumIOThreads(
  const char* optarg)
{
  int numIOThreads = atoi(optarg);
  if (numIOThreads <= 0)
  {
    proxyLog("invalid num io threads %s", optarg);
    exit(1);
  }
  return numIOThreads;
}

static struct addrinfo* parseAddrPort(
  const char* optarg)
{
  struct addrinfo hints;
  struct addrinfo* addressInfo = NULL;
  char addressString[NI_MAXHOST];
  char portString[NI_MAXSERV];
  const size_t optargLen = strlen(optarg);
  size_t colonIndex;
  size_t hostLength;
  size_t portLength;
  int retVal;

  colonIndex = optargLen;
  while (colonIndex > 0)
  {
    --colonIndex;
    if (optarg[colonIndex] == ':')
    {
      break;
    }
  }

  if ((colonIndex <= 0) ||
      (colonIndex >= (optargLen - 1)))
  {
    proxyLog("invalid address:port argument: '%s'", optarg);
    exit(1);
  }

  hostLength = colonIndex;
  portLength = optargLen - colonIndex - 1;

  if ((hostLength >= NI_MAXHOST) ||
      (portLength >= NI_MAXSERV))
  {
    proxyLog("invalid address:port argument: '%s'", optarg);
    exit(1);
  }

  strncpy(addressString, optarg, hostLength);
  addressString[hostLength] = 0;

  strncpy(portString, &(optarg[colonIndex + 1]), portLength);
  portString[portLength] = 0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_ADDRCONFIG;

  if (((retVal = getaddrinfo(addressString, portString, 
                             &hints, &addressInfo)) != 0) ||
      (addressInfo == NULL))
  {
    proxyLog("error resolving address %s %s",
             optarg, gai_strerror(retVal));
    exit(1);
  }

  return addressInfo;
}

static struct addrinfo* parseRemoteAddrPort(
  const char* optarg,
  struct AddrPortStrings* addrPortStrings)
{
  struct addrinfo* addressInfo = parseAddrPort(optarg);
  
  if (addressToNameAndPort(
        addressInfo->ai_addr,
        addressInfo->ai_addrlen,
        addrPortStrings) < 0)
  {
    exit(1);
  }

  return addressInfo;
}

struct ProxySettings
{
  size_t bufferSize;
  bool noDelay;
  size_t numIOThreads;
  struct LinkedList serverAddrInfoList;
  struct addrinfo* remoteAddrInfo;
  struct AddrPortStrings remoteAddrPortStrings;
};

static const struct ProxySettings* processArgs(
  int argc,
  char** argv)
{
  int retVal;
  bool foundLocalAddress = false;
  bool foundRemoteAddress = false;
  struct ProxySettings* proxySettings = 
    checkedCalloc(1, sizeof(struct ProxySettings));
  proxySettings->bufferSize = DEFAULT_BUFFER_SIZE;
  proxySettings->noDelay = DEFAULT_NO_DELAY_SETTING;
  proxySettings->numIOThreads = DEFAULT_NUM_IO_THREADS;
  initializeLinkedList(&(proxySettings->serverAddrInfoList));

  do
  {
    retVal = getopt(argc, argv, "b:l:nr:t:");
    switch (retVal)
    {
    case 'b':
      proxySettings->bufferSize = parseBufferSize(optarg);
      break;

    case 'l':
      addToLinkedList(&(proxySettings->serverAddrInfoList),
                      parseAddrPort(optarg));
      foundLocalAddress = true;
      break;

    case 'n':
      proxySettings->noDelay = true;
      break;

    case 'r':
      if (foundRemoteAddress)
      {
        printUsageAndExit();
      }
      proxySettings->remoteAddrInfo =
        parseRemoteAddrPort(
          optarg,
          &(proxySettings->remoteAddrPortStrings));
      foundRemoteAddress = true;
      break;

    case 't':
      proxySettings->numIOThreads = parseNumIOThreads(optarg);
      break;

    case '?':
      printUsageAndExit();
      break;
    }
  }
  while (retVal != -1);

  if ((!foundLocalAddress) || (!foundRemoteAddress))
  {
    printUsageAndExit();
  }

  return proxySettings;
}

static void setupSignals()
{
  struct sigaction newAction;
  memset(&newAction, 0, sizeof(struct sigaction));
  newAction.sa_handler = SIG_IGN;
  if (sigaction(SIGPIPE, &newAction, NULL) < 0)
  {
    proxyLog("sigaction error errno = %d", errno);
    exit(1);
  }
}

struct ServerSocketInfo
{
  int socket;
};

enum ConnectionSocketInfoType
{
  CLIENT_TO_PROXY,
  PROXY_TO_REMOTE
};

struct ConnectionSocketInfo
{
  int socket;
  enum ConnectionSocketInfoType type;
  size_t waitingToWriteBufferOffset;
  size_t waitingToWriteBufferSize;
  size_t waitingToWriteBufferCapacity;
  bool disconnectWhenWriteFinishes;
  bool waitingForConnect;
  bool waitingForRead;
  bool waitingForWrite;
  struct ConnectionSocketInfo* relatedConnectionSocketInfo;
  struct AddrPortStrings clientAddrPortStrings;
  struct AddrPortStrings serverAddrPortStrings;
  unsigned char waitingToWriteBuffer[];
};

static void addConnectionSocketInfoToPollState(
  struct PollState* pollState,
  struct ConnectionSocketInfo* connectionSocketInfo)
{
  addPollFDToPollState(
    pollState,
    connectionSocketInfo->socket,
    connectionSocketInfo,
    (connectionSocketInfo->waitingForRead ?
     INTERESTED_IN_READ_EVENTS :
     NOT_INTERESTED_IN_READ_EVENTS),
    ((connectionSocketInfo->waitingForConnect || 
      connectionSocketInfo->waitingForWrite) ?
     INTERESTED_IN_WRITE_EVENTS :
     NOT_INTERESTED_IN_WRITE_EVENTS));
}

static void updatePollStateForConnectionSocketInfo(
  struct PollState* pollState,
  struct ConnectionSocketInfo* connectionSocketInfo)
{
  updatePollFDInPollState(
    pollState,
    connectionSocketInfo->socket,
    connectionSocketInfo,
    (connectionSocketInfo->waitingForRead ?
     INTERESTED_IN_READ_EVENTS :
     NOT_INTERESTED_IN_READ_EVENTS),
    ((connectionSocketInfo->waitingForConnect || 
      connectionSocketInfo->waitingForWrite) ?
     INTERESTED_IN_WRITE_EVENTS :
     NOT_INTERESTED_IN_WRITE_EVENTS));
}

static void setupServerSockets(
  const struct LinkedList* serverAddrInfoList,
  struct PollState* pollState)
{
  struct LinkedListNode* nodePtr;

  for (nodePtr = serverAddrInfoList->head;
       nodePtr;
       nodePtr = nodePtr->next)
  {
    struct addrinfo* listenAddrInfo = nodePtr->data;
    struct AddrPortStrings serverAddrPortStrings;
    struct ServerSocketInfo* serverSocketInfo =
      checkedMalloc(sizeof(struct ServerSocketInfo));

    if (addressToNameAndPort(listenAddrInfo->ai_addr,
                             listenAddrInfo->ai_addrlen,
                             &serverAddrPortStrings) < 0)
    {
      exit(1);
    }

    serverSocketInfo->socket = socket(listenAddrInfo->ai_family,
                                      listenAddrInfo->ai_socktype,
                                      listenAddrInfo->ai_protocol);
    if (serverSocketInfo->socket < 0)
    {
      proxyLog("error creating server socket %s:%s",
               serverAddrPortStrings.addrString,
               serverAddrPortStrings.portString);
      exit(1);
    }

    if (setSocketReuseAddress(serverSocketInfo->socket) < 0)
    {
      proxyLog("setSocketReuseAddress error on server socket %s:%s",
               serverAddrPortStrings.addrString,
               serverAddrPortStrings.portString);
      exit(1);
    }

    if (bind(serverSocketInfo->socket,
             listenAddrInfo->ai_addr,
             listenAddrInfo->ai_addrlen) < 0)
    {
      proxyLog("bind error on server socket %s:%s",
               serverAddrPortStrings.addrString,
               serverAddrPortStrings.portString);
      exit(1);
    }

    if (setSocketListening(serverSocketInfo->socket) < 0)
    {
      proxyLog("listen error on server socket %s:%s",
               serverAddrPortStrings.addrString,
               serverAddrPortStrings.portString);
      exit(1);
    }

    if (setFDNonBlocking(serverSocketInfo->socket) < 0)
    {
      proxyLog("error setting non-blocking on server socket %s:%s",
               serverAddrPortStrings.addrString,
               serverAddrPortStrings.portString);
      exit(1);
    }

    proxyLog("listening on %s:%s (fd=%d)", 
             serverAddrPortStrings.addrString,
             serverAddrPortStrings.portString,
             serverSocketInfo->socket);

    addPollFDToPollState(
      pollState,
      serverSocketInfo->socket,
      serverSocketInfo,
      INTERESTED_IN_READ_EVENTS,
      NOT_INTERESTED_IN_WRITE_EVENTS);
  }
}

static bool setupClientSocket(
  int clientSocket,
  const struct ProxySettings* proxySettings,
  struct AddrPortStrings* clientAddrPortStrings,
  struct AddrPortStrings* proxyServerAddrPortStrings)
{
  struct sockaddr_storage clientAddress;
  socklen_t clientAddressSize;
  struct sockaddr_storage proxyServerAddress;
  socklen_t proxyServerAddressSize;

  if (setFDNonBlocking(clientSocket) < 0)
  {
    proxyLog("error setting non-blocking on accepted socket");
    return false;
  }

  if ((proxySettings->noDelay) && (setSocketNoDelay(clientSocket) < 0))
  {
    proxyLog("error setting no delay on accepted socket");
    return false;
  }

  clientAddressSize = sizeof(clientAddress);
  if (getpeername(
        clientSocket, 
        (struct sockaddr*)&clientAddress,
        &clientAddressSize) < 0)
  {
    proxyLog("getpeername error errno = %d", errno);
    return false;
  }

  if (addressToNameAndPort((struct sockaddr*)&clientAddress,
                           clientAddressSize,
                           clientAddrPortStrings) < 0)
  {
    proxyLog("error getting client address name and port");
    return false;
  }

  proxyServerAddressSize = sizeof(proxyServerAddress);
  if (getsockname(
        clientSocket, 
        (struct sockaddr*)&proxyServerAddress,
        &proxyServerAddressSize) < 0)
  {
    proxyLog("getsockname error errno = %d", errno);
    return false;
  }

  if (addressToNameAndPort((struct sockaddr*)&proxyServerAddress,
                           proxyServerAddressSize,
                           proxyServerAddrPortStrings) < 0)
  {
    proxyLog("error getting proxy server address name and port");
    return false;
  }

  proxyLog("connect client to proxy %s:%s -> %s:%s (fd=%d)",
           clientAddrPortStrings->addrString,
           clientAddrPortStrings->portString,
           proxyServerAddrPortStrings->addrString,
           proxyServerAddrPortStrings->portString,
           clientSocket);

  return true;
}

enum RemoteSocketStatus
{
  REMOTE_SOCKET_ERROR,
  REMOTE_SOCKET_CONNECTED,
  REMOTE_SOCKET_IN_PROGRESS
};

struct RemoteSocketResult
{
  enum RemoteSocketStatus status;
  int remoteSocket;
};

static struct RemoteSocketResult createRemoteSocket(
  const struct ProxySettings* proxySettings,
  struct AddrPortStrings* proxyClientAddrPortStrings)
{
  int connectRetVal;
  struct sockaddr_storage proxyClientAddress;
  socklen_t proxyClientAddressSize;
  struct RemoteSocketResult result =
  {
    .status = REMOTE_SOCKET_ERROR,
    .remoteSocket =
       socket(proxySettings->remoteAddrInfo->ai_family,
              proxySettings->remoteAddrInfo->ai_socktype,
              proxySettings->remoteAddrInfo->ai_protocol)
  };
  if (result.remoteSocket < 0)
  {
    proxyLog("error creating remote socket errno = %d", errno);
    result.status = REMOTE_SOCKET_ERROR;
    result.remoteSocket = -1;
    return result;
  }

  if (setFDNonBlocking(result.remoteSocket) < 0)
  {
    proxyLog("error setting non-blocking on remote socket");
    signalSafeClose(result.remoteSocket);
    result.status = REMOTE_SOCKET_ERROR;
    result.remoteSocket = -1;
    return result;
  }

  connectRetVal = connect(
    result.remoteSocket,
    proxySettings->remoteAddrInfo->ai_addr,
    proxySettings->remoteAddrInfo->ai_addrlen);
  if ((connectRetVal < 0) &&
      ((errno == EINPROGRESS) ||
       (errno == EINTR)))
  {
    result.status = REMOTE_SOCKET_IN_PROGRESS;
  }
  else if (connectRetVal < 0)
  {
    char* socketErrorString = errnoToString(errno);
    proxyLog("remote socket connect error errno = %d: %s",
             errno, socketErrorString);
    free(socketErrorString);
    signalSafeClose(result.remoteSocket);
    result.status = REMOTE_SOCKET_ERROR;
    result.remoteSocket = -1;
    return result;
  }
  else
  {
    result.status = REMOTE_SOCKET_CONNECTED;
  }

  if ((proxySettings->noDelay) && (setSocketNoDelay(result.remoteSocket) < 0))
  {
    proxyLog("error setting no delay on remote socket");
    signalSafeClose(result.remoteSocket);
    result.status = REMOTE_SOCKET_ERROR;
    result.remoteSocket = -1;
    return result;
  }

  proxyClientAddressSize = sizeof(proxyClientAddress);
  if (getsockname(
        result.remoteSocket,
        (struct sockaddr*)&proxyClientAddress,
        &proxyClientAddressSize) < 0)
  {
    proxyLog("getsockname error errno = %d", errno);
    signalSafeClose(result.remoteSocket);
    result.status = REMOTE_SOCKET_ERROR;
    result.remoteSocket = -1;
    return result;
  }

  if (addressToNameAndPort((struct sockaddr*)&proxyClientAddress,
                           proxyClientAddressSize,
                           proxyClientAddrPortStrings) < 0)
  {
    proxyLog("error getting proxy client address name and port");
    signalSafeClose(result.remoteSocket);
    result.status = REMOTE_SOCKET_ERROR;
    result.remoteSocket = -1;
    return result;
  }

  if (result.status != REMOTE_SOCKET_ERROR)
  {
    proxyLog("connect %s proxy to remote %s:%s -> %s:%s (fd=%d)",
             ((result.status == REMOTE_SOCKET_CONNECTED) ? "complete" : "starting"),
             proxyClientAddrPortStrings->addrString,
             proxyClientAddrPortStrings->portString,
             proxySettings->remoteAddrPortStrings.addrString,
             proxySettings->remoteAddrPortStrings.portString,
             result.remoteSocket);
  }

  return result;
}

static void handleNewClientSocket(
  int clientSocket,
  const struct ProxySettings* proxySettings,
  struct PollState* pollState,
  struct BufferPool* connectionSocketInfoPool)
{
  struct AddrPortStrings clientAddrPortStrings;
  struct AddrPortStrings proxyServerAddrPortStrings;
  struct AddrPortStrings proxyClientAddrPortStrings;
  if (!setupClientSocket(
        clientSocket,
        proxySettings,
        &clientAddrPortStrings,
        &proxyServerAddrPortStrings))
  {
    signalSafeClose(clientSocket);
  }
  else
  {
    const struct RemoteSocketResult remoteSocketResult =
      createRemoteSocket(proxySettings,
                         &proxyClientAddrPortStrings);
    if (remoteSocketResult.status == REMOTE_SOCKET_ERROR)
    {
      signalSafeClose(clientSocket);
    }
    else
    {
      struct ConnectionSocketInfo* connInfo1;
      struct ConnectionSocketInfo* connInfo2;

      connInfo1 = getBufferFromBufferPool(connectionSocketInfoPool);
      connInfo1->socket = clientSocket;
      connInfo1->type = CLIENT_TO_PROXY;
      connInfo1->waitingToWriteBufferOffset = 0;
      connInfo1->waitingToWriteBufferSize = 0;
      connInfo1->waitingToWriteBufferCapacity = proxySettings->bufferSize;
      connInfo1->disconnectWhenWriteFinishes = false;
      if (remoteSocketResult.status == REMOTE_SOCKET_CONNECTED)
      {
        connInfo1->waitingForConnect = false;
        connInfo1->waitingForRead = true;
        connInfo1->waitingForWrite = false;
      }
      else if (remoteSocketResult.status == REMOTE_SOCKET_IN_PROGRESS)
      {
        connInfo1->waitingForConnect = false;
        connInfo1->waitingForRead = false;
        connInfo1->waitingForWrite = false;
      }
      memcpy(&(connInfo1->clientAddrPortStrings),
             &clientAddrPortStrings,
             sizeof(struct AddrPortStrings));
      memcpy(&(connInfo1->serverAddrPortStrings),
             &proxyServerAddrPortStrings,
             sizeof(struct AddrPortStrings));

      connInfo2 = getBufferFromBufferPool(connectionSocketInfoPool);
      connInfo2->socket = remoteSocketResult.remoteSocket;
      connInfo2->type = PROXY_TO_REMOTE;
      connInfo2->waitingToWriteBufferOffset = 0;
      connInfo2->waitingToWriteBufferSize = 0;
      connInfo2->waitingToWriteBufferCapacity = proxySettings->bufferSize;
      connInfo2->disconnectWhenWriteFinishes = false;
      if (remoteSocketResult.status == REMOTE_SOCKET_CONNECTED)
      {
        connInfo2->waitingForConnect = false;
        connInfo2->waitingForRead = true;
        connInfo2->waitingForWrite = false;
      }
      else if (remoteSocketResult.status == REMOTE_SOCKET_IN_PROGRESS)
      {
        connInfo2->waitingForConnect = true;
        connInfo2->waitingForRead = false;
        connInfo2->waitingForWrite = false;
      }
      memcpy(&(connInfo2->clientAddrPortStrings),
             &proxyClientAddrPortStrings,
             sizeof(struct AddrPortStrings));
      memcpy(&(connInfo2->serverAddrPortStrings),
             &(proxySettings->remoteAddrPortStrings),
             sizeof(struct AddrPortStrings));

      connInfo1->relatedConnectionSocketInfo = connInfo2;
      connInfo2->relatedConnectionSocketInfo = connInfo1;

      addConnectionSocketInfoToPollState(pollState, connInfo1);
      addConnectionSocketInfoToPollState(pollState, connInfo2);
    }
  }
}

static void printDisconnectMessage(
  const struct ConnectionSocketInfo* connectionSocketInfo)
{
  proxyLog("disconnect %s %s:%s -> %s:%s (fd=%d)",
           ((connectionSocketInfo->type == CLIENT_TO_PROXY) ?
            "client to proxy" :
            "proxy to remote"),
           connectionSocketInfo->clientAddrPortStrings.addrString,
           connectionSocketInfo->clientAddrPortStrings.portString,
           connectionSocketInfo->serverAddrPortStrings.addrString,
           connectionSocketInfo->serverAddrPortStrings.portString,
           connectionSocketInfo->socket);
}

static void destroyConnection(
  struct ConnectionSocketInfo* connectionSocketInfo,
  struct PollState* pollState,
  struct BufferPool* connectionSocketInfoPool)
{
  int socket = connectionSocketInfo->socket;
  struct ConnectionSocketInfo* relatedConnectionSocketInfo =
    connectionSocketInfo->relatedConnectionSocketInfo;

  printDisconnectMessage(connectionSocketInfo);
  returnBufferToBufferPool(connectionSocketInfoPool, connectionSocketInfo);
  removePollFDFromPollState(pollState, socket);
  signalSafeClose(socket);

  if (relatedConnectionSocketInfo)
  {
    relatedConnectionSocketInfo->relatedConnectionSocketInfo = NULL;
    if (relatedConnectionSocketInfo->waitingForWrite)
    {
      relatedConnectionSocketInfo->disconnectWhenWriteFinishes = true;
      relatedConnectionSocketInfo->waitingForRead = false;
      updatePollStateForConnectionSocketInfo(
        pollState, relatedConnectionSocketInfo);
    }
    else
    {
      destroyConnection(
        relatedConnectionSocketInfo,
        pollState,
        connectionSocketInfoPool);
    }
  }
}

static struct ConnectionSocketInfo* handleConnectionReadyForError(
  struct ConnectionSocketInfo* connectionSocketInfo)
{
  struct ConnectionSocketInfo* pDisconnectSocketInfo = NULL;
  int socketError = getSocketError(connectionSocketInfo->socket);
  if (socketError != 0)
  {
    char* socketErrorString = errnoToString(socketError);

    pDisconnectSocketInfo = connectionSocketInfo;

    proxyLog("fd %d errno %d: %s",
             connectionSocketInfo->socket,
             socketError,
             socketErrorString);

    free(socketErrorString);
  }

  return pDisconnectSocketInfo;
}

static struct ConnectionSocketInfo* handleConnectionReadyForRead(
  struct ConnectionSocketInfo* connectionSocketInfo,
  struct PollState* pollState)
{
  struct ConnectionSocketInfo* pDisconnectSocketInfo = NULL;
  struct ConnectionSocketInfo* relatedConnectionSocketInfo =
    connectionSocketInfo->relatedConnectionSocketInfo;

  if (connectionSocketInfo->waitingForRead)
  {
    bool readWouldBlock = false;
    bool writeWouldBlock = false;
    int numReads = 0;

    assert(relatedConnectionSocketInfo != NULL);

    while ((!pDisconnectSocketInfo) &&
           (!readWouldBlock) &&
           (!writeWouldBlock) &&
           (numReads < MAX_OPERATIONS_FOR_ONE_FD))
    {
      const struct ReadFromFDResult readResult = readFromFD(
        connectionSocketInfo->socket,
        relatedConnectionSocketInfo->waitingToWriteBuffer,
        relatedConnectionSocketInfo->waitingToWriteBufferCapacity);
      ++numReads;
      if (readResult.status == READ_FROM_FD_WOULD_BLOCK)
      {
        readWouldBlock = true;
      }
      else if ((readResult.status == READ_FROM_FD_ERROR) ||
               (readResult.status == READ_FROM_FD_EOF))
      {
        pDisconnectSocketInfo = connectionSocketInfo;
      }
      else
      {
        relatedConnectionSocketInfo->waitingToWriteBufferOffset = 0;
        relatedConnectionSocketInfo->waitingToWriteBufferSize =
          readResult.bytesRead;
        while ((relatedConnectionSocketInfo->waitingToWriteBufferSize > 
                relatedConnectionSocketInfo->waitingToWriteBufferOffset) &&
               (!pDisconnectSocketInfo) &&
               (!writeWouldBlock))
        {
          const struct WriteToFDResult writeResult = writeToFD(
            relatedConnectionSocketInfo->socket,
            &(relatedConnectionSocketInfo->waitingToWriteBuffer[
              relatedConnectionSocketInfo->waitingToWriteBufferOffset]),
            relatedConnectionSocketInfo->waitingToWriteBufferSize -
            relatedConnectionSocketInfo->waitingToWriteBufferOffset);
          if (writeResult.status == WRITE_TO_FD_WOULD_BLOCK)
          {
            relatedConnectionSocketInfo->waitingForWrite = true;
            updatePollStateForConnectionSocketInfo(pollState, relatedConnectionSocketInfo);
            connectionSocketInfo->waitingForRead = false;
            updatePollStateForConnectionSocketInfo(pollState, connectionSocketInfo);
            writeWouldBlock = true;
          }
          else if (writeResult.status == WRITE_TO_FD_ERROR)
          {
            pDisconnectSocketInfo = relatedConnectionSocketInfo;
          }
          else
          {
            relatedConnectionSocketInfo->waitingToWriteBufferOffset +=
              writeResult.bytesWritten;
          }
        }
      }
    }
  }

  return pDisconnectSocketInfo;
}

static struct ConnectionSocketInfo* handleConnectionReadyForWrite(
  struct ConnectionSocketInfo* connectionSocketInfo,
  struct PollState* pollState)
{
  struct ConnectionSocketInfo* pDisconnectSocketInfo = NULL;
  struct ConnectionSocketInfo* relatedConnectionSocketInfo =
    connectionSocketInfo->relatedConnectionSocketInfo;

  if (connectionSocketInfo->waitingForConnect)
  {
    int socketError;

    assert(relatedConnectionSocketInfo != NULL);

    socketError = getSocketError(connectionSocketInfo->socket);
    if (socketError == 0)
    {
      proxyLog("connect complete proxy to remote %s:%s -> %s:%s (fd=%d)",
               connectionSocketInfo->clientAddrPortStrings.addrString,
               connectionSocketInfo->clientAddrPortStrings.portString,
               connectionSocketInfo->serverAddrPortStrings.addrString,
               connectionSocketInfo->serverAddrPortStrings.portString,
               connectionSocketInfo->socket);
      connectionSocketInfo->waitingForConnect = false;
      connectionSocketInfo->waitingForRead = true;
      updatePollStateForConnectionSocketInfo(pollState, connectionSocketInfo);
      relatedConnectionSocketInfo->waitingForRead = true;
      updatePollStateForConnectionSocketInfo(pollState, relatedConnectionSocketInfo);
    }
    else if (socketError == EINPROGRESS)
    {
      /* do nothing, still in progress */
    }
    else
    {
      char* socketErrorString = errnoToString(socketError);
      proxyLog("async remote connect fd %d errno %d: %s",
               connectionSocketInfo->socket,
               socketError,
               socketErrorString);
      free(socketErrorString);
      pDisconnectSocketInfo = connectionSocketInfo;
    }
  }

  else if (connectionSocketInfo->waitingForWrite)
  {
    bool writeWouldBlock = false;
    while ((connectionSocketInfo->waitingToWriteBufferSize > 
            connectionSocketInfo->waitingToWriteBufferOffset) &&
           (!pDisconnectSocketInfo) &&
           (!writeWouldBlock))
    {
      const struct WriteToFDResult writeResult = writeToFD(
        connectionSocketInfo->socket,
        &(connectionSocketInfo->waitingToWriteBuffer[
           connectionSocketInfo->waitingToWriteBufferOffset]),
        connectionSocketInfo->waitingToWriteBufferSize -
        connectionSocketInfo->waitingToWriteBufferOffset);
      if (writeResult.status == WRITE_TO_FD_WOULD_BLOCK)
      {
        writeWouldBlock = true;
      }
      else if (writeResult.status == WRITE_TO_FD_ERROR)
      {
        pDisconnectSocketInfo = connectionSocketInfo;
      }
      else
      {
        connectionSocketInfo->waitingToWriteBufferOffset +=
          writeResult.bytesWritten;
      }
    }
    if ((!pDisconnectSocketInfo) && (!writeWouldBlock))
    {
      if (connectionSocketInfo->disconnectWhenWriteFinishes)
      {
        pDisconnectSocketInfo = connectionSocketInfo;
      }
      else
      {
        assert(relatedConnectionSocketInfo != NULL);

        connectionSocketInfo->waitingToWriteBufferOffset = 0;
        connectionSocketInfo->waitingToWriteBufferSize = 0;
        connectionSocketInfo->waitingForWrite = false;
        updatePollStateForConnectionSocketInfo(pollState, connectionSocketInfo);
        relatedConnectionSocketInfo->waitingForRead = true;
        updatePollStateForConnectionSocketInfo(pollState, relatedConnectionSocketInfo);
      }
    }
  }

  return pDisconnectSocketInfo;
}

enum HandleConnectionReadyResult
{
  POLL_STATE_INVALIDATED_RESULT,
  POLL_STATE_NOT_INVALIDATED_RESULT
};

static enum HandleConnectionReadyResult handleConnectionReady(
  const struct ReadyFDInfo* readyFDInfo,
  struct ConnectionSocketInfo* connectionSocketInfo,
  struct PollState* pollState,
  struct BufferPool* connectionSocketInfoPool)
{
  enum HandleConnectionReadyResult handleConnectionReadyResult =
    POLL_STATE_NOT_INVALIDATED_RESULT;
  struct ConnectionSocketInfo* pDisconnectSocketInfo = NULL;

#ifdef DEBUG_PROXY
  proxyLog("fd %d readyForRead %d readyForWrite %d readyForError %d",
           connectionSocketInfo->socket,
           readyFDInfo->readyForRead,
           readyFDInfo->readyForWrite,
           readyFDInfo->readyForError);
#endif

  if (readyFDInfo->readyForError &&
      (!pDisconnectSocketInfo))
  {
    pDisconnectSocketInfo = 
      handleConnectionReadyForError(
        connectionSocketInfo);
  }

  if (readyFDInfo->readyForRead &&
      (!pDisconnectSocketInfo))
  {
    pDisconnectSocketInfo =
      handleConnectionReadyForRead(
        connectionSocketInfo,
        pollState);
  }

  if (readyFDInfo->readyForWrite &&
      (!pDisconnectSocketInfo))
  {
    pDisconnectSocketInfo =
      handleConnectionReadyForWrite(
        connectionSocketInfo,
        pollState);
  }

  if (pDisconnectSocketInfo)
  {
    destroyConnection(
      pDisconnectSocketInfo,
      pollState,
      connectionSocketInfoPool);
    handleConnectionReadyResult = POLL_STATE_INVALIDATED_RESULT;
  }

  return handleConnectionReadyResult;
}

struct IOThreadReceiveFDInfo
{
  int addClientMessageFD;
  int receivedFD;
  size_t receiveIndex;
};

static void handleAddClientMessageFDReady(
  const struct ProxySettings* proxySettings,
  struct IOThreadReceiveFDInfo* pIOThreadReceiveFDInfo,
  struct PollState* pollState,
  struct BufferPool* connectionSocketInfoPool)
{
  bool readWouldBlock = false;
  unsigned char* pCharBuffer =
    (unsigned char*)(&(pIOThreadReceiveFDInfo->receivedFD));

  do
  {
    size_t bytesToRead = sizeof(int) - pIOThreadReceiveFDInfo->receiveIndex;
    const struct ReadFromFDResult readResult = readFromFD(
      pIOThreadReceiveFDInfo->addClientMessageFD,
      &(pCharBuffer[pIOThreadReceiveFDInfo->receiveIndex]),
      bytesToRead);
    if (readResult.status == READ_FROM_FD_WOULD_BLOCK)
    {
      readWouldBlock = true;
    }
    else if (readResult.status == READ_FROM_FD_ERROR)
    {
      proxyLog("read error %d from pipe fd %d",
               readResult.readErrno,
               pIOThreadReceiveFDInfo->addClientMessageFD);
      abort();
    }
    else if (readResult.status == READ_FROM_FD_EOF)
    {
      proxyLog("read EOF from pipe fd %d",
               pIOThreadReceiveFDInfo->addClientMessageFD);
      abort();
    }
    else
    {
      pIOThreadReceiveFDInfo->receiveIndex += readResult.bytesRead;
      bytesToRead -= readResult.bytesRead;
    }

    if (bytesToRead == 0)
    {
      handleNewClientSocket(
        pIOThreadReceiveFDInfo->receivedFD,
        proxySettings,
        pollState,
        connectionSocketInfoPool);
      pIOThreadReceiveFDInfo->receivedFD = 0;
      pIOThreadReceiveFDInfo->receiveIndex = 0;
    }
  } while (!readWouldBlock);
}

struct IOThreadCreateMessage
{
  int ioThreadNumber;
  int addClientMessageFD;
  const struct ProxySettings* proxySettings;
};

static void setIOThreadName(int ioThreadNumber)
{
  char threadNameBuffer[80];

  snprintf(threadNameBuffer, 80, "io-%d", ioThreadNumber);
  proxyLogSetThreadName(threadNameBuffer);
}

static void* runIOThread(void* param)
{
  struct IOThreadCreateMessage* pIOThreadCreateMessage = param;
  const struct ProxySettings* proxySettings =
    pIOThreadCreateMessage->proxySettings;
  struct IOThreadReceiveFDInfo ioThreadReceiveFDInfo;
  struct PollState pollState;
  struct BufferPool connectionSocketInfoPool;

  setIOThreadName(pIOThreadCreateMessage->ioThreadNumber);

  memset(&ioThreadReceiveFDInfo, 0, sizeof(ioThreadReceiveFDInfo));
  ioThreadReceiveFDInfo.addClientMessageFD =
    pIOThreadCreateMessage->addClientMessageFD;

  free(pIOThreadCreateMessage);
  pIOThreadCreateMessage = NULL;
  param = NULL;

  initializePollState(&pollState);

  if (setFDNonBlocking(ioThreadReceiveFDInfo.addClientMessageFD) < 0)
  {
    proxyLog("error setting addClientMessageFD non blocking");
    abort();
  }

  addPollFDToPollState(
    &pollState, 
    ioThreadReceiveFDInfo.addClientMessageFD,
    NULL,
    INTERESTED_IN_READ_EVENTS,
    NOT_INTERESTED_IN_WRITE_EVENTS);

  initializeBufferPool(
    &connectionSocketInfoPool,
    sizeof(struct ConnectionSocketInfo) + proxySettings->bufferSize,
    INITIAL_CONNECTION_SOCKET_INFO_POOL_SIZE);

  while (true)
  {
    size_t i;
    bool pollStateInvalidated = false;
    const struct PollResult* pollResult = blockingPoll(&pollState);
    if (!pollResult)
    {
      proxyLog("blockingPoll failed");
      abort();
    }

    for (i = 0; 
         (!pollStateInvalidated) &&
         (i < pollResult->numReadyFDs);
         ++i)
    {
      struct ReadyFDInfo* readyFDInfo =
        &(pollResult->readyFDInfoArray[i]);
      if (!(readyFDInfo->data))
      {
        handleAddClientMessageFDReady(
          proxySettings,
          &ioThreadReceiveFDInfo,
          &pollState,
          &connectionSocketInfoPool);
      }
      else
      {
        struct ConnectionSocketInfo* connectionSocketInfo =
          readyFDInfo->data;
        if (handleConnectionReady(
              readyFDInfo,
              connectionSocketInfo,
              &pollState,
              &connectionSocketInfoPool) == POLL_STATE_INVALIDATED_RESULT)
        {
          pollStateInvalidated = true;
        }
      }
    }
  }

  return NULL;
}

struct IOThreadPipeInfo
{
  int readFD;
  int writeFD;
};

static struct IOThreadPipeInfo* createIOThreadPipes(
  size_t numPipes)
{
  size_t i;
  int pipeFDs[2]; 
  struct IOThreadPipeInfo* ioThreadPipeInfoArray;

  ioThreadPipeInfoArray =
    checkedCalloc(numPipes, sizeof(struct IOThreadPipeInfo));
  for (i = 0; i < numPipes; ++i)
  {
    if (pipe(pipeFDs) < 0)
    {
      proxyLog("pipe error errno = %d", errno);
      abort();
    }
    ioThreadPipeInfoArray[i].readFD = pipeFDs[0];
    ioThreadPipeInfoArray[i].writeFD = pipeFDs[1];
  }
  return ioThreadPipeInfoArray;
}

static void startIOThreads(
  const struct ProxySettings* proxySettings,
  const struct IOThreadPipeInfo* ioThreadPipeInfoArray,
  struct LinkedList* pthreadList)
{
  size_t i;
  struct IOThreadCreateMessage* pIOThreadCreateMessage;
  pthread_t* pPthread;
  int pthreadRetVal;

  for (i = 0; i < proxySettings->numIOThreads; ++i)
  {
    pIOThreadCreateMessage =
      checkedMalloc(sizeof(struct IOThreadCreateMessage));
    pIOThreadCreateMessage->ioThreadNumber = i;
    pIOThreadCreateMessage->addClientMessageFD = ioThreadPipeInfoArray[i].readFD;
    pIOThreadCreateMessage->proxySettings = proxySettings;
    pPthread = checkedMalloc(sizeof(pthread_t));

    pthreadRetVal =
      pthread_create(
        pPthread, NULL,
        &runIOThread, pIOThreadCreateMessage);
    if (pthreadRetVal != 0)
    {
      proxyLog("pthread_create error %d", pthreadRetVal);
      abort();
    }

    addToLinkedList(pthreadList, pPthread);
  }
}

static void writeAcceptedFDToIOThread(
  const struct ProxySettings* proxySettings,
  const int* ioThreadPipeWriteFDs,
  size_t* nextPipeWriteFDIndex,
  const int acceptedFD)
{
  const unsigned char* pCharBuffer = (const unsigned char*)(&acceptedFD);
  size_t bytesToWrite = sizeof(int);
  size_t totalBytesWritten = 0;

  do
  {
    const struct WriteToFDResult writeResult = writeToFD(
      ioThreadPipeWriteFDs[*nextPipeWriteFDIndex],
      &(pCharBuffer[totalBytesWritten]),
      bytesToWrite);
    if (writeResult.status != WRITE_TO_FD_SUCCESS)
    {
      proxyLog("error writing to pipeFD %d",
               ioThreadPipeWriteFDs[*nextPipeWriteFDIndex]);
      abort();
    }
    else
    {
      bytesToWrite -= writeResult.bytesWritten;
      totalBytesWritten += writeResult.bytesWritten;
    }
  } while (bytesToWrite > 0);

  ++(*nextPipeWriteFDIndex);
  if ((*nextPipeWriteFDIndex) >= proxySettings->numIOThreads)
  {
    *nextPipeWriteFDIndex = 0;
  }
}

static void handleServerSocketReady(
  const struct ServerSocketInfo* serverSocketInfo,
  const struct ProxySettings* proxySettings,
  const int* ioThreadPipeWriteFDs,
  size_t* nextPipeWriteFDIndex)
{
  bool acceptError = false;
  int numAccepts = 0;
  while ((!acceptError) &&
         (numAccepts < MAX_OPERATIONS_FOR_ONE_FD))
  {
    const int acceptedFD = signalSafeAccept(serverSocketInfo->socket, NULL, NULL);
    ++numAccepts;
    if (acceptedFD < 0)
    {
      if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
      {
        proxyLog("accept error errno %d", errno);
      }
      acceptError = true;
    }
    else
    {
      proxyLog("accepted fd %d", acceptedFD);
      writeAcceptedFDToIOThread(
        proxySettings,
        ioThreadPipeWriteFDs,
        nextPipeWriteFDIndex,
        acceptedFD);
    }
  }
}

struct AcceptorThreadCreateMessage
{
  int* ioThreadPipeWriteFDs;
  const struct ProxySettings* proxySettings;
};

static void* runAcceptorThread(void* param)
{
  struct AcceptorThreadCreateMessage* pCreateMessage = param;
  const int* ioThreadPipeWriteFDs = pCreateMessage->ioThreadPipeWriteFDs;
  const struct ProxySettings* proxySettings = pCreateMessage->proxySettings;
  size_t nextPipeWriteFDIndex = 0;
  struct PollState pollState;

  proxyLogSetThreadName("acceptor");

  free(pCreateMessage);
  pCreateMessage = NULL;
  param = NULL;

  initializePollState(&pollState);

  setupServerSockets(
    &(proxySettings->serverAddrInfoList),
    &pollState);

  while (true)
  {
    size_t i;
    const struct PollResult* pollResult = blockingPoll(&pollState);
    if (!pollResult)
    {
      proxyLog("blockingPoll failed");
      abort();
    }

    for (i = 0; 
         i < pollResult->numReadyFDs;
         ++i)
    {
      const struct ReadyFDInfo* readyFDInfo = &(pollResult->readyFDInfoArray[i]);
      const struct ServerSocketInfo* serverSocketInfo = readyFDInfo->data;
      handleServerSocketReady(
        serverSocketInfo, 
        proxySettings,
        ioThreadPipeWriteFDs,
        &nextPipeWriteFDIndex);
    }
  }

  return NULL;
}

static void startAcceptorThread(
  const struct ProxySettings* proxySettings,
  const struct IOThreadPipeInfo* ioThreadPipeInfoArray,
  struct LinkedList* pthreadList)
{
  int* ioThreadPipeWriteFDs;
  size_t i;
  struct AcceptorThreadCreateMessage* pAcceptorThreadCreateMessage;
  int pthreadRetVal;
  pthread_t* pPthread;

  ioThreadPipeWriteFDs = 
    checkedCalloc(
      proxySettings->numIOThreads, 
      sizeof(int));
  for (i = 0; i < proxySettings->numIOThreads; ++i)
  {
    ioThreadPipeWriteFDs[i] = ioThreadPipeInfoArray[i].writeFD;
  }

  pAcceptorThreadCreateMessage = 
    checkedMalloc(sizeof(struct AcceptorThreadCreateMessage));
  pAcceptorThreadCreateMessage->ioThreadPipeWriteFDs = ioThreadPipeWriteFDs;
  pAcceptorThreadCreateMessage->proxySettings = proxySettings;
  pPthread = checkedMalloc(sizeof(pthread_t));

  pthreadRetVal =
    pthread_create(
      pPthread, NULL,
      &runAcceptorThread,
      pAcceptorThreadCreateMessage);
  if (pthreadRetVal != 0)
  {
    proxyLog("pthread_create error %d", pthreadRetVal);
    abort();
  }

  addToLinkedList(pthreadList, pPthread);
}

static void joinThreads(
  struct LinkedList* pthreadList)
{
  struct LinkedListNode* nodePtr;

  for (nodePtr = pthreadList->head;
       nodePtr;
       nodePtr = nodePtr->next)
  {
    pthread_t* pPthread = nodePtr->data;
    int pthreadRetVal = pthread_join(*pPthread, NULL);
    if (pthreadRetVal != 0)
    {
      proxyLog("pthread_join error %d", pthreadRetVal);
      abort();
    }
  }
}

static void runProxy(
  const struct ProxySettings* proxySettings)
{
  struct IOThreadPipeInfo* ioThreadPipeInfoArray;
  struct LinkedList pthreadList = EMPTY_LINKED_LIST;

  setupSignals();

  proxyLog("remote address = %s:%s",
           proxySettings->remoteAddrPortStrings.addrString,
           proxySettings->remoteAddrPortStrings.portString);
  proxyLog("buffer size = %ld",
           (unsigned long)(proxySettings->bufferSize));
  proxyLog("no delay = %d",
           (unsigned int)(proxySettings->noDelay));
  proxyLog("num io threads = %ld",
           (unsigned long)(proxySettings->numIOThreads));

  ioThreadPipeInfoArray = createIOThreadPipes(proxySettings->numIOThreads);

  startIOThreads(proxySettings, ioThreadPipeInfoArray, &pthreadList);
  startAcceptorThread(proxySettings, ioThreadPipeInfoArray, &pthreadList);

  free(ioThreadPipeInfoArray);
  ioThreadPipeInfoArray = NULL;

  joinThreads(&pthreadList);
}

int main( 
  int argc,
  char** argv)
{
  const struct ProxySettings* proxySettings;

  proxyLogSetThreadName("main");

  proxySettings = processArgs(argc, argv);
  runProxy(proxySettings);

  return 0;
}
