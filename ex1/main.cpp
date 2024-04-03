//#include "stdafx.h"
#include <tchar.h>
#include <stdio.h>
#include <Windows.h>
#include <process.h>
#include <string.h>

//静态加入一个lib库文件,ws2_32.lib文件，提供了对以下网络相关API的支持，若使用其中的API，则应该将ws2_32.lib加入工程
#pragma comment(lib, "Ws2_32.lib")

#define MAXSIZE 65507 // 发送数据报文的最大长度
#define HTTP_PORT 80  // http 服务器端口

#define INVALID_WEBSITE "http://http.p2hp.com/"   //网站过滤
#define FISH_WEBSITE_FROM "http://http.p2hp.com/"  //钓鱼网站源网址
#define FISH_WEBSITE_TO "http://jwes.hit.edu.cn/"  //钓鱼网站目的网址
#define FISH_WEBSITE_HOST "jwes.hit.edu.cn"        //钓鱼目的地址的主机名

// Http 重要头部数据
struct HttpHeader
{
    char method[4];         // POST 或者 GET，注意有些为 CONNECT，本实验暂不考虑
    char url[1024];         // 请求的 url
    char host[1024];        // 目标主机
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
代理相关参数:
    SOCKET：本质是一个unsigned int整数，是唯一的ID
    sockaddr_in：用来处理网络通信的地址。sockaddr_in用于socket定义和赋值；sockaddr用于函数参数
        short   sin_family;         地址族
        u_short sin_port;           16位TCP/UDP端口号
        struct  in_addr sin_addr;   32位IP地址
        char    sin_zero[8];        不使用
*/
SOCKET ProxyServer;
sockaddr_in ProxyServerAddr;
const int ProxyPort = 10240;

//缓存相关参数
boolean haveCache = FALSE;
boolean needCache = TRUE;
char* strArr[100];

// 由于新的连接都使用新线程进行处理，对线程的频繁的创建和销毁特别浪费资源
// 可以使用线程池技术提高服务器效率
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
    printf("代理服务器正在启动\n");
    printf("初始化...\n");
    if (!InitSocket())
    {
        printf("socket 初始化失败\n");
        return -1;
    }
    printf("代理服务器正在运行，监听端口 %d\n", ProxyPort);
    SOCKET acceptSocket = INVALID_SOCKET; //无效套接字-->初始化？
    ProxyParam* lpProxyParam;
    HANDLE hThread; //句柄：和对象一一对应的32位无符号整数值
    DWORD dwThreadID; //unsigned long

    // 代理服务器不断监听
    while (true)
    {
        //printf("111\n");

        //accept将客户端的信息绑定到一个socket上，也就是给客户端创建一个socket，通过返回值返回给我们客户端的socket
        acceptSocket = accept(ProxyServer, NULL, NULL);
        lpProxyParam = new ProxyParam;
        if (lpProxyParam == NULL)
        {
            continue;
        }
        lpProxyParam->clientSocket = acceptSocket;

        //_beginthreadex创建线程，第3、4个参数分别为线程执行函数、线程函数的参数
        hThread = (HANDLE)_beginthreadex(NULL, 0, &ProxyThread, (LPVOID)lpProxyParam, 0, 0);

        /* CloseHandle
            只是关闭了一个线程句柄对象，表示我不再使用该句柄，
            即不对这个句柄对应的线程做任何干预了，和结束线程没有一点关系
        */
        CloseHandle(hThread);

        //延迟 from <windows.h>
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
// Qualifier: 初始化套接字
//************************************
BOOL InitSocket()
{

    // 加载套接字库（必须）
    WORD wVersionRequested;
    WSADATA wsaData;
    // 套接字加载时错误提示
    int err;
    // 版本 2.2
    wVersionRequested = MAKEWORD(2, 2);
    // 加载 dll 文件 Scoket 库
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        // 找不到 winsock.dll
        printf("加载 winsock 失败，错误代码为: %d\n", WSAGetLastError());
        return FALSE;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        printf("不能找到正确的 winsock 版本\n");
        WSACleanup();
        return FALSE;
    }

    /*socket() 函数：
        int af：地址族规范。当前支持的值为AF_INET或AF_INET6，这是IPv4和IPv6的Internet地址族格式
        int type：新套接字的类型规范。SOCK_STREAM 1是一种套接字类型，可通过OOB数据传输机制提供
                  顺序的，可靠的，双向的，基于连接的字节流
        int protocol：协议。值为0，则调用者不希望指定协议，服务提供商将选择要使用的协议
    */
    ProxyServer = socket(AF_INET, SOCK_STREAM, 0);

    if (INVALID_SOCKET == ProxyServer)
    {
        printf("创建套接字失败，错误代码为： %d\n", WSAGetLastError());
        return FALSE;
    }

    ProxyServerAddr.sin_family = AF_INET;
    ProxyServerAddr.sin_port = htons(ProxyPort);    //将一个无符号短整型数值转换为TCP/IP网络字节序，即大端模式(big-endian)
    //ProxyServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;  //INADDR_ANY == 0.0.0.0，表示本机的所有IP
    ProxyServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.3");  //仅限本机用户可访问


    //绑定套接字与网络地址
    if (bind(ProxyServer, (SOCKADDR*)&ProxyServerAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        printf("绑定套接字失败\n");
        return FALSE;
    }

    //监听套接字
    if (listen(ProxyServer, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("监听端口%d 失败", ProxyPort);
        return FALSE;
    }
    return TRUE;
}

//************************************
// Method: ProxyThread
// FullName: ProxyThread
// Access: public
// Returns: unsigned int __stdcall
// Qualifier: 线程执行函数
// Parameter: LPVOID lpParameter
// 线程的生命周期就是线程函数从开始执行到线程结束
// //************************************
unsigned int __stdcall ProxyThread(LPVOID lpParameter)
{
    //定义缓存变量
    //char fileBuffer[MAXSIZE];
    //char* DateBuffer;
    //DateBuffer = (char*)malloc(MAXSIZE);
    //char filename[100];
    //char* field = "Date";
    //char date_str[30];  //保存字段Date的值

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

    //goto语句不能跳过实例化（局部变量定义），把实例化移到函数开头就可以了
    if (recvSize <= 0)
    {
        goto error;
    }

    ZeroMemory(CacheBuffer, recvSize + 1);
    memcpy(CacheBuffer, Buffer, recvSize);  //由src指向地址为起始地址的连续n个字节的数据复制到以destin指向地址为起始地址的空间内
    //解析httpheader
    ParseHttpHead(CacheBuffer, httpHeader);

    //缓存
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
    //    printf("\n有缓存\n");
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

    //在发送报文前进行拦截
    //if (strcmp(httpHeader->url,INVALID_WEBSITE)==0)
    //{
    //    printf("************************************\n");
    //    printf("----------该网站已被屏蔽------------\n");
    //    printf("************************************\n");
    //    goto error;
    //}

    //网站引导
    //if (strstr(httpHeader->url, FISH_WEBSITE_FROM) != NULL) {
    //    printf("\n=====================================\n\n");
    //    printf("-------------已从源网址：%s 转到 目的网址 ：%s ----------------\n", FISH_WEBSITE_FROM, FISH_WEBSITE_TO);
    //    memcpy(httpHeader->host, FISH_WEBSITE_HOST, strlen(FISH_WEBSITE_HOST) + 1);
    //    memcpy(httpHeader->url, FISH_WEBSITE_TO, strlen(FISH_WEBSITE_TO));
    // }


    if (!ConnectToServer(&((ProxyParam*)lpParameter)->serverSocket, httpHeader->host))
    {
        //printf("error\n");
        goto error;
    }
    printf("代理连接主机 %s 成功\n", httpHeader->host);
    //printf("test");

    // 将客户端发送的 HTTP 数据报文直接转发给目标服务器
    ret = send(((ProxyParam*)lpParameter)->serverSocket, Buffer, strlen(Buffer) + 1, 0);

    // 等待目标服务器返回数据
    recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buffer, MAXSIZE, 0);
    if (recvSize <= 0)
    {
        goto error;
    }
    // 将目标服务器返回的数据直接转发给客户端
    ret = send(((ProxyParam*)lpParameter)->clientSocket, Buffer, sizeof(Buffer), 0);


    //success:
    //    if (!ConnectToServer(&((ProxyParam*)lpParameter)->serverSocket, httpHeader->host)) {
    //        printf("连接目标服务器失败！！！\n");
    //        goto error;
    //    }
    //    printf("代理连接主机 %s 成功\n", httpHeader->host);
    //    //将客户端发送的 HTTP 数据报文直接转发给目标服务器
    //    ret = send(((ProxyParam*)lpParameter)->serverSocket, Buffer, strlen(Buffer) + 1, 0);
    //    //等待目标服务器返回数据
    //    recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buffer, MAXSIZE, 0);
    //    if (recvSize <= 0) {
    //        printf("返回目标服务器的数据失败！！！\n");
    //        goto error;
    //    }
    //    //有缓存时，判断返回的状态码是否是304，若是则将缓存的内容发送给客户端
    //    if (haveCache == TRUE) {
    //        getCache(Buffer, filename);
    //    }
    //    if (needCache == TRUE) {
    //        makeCache(Buffer, httpHeader->url);  //缓存报文
    //    }
    //    //将目标服务器返回的数据直接转发给客户端
    //    ret = send(((ProxyParam*)lpParameter)->clientSocket, Buffer, sizeof(Buffer), 0);
    //    

    // 错误处理
error:
    printf("关闭套接字\n");
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
// Qualifier: 解析 TCP 报文中的 HTTP 头部
// Parameter: char * buffer
// Parameter: HttpHeader * httpHeader
//************************************
void ParseHttpHead(char* buffer, HttpHeader* httpHeader)
{
    char* p;
    char* ptr;
    const char* delim = "\r\n";
    p = strtok_s(buffer, delim, &ptr); // 提取第一行
    printf("%s\n", p);
    if (p[0] == 'G')
    { // GET 方式
        memcpy(httpHeader->method, "GET", 3);
        memcpy(httpHeader->url, &p[4], strlen(p) - 13);
    }
    else if (p[0] == 'P')
    { // POST 方式
        memcpy(httpHeader->method, "POST", 4);
        memcpy(httpHeader->url, &p[5], strlen(p) - 14);
    }
    printf("正在访问url：%s\n", httpHeader->url);
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
// Qualifier: 根据主机创建目标服务器套接字，并连接
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

