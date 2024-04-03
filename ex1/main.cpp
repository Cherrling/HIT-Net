//#include "stdafx.h"
#include <tchar.h>
#include <stdio.h>
#include <Windows.h>
#include <process.h>
#include <string.h>

//��̬����һ��lib���ļ�,ws2_32.lib�ļ����ṩ�˶������������API��֧�֣���ʹ�����е�API����Ӧ�ý�ws2_32.lib���빤��
#pragma comment(lib, "Ws2_32.lib")

#define MAXSIZE 65507 // �������ݱ��ĵ���󳤶�
#define HTTP_PORT 80  // http �������˿�

#define INVALID_WEBSITE "http://http.p2hp.com/"   //��վ����
#define FISH_WEBSITE_FROM "http://http.p2hp.com/"  //������վԴ��ַ
#define FISH_WEBSITE_TO "http://jwes.hit.edu.cn/"  //������վĿ����ַ
#define FISH_WEBSITE_HOST "jwes.hit.edu.cn"        //����Ŀ�ĵ�ַ��������

// Http ��Ҫͷ������
struct HttpHeader
{
    char method[4];         // POST ���� GET��ע����ЩΪ CONNECT����ʵ���ݲ�����
    char url[1024];         // ����� url
    char host[1024];        // Ŀ������
    char cookie[1024 * 10]; // cookie
    HttpHeader()
    {
        ZeroMemory(this, sizeof(HttpHeader));
    }
};

BOOL InitSocket();
void ParseHttpHead(char* buffer, HttpHeader* httpHeader);
BOOL ConnectToServer(SOCKET* serverSocket, char* host);
unsigned int __stdcall ProxyThread(LPVOID lpParameter);
boolean ParseDate(char* buffer, char* field, char* tempDate);
void makeNewHTTP(char* buffer, char* value);
void makeFilename(char* url, char* filename);
void makeCache(char* buffer, char* url);
void getCache(char* buffer, char* filename);

/*
������ز���:
    SOCKET��������һ��unsigned int��������Ψһ��ID
    sockaddr_in��������������ͨ�ŵĵ�ַ��sockaddr_in����socket����͸�ֵ��sockaddr���ں�������
        short   sin_family;         ��ַ��
        u_short sin_port;           16λTCP/UDP�˿ں�
        struct  in_addr sin_addr;   32λIP��ַ
        char    sin_zero[8];        ��ʹ��
*/
SOCKET ProxyServer;
sockaddr_in ProxyServerAddr;
const int ProxyPort = 10240;

//������ز���
boolean haveCache = FALSE;
boolean needCache = TRUE;
char* strArr[100];

// �����µ����Ӷ�ʹ�����߳̽��д������̵߳�Ƶ���Ĵ����������ر��˷���Դ
// ����ʹ���̳߳ؼ�����߷�����Ч��
// const int ProxyThreadMaxNum = 20;
// HANDLE ProxyThreadHandle[ProxyThreadMaxNum] = {0};
// DWORD ProxyThreadDW[ProxyThreadMaxNum] = {0};

struct ProxyParam
{
    SOCKET clientSocket;
    SOCKET serverSocket;
};

int _tmain(int argc, _TCHAR* argv[])
{
    printf("�����������������\n");
    printf("��ʼ��...\n");
    if (!InitSocket())
    {
        printf("socket ��ʼ��ʧ��\n");
        return -1;
    }
    printf("����������������У������˿� %d\n", ProxyPort);
    SOCKET acceptSocket = INVALID_SOCKET; //��Ч�׽���-->��ʼ����
    ProxyParam* lpProxyParam;
    HANDLE hThread; //������Ͷ���һһ��Ӧ��32λ�޷�������ֵ
    DWORD dwThreadID; //unsigned long

    // ������������ϼ���
    while (true)
    {
        //printf("111\n");

        //accept���ͻ��˵���Ϣ�󶨵�һ��socket�ϣ�Ҳ���Ǹ��ͻ��˴���һ��socket��ͨ������ֵ���ظ����ǿͻ��˵�socket
        acceptSocket = accept(ProxyServer, NULL, NULL);
        lpProxyParam = new ProxyParam;
        if (lpProxyParam == NULL)
        {
            continue;
        }
        lpProxyParam->clientSocket = acceptSocket;

        //_beginthreadex�����̣߳���3��4�������ֱ�Ϊ�߳�ִ�к������̺߳����Ĳ���
        hThread = (HANDLE)_beginthreadex(NULL, 0, &ProxyThread, (LPVOID)lpProxyParam, 0, 0);

        /* CloseHandle
            ֻ�ǹر���һ���߳̾�����󣬱�ʾ�Ҳ���ʹ�øþ����
            ��������������Ӧ���߳����κθ�Ԥ�ˣ��ͽ����߳�û��һ���ϵ
        */
        CloseHandle(hThread);

        //�ӳ� from <windows.h>
        Sleep(200);
    }
    closesocket(ProxyServer);
    WSACleanup();
    return 0;
}

//************************************
// Method: InitSocket
// FullName: InitSocket
// Access: public
// Returns: BOOL
// Qualifier: ��ʼ���׽���
//************************************
BOOL InitSocket()
{

    // �����׽��ֿ⣨���룩
    WORD wVersionRequested;
    WSADATA wsaData;
    // �׽��ּ���ʱ������ʾ
    int err;
    // �汾 2.2
    wVersionRequested = MAKEWORD(2, 2);
    // ���� dll �ļ� Scoket ��
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        // �Ҳ��� winsock.dll
        printf("���� winsock ʧ�ܣ��������Ϊ: %d\n", WSAGetLastError());
        return FALSE;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        printf("�����ҵ���ȷ�� winsock �汾\n");
        WSACleanup();
        return FALSE;
    }

    /*socket() ������
        int af����ַ��淶����ǰ֧�ֵ�ֵΪAF_INET��AF_INET6������IPv4��IPv6��Internet��ַ���ʽ
        int type�����׽��ֵ����͹淶��SOCK_STREAM 1��һ���׽������ͣ���ͨ��OOB���ݴ�������ṩ
                  ˳��ģ��ɿ��ģ�˫��ģ��������ӵ��ֽ���
        int protocol��Э�顣ֵΪ0��������߲�ϣ��ָ��Э�飬�����ṩ�̽�ѡ��Ҫʹ�õ�Э��
    */
    ProxyServer = socket(AF_INET, SOCK_STREAM, 0);

    if (INVALID_SOCKET == ProxyServer)
    {
        printf("�����׽���ʧ�ܣ��������Ϊ�� %d\n", WSAGetLastError());
        return FALSE;
    }

    ProxyServerAddr.sin_family = AF_INET;
    ProxyServerAddr.sin_port = htons(ProxyPort);    //��һ���޷��Ŷ�������ֵת��ΪTCP/IP�����ֽ��򣬼����ģʽ(big-endian)
    //ProxyServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;  //INADDR_ANY == 0.0.0.0����ʾ����������IP
    ProxyServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.3");  //���ޱ����û��ɷ���


    //���׽����������ַ
    if (bind(ProxyServer, (SOCKADDR*)&ProxyServerAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        printf("���׽���ʧ��\n");
        return FALSE;
    }

    //�����׽���
    if (listen(ProxyServer, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("�����˿�%d ʧ��", ProxyPort);
        return FALSE;
    }
    return TRUE;
}

//************************************
// Method: ProxyThread
// FullName: ProxyThread
// Access: public
// Returns: unsigned int __stdcall
// Qualifier: �߳�ִ�к���
// Parameter: LPVOID lpParameter
// �̵߳��������ھ����̺߳����ӿ�ʼִ�е��߳̽���
// //************************************
unsigned int __stdcall ProxyThread(LPVOID lpParameter)
{
    //���建�����
    //char fileBuffer[MAXSIZE];
    //char* DateBuffer;
    //DateBuffer = (char*)malloc(MAXSIZE);
    //char filename[100];
    //char* field = "Date";
    //char date_str[30];  //�����ֶ�Date��ֵ

    char Buffer[MAXSIZE];
    char* CacheBuffer;
    ZeroMemory(Buffer, MAXSIZE);
    SOCKADDR_IN clientAddr;
    int length = sizeof(SOCKADDR_IN);
    int recvSize;
    int ret;
    recvSize = recv(((ProxyParam*)lpParameter)->clientSocket, Buffer, MAXSIZE, 0);
    HttpHeader* httpHeader = new HttpHeader();
    CacheBuffer = new char[recvSize + 1];

    //goto��䲻������ʵ�������ֲ��������壩����ʵ�����Ƶ�������ͷ�Ϳ�����
    if (recvSize <= 0)
    {
        goto error;
    }

    ZeroMemory(CacheBuffer, recvSize + 1);
    memcpy(CacheBuffer, Buffer, recvSize);  //��srcָ���ַΪ��ʼ��ַ������n���ֽڵ����ݸ��Ƶ���destinָ���ַΪ��ʼ��ַ�Ŀռ���
    //����httpheader
    ParseHttpHead(CacheBuffer, httpHeader);

    //����
    //ZeroMemory(DateBuffer, strlen(Buffer) + 1);
    //memcpy(DateBuffer, Buffer, strlen(Buffer) + 1);
    ////printf("DateBuffer: \n%s\n", DateBuffer);
    //ZeroMemory(filename, 100);
    //makeFilename(httpHeader->url, filename);
    ////printf("filename : %s\n", filename);
    //ZeroMemory(date_str, 30);
    //ZeroMemory(fileBuffer, MAXSIZE);
    //FILE* in;
    //if ((in = fopen(filename, "rb")) != NULL) {
    //    printf("\n�л���\n");
    //    fread(fileBuffer, sizeof(char), MAXSIZE, in);
    //    fclose(in);
    //    ParseDate(fileBuffer, field, date_str);
    //    printf("date_str:%s\n", date_str);
    //    makeNewHTTP(Buffer, date_str);
    //    haveCache = TRUE;
    //    goto success;
    //}

    delete CacheBuffer;
    //printf("test\n");

    //�ڷ��ͱ���ǰ��������
    //if (strcmp(httpHeader->url,INVALID_WEBSITE)==0)
    //{
    //    printf("************************************\n");
    //    printf("----------����վ�ѱ�����------------\n");
    //    printf("************************************\n");
    //    goto error;
    //}

    //��վ����
    //if (strstr(httpHeader->url, FISH_WEBSITE_FROM) != NULL) {
    //    printf("\n=====================================\n\n");
    //    printf("-------------�Ѵ�Դ��ַ��%s ת�� Ŀ����ַ ��%s ----------------\n", FISH_WEBSITE_FROM, FISH_WEBSITE_TO);
    //    memcpy(httpHeader->host, FISH_WEBSITE_HOST, strlen(FISH_WEBSITE_HOST) + 1);
    //    memcpy(httpHeader->url, FISH_WEBSITE_TO, strlen(FISH_WEBSITE_TO));
    // }


    if (!ConnectToServer(&((ProxyParam*)lpParameter)->serverSocket, httpHeader->host))
    {
        //printf("error\n");
        goto error;
    }
    printf("������������ %s �ɹ�\n", httpHeader->host);
    //printf("test");

    // ���ͻ��˷��͵� HTTP ���ݱ���ֱ��ת����Ŀ�������
    ret = send(((ProxyParam*)lpParameter)->serverSocket, Buffer, strlen(Buffer) + 1, 0);

    // �ȴ�Ŀ���������������
    recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buffer, MAXSIZE, 0);
    if (recvSize <= 0)
    {
        goto error;
    }
    // ��Ŀ����������ص�����ֱ��ת�����ͻ���
    ret = send(((ProxyParam*)lpParameter)->clientSocket, Buffer, sizeof(Buffer), 0);


    //success:
    //    if (!ConnectToServer(&((ProxyParam*)lpParameter)->serverSocket, httpHeader->host)) {
    //        printf("����Ŀ�������ʧ�ܣ�����\n");
    //        goto error;
    //    }
    //    printf("������������ %s �ɹ�\n", httpHeader->host);
    //    //���ͻ��˷��͵� HTTP ���ݱ���ֱ��ת����Ŀ�������
    //    ret = send(((ProxyParam*)lpParameter)->serverSocket, Buffer, strlen(Buffer) + 1, 0);
    //    //�ȴ�Ŀ���������������
    //    recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buffer, MAXSIZE, 0);
    //    if (recvSize <= 0) {
    //        printf("����Ŀ�������������ʧ�ܣ�����\n");
    //        goto error;
    //    }
    //    //�л���ʱ���жϷ��ص�״̬���Ƿ���304�������򽫻�������ݷ��͸��ͻ���
    //    if (haveCache == TRUE) {
    //        getCache(Buffer, filename);
    //    }
    //    if (needCache == TRUE) {
    //        makeCache(Buffer, httpHeader->url);  //���汨��
    //    }
    //    //��Ŀ����������ص�����ֱ��ת�����ͻ���
    //    ret = send(((ProxyParam*)lpParameter)->clientSocket, Buffer, sizeof(Buffer), 0);
    //    

    // ������
error:
    printf("�ر��׽���\n");
    Sleep(200);
    closesocket(((ProxyParam*)lpParameter)->clientSocket);
    closesocket(((ProxyParam*)lpParameter)->serverSocket);
    delete lpParameter;
    _endthreadex(0);
    return 0;
}

//************************************
// Method: ParseHttpHead
// FullName: ParseHttpHead
// Access: public
// Returns: void
// Qualifier: ���� TCP �����е� HTTP ͷ��
// Parameter: char * buffer
// Parameter: HttpHeader * httpHeader
//************************************
void ParseHttpHead(char* buffer, HttpHeader* httpHeader)
{
    char* p;
    char* ptr;
    const char* delim = "\r\n";
    p = strtok_s(buffer, delim, &ptr); // ��ȡ��һ��
    printf("%s\n", p);
    if (p[0] == 'G')
    { // GET ��ʽ
        memcpy(httpHeader->method, "GET", 3);
        memcpy(httpHeader->url, &p[4], strlen(p) - 13);
    }
    else if (p[0] == 'P')
    { // POST ��ʽ
        memcpy(httpHeader->method, "POST", 4);
        memcpy(httpHeader->url, &p[5], strlen(p) - 14);
    }
    printf("���ڷ���url��%s\n", httpHeader->url);
    p = strtok_s(NULL, delim, &ptr);
    while (p)
    {
        switch (p[0])
        {
        case 'H': // Host
            memcpy(httpHeader->host, &p[6], strlen(p) - 6);
            break;
        case 'C': // Cookie
            if (strlen(p) > 8)
            {
                char header[8];
                ZeroMemory(header, sizeof(header));
                memcpy(header, p, 6);
                if (!strcmp(header, "Cookie"))
                {
                    memcpy(httpHeader->cookie, &p[8], strlen(p) - 8);
                }
            }
            break;
        default:
            break;
        }
        p = strtok_s(NULL, delim, &ptr);
    }
}

//************************************
// Method: ConnectToServer
// FullName: ConnectToServer
// Access: public
// Returns: BOOL
// Qualifier: ������������Ŀ��������׽��֣�������
// Parameter: SOCKET * serverSocket
// Parameter: char * host
//************************************
BOOL ConnectToServer(SOCKET* serverSocket, char* host)
{
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(HTTP_PORT);
    HOSTENT* hostent = gethostbyname(host);
    if (!hostent)
    {
        printf("error_hostent\n");
        return FALSE;
    }
    in_addr Inaddr = *((in_addr*)*hostent->h_addr_list);
    serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));
    *serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (*serverSocket == INVALID_SOCKET)
    {
        printf("error_serverSocket\n");
        return FALSE;
    }
    if (connect(*serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        printf("error_connect\n");
        closesocket(*serverSocket);
        return FALSE;
    }
    return TRUE;
}

