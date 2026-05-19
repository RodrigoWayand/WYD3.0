#pragma once

#include <WinSock2.h>

#define RECV_BUFFER_SIZE   131072
#define SEND_BUFFER_SIZE   131072

#define MAX_KEYWORD_QUEUE  16

#define INIT_CODE          521270033

class CPSock
{
public:
    CPSock();
    ~CPSock();

    // ---------------------------------------------------------------------------
    // Initialization
    // ---------------------------------------------------------------------------

    bool         WSAInitialize();

    // ---------------------------------------------------------------------------
    // Server / Client connection
    // ---------------------------------------------------------------------------

    unsigned int StartListen(HWND hWnd, int ip, int port, int WSA);
    unsigned int ConnectServer(char* HostAddr, int Port, int ip, int WSA);
    unsigned int SingleConnect(char* HostAddr, int Port, int ip, int WSA);

    // ---------------------------------------------------------------------------
    // I/O
    // ---------------------------------------------------------------------------

    int   Receive();
    char* ReadMessage(int* ErrorCode, int* ErrorType);
    int   CloseSocket();

    // ---------------------------------------------------------------------------
    // Message sending (encrypted)
    // ---------------------------------------------------------------------------

    int  AddMessage(char* pMsg, int Size);
    int  AddMessage(char* pMsg, int Size, int FixedKeyWord);
    bool SendMessageA();
    int  SendOneMessage(char* Msg, int Size);
    int  SendOneMessageKeyword(char* Msg, int Size, int Keyword);

    // ---------------------------------------------------------------------------
    // Message sending (raw)
    // ---------------------------------------------------------------------------

    int   AddMessage2(char* pMsg, int Size);
    char* ReadMessage2(int* ErrorCode, int* ErrorType);

    // ---------------------------------------------------------------------------
    // Buffer management
    // ---------------------------------------------------------------------------

    void RefreshRecvBuffer();
    void RefreshSendBuffer();

public:
    unsigned int Sock;
    char* pSendBuffer;
    char* pRecvBuffer;
    int          nSendPosition;
    int          nRecvPosition;
    int          nProcPosition;
    int          nSentPosition;
    int          Init;

    char SendQueue[MAX_KEYWORD_QUEUE];
    char RecvQueue[MAX_KEYWORD_QUEUE];

    int SendCount;
    int RecvCount;
    int ErrCount;

private:
    // ---------------------------------------------------------------------------
    // Internal helpers
    // ---------------------------------------------------------------------------

    // Shared bind/connect logic for ConnectServer / SingleConnect
    unsigned int BindAndConnect(SOCKET tSock, sockaddr_in& InAddr, sockaddr_in& local_sin, int WSA, bool bIsSingle);

    // Generate next keyword from SendQueue (original algorithm preserved)
    int  GenerateSendKeyword();

    // Decrypt message body after the standard header (original algorithm preserved)
    bool DecryptMessageBody(char* pMsg, int Size, unsigned char KeyWord, unsigned char CheckSum, int* ErrorCode, int* ErrorType);

    // Encrypt message body into send buffer (original algorithm preserved)
    void EncryptMessageBody(char* pMsg, int Size, unsigned char iKeyWord, unsigned char* pOutSum1, unsigned char* pOutSum2);
};