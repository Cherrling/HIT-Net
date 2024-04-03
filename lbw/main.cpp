#include <stdio.h>
#include <Windows.h>
#include <process.h>
#include <string.h>
#include <tchar.h> 
#pragma comment(lib,"Ws2_32.lib")
#define MAXSIZE 65507 //发送数据报文的最大长度
#define HTTP_PORT 80 //http 服务器端口
#define tt 1
//Http重要头部数据
struct HttpHeader {
    char method[4]; // POST 或者 GET，注意有些为 CONNECT，本实验暂不考虑
    char url[1024]; // 请求的 url
    char host[1024]; // 目标主机
    char cookie[1024 * 10]; //cookie
    HttpHeader() {
        ZeroMemory(this, sizeof(HttpHeader));
    };
};
//禁止访问的网站和钓鱼网站是否可以输入选择
char Invilid_web[1024] = "http://hit.edu.cn/";//不允许访问的网站

char Target_web[1024] = "http://ivpn.hit.edu.cn";//钓鱼原网站
char Fish_web[1024] = "http://jwts.hit.edu.cn/";//钓鱼网站
char Fish_host[1024] = "jwts.hit.edu.cn"; //钓鱼主机名

char InvalidIP[] = "128.0.0.1";//屏蔽的用户IP
BOOL InitSocket();//初始化套接字
void ParseHttpHead(char* buffer, HttpHeader* httpHeader);//解析HTTP报文头部
BOOL ConnectToServer(SOCKET* serverSocket, char* host);//连接到服务器
unsigned int __stdcall ProxyThread(LPVOID lpParameter);//代理线程执行函数,__stdcall关键字，表示使用标准调用约定
//代理相关参数
SOCKET ProxyServer;//代理服务器
sockaddr_in ProxyServerAddr;//代理服务器地址
const int ProxyPort = 65432;//设置代理窗口
int addrlen = 32;

//缓存相关参数
boolean haveCache = false;//表示是否已经缓存了文件
boolean needCache = true;
void getfileDate(FILE* in, char* tempDate);//从文件中读取日期信息，并将日期信息存储在tempDate中
void sendnewHTTP(char* buffer, char* datestring);//发送一个新的HTTP请求，并将响应存储在buffer中
void makeFilename(char* url, char* filename);//根据URL生成文件名
void storefileCache(char* buffer, char* url);//将文件内容存储到缓存中
void checkfileCache(char* buffer, char* filename);//检查缓存中是否存在该文件，如果存在，则将文件内容返回
//由于新的连接都使用新线程进行处理，对线程的频繁的创建和销毁特别浪费资源
//可以使用线程池技术提高服务器效率
//const int ProxyThreadMaxNum = 20;
//HANDLE ProxyThreadHandle[ProxyThreadMaxNum] = {0};
//DWORD ProxyThreadDW[ProxyThreadMaxNum] = {0};;;



//线程池相关结构体和函数
const int MAX_THREADS = 20; // 最大线程数量
HANDLE ThreadPool[MAX_THREADS]; // 线程池数组
unsigned int ThreadPoolID[MAX_THREADS]; // 线程池线程ID数组
HANDLE ThreadPoolMutex; // 互斥量，用于保护线程池的操作
int ThreadPoolIndex = 0; // 线程池索引


// 代表线程状态的枚举
enum ThreadStatus {
    RUNNING,
    IDLE,
    TERMINATED
};
struct ThreadPoolTask {
    SOCKET clientSocket;
    SOCKET serverSocket;
    ThreadStatus status; // 线程状态
};

// 线程池初始化
void InitThreadPool() {
    ThreadPoolMutex = CreateMutex(NULL, FALSE, NULL);
}

// 提交任务到线程池
void SubmitTaskToThreadPool(ThreadPoolTask* task) {
    WaitForSingleObject(ThreadPoolMutex, INFINITE); // 获取互斥量
    if (ThreadPoolIndex < MAX_THREADS) {
        task->status = RUNNING; // 设置任务状态为运行中
        ThreadPool[ThreadPoolIndex] = (HANDLE)_beginthreadex(NULL, 0, ProxyThread, (LPVOID)task, 0, &ThreadPoolID[ThreadPoolIndex]);
        ThreadPoolIndex++;
    }
    ReleaseMutex(ThreadPoolMutex); // 释放互斥量
}

// 释放线程资源
void ReleaseThreadResource(ThreadPoolTask* task) {
    task->status = TERMINATED; // 设置任务状态为终止
    _endthreadex(0); // 终止线程
    WaitForSingleObject(ThreadPoolMutex, INFINITE); // 获取互斥量
    ThreadPoolIndex--; // 减少线程池索引
    ReleaseMutex(ThreadPoolMutex); // 释放互斥量
}



struct ProxyParam {
    SOCKET clientSocket;
    SOCKET serverSocket;
};//存储代理服务器与客户端和服务器之间的连接信息

//主程序
int _tmain(int argc, _TCHAR* argv[])
{
    printf("代理服务器正在启动\n");
    printf("初始化...\n");
    if (!InitSocket()) {
        printf("socket 初始化失败\n");
        return -1;
    }
    printf("代理服务器正在运行，监听端口 %d\n", ProxyPort);
    SOCKET acceptSocket = INVALID_SOCKET;//用于存储接受到的客户端套接字，初始值为 INVALID_SOCKET.当服务器接收到客户端的连接请求时，会创建一个新的套接字，并将其赋值给 acceptSocket
    //把socket设置成无效套接字
    SOCKADDR_IN acceptAddr; //自定义变量，用来获得用户的IP。这个变量是一个结构体，用于存储客户端的IP地址和端口号
    ProxyParam* lpProxyParam;
    HANDLE hThread;//用于存储接受客户端连接的线程句柄
    DWORD dwThreadID;//unsigned long，无符号32位整型,线程 ID 是线程的唯一标识符,存储接受客户端连接的线程 ID
    //代理服务器不断监听
    while (TRUE) {//使用accept()函数接受客户端的连接请求，并返回一个新的套接字acceptSocket用于通信。
        //同时，将客户端的IP地址存储在acceptAddr结构体中，并获取客户端的IP地址字符串
        acceptSocket = accept(ProxyServer, (SOCKADDR*)&acceptAddr, &addrlen);
        printf("===============用户IP地址为%s===============\n", inet_ntoa(acceptAddr.sin_addr));
        if (strcmp(inet_ntoa(acceptAddr.sin_addr), InvalidIP) == 0)//屏蔽用户ip
        {
            printf("\n\n===============该用户不允许访问===============\n\n");
        }
        else {
            if (tt==0)
            {

                lpProxyParam = new ProxyParam;//每个请求都创建一个新的线程来处理
                if (lpProxyParam == NULL) //如果lpProxyParam为空，则跳过当前循环，继续接受新的连接请求
                {
                    continue;
                }
                lpProxyParam->clientSocket = acceptSocket;
                //线程开始
                //创建一个新的线程来处理代理功能，传入 ProxyParam 结构体作为参数
                hThread = (HANDLE)_beginthreadex(NULL, 0, &ProxyThread, (LPVOID)lpProxyParam, 0, 0);
                CloseHandle(hThread);//关闭线程句柄
            }
            else
            {

                ThreadPoolTask* task = new ThreadPoolTask;
                task->clientSocket = acceptSocket;
                SubmitTaskToThreadPool(task); // 提交任务到线程池处理

            }


        }
        Sleep(200);
    }
    closesocket(ProxyServer);//关闭代理服务器的套接字
    WSACleanup();//清理 Winsock 库的资源
    return 0;
}
//************************************
// Method: InitSocket
// FullName: InitSocket
// Access: public
// Returns: BOOL
// Qualifier: 初始化套接字
//************************************

BOOL InitSocket() //初始化套接字
{
    //加载套接字库（必须）
    WORD wVersionRequested;//16位整数类型，用于表示版本号
    WSADATA wsaData;//这是一个结构体类型，用于存储Winsock库的版本信息、套接字选项等
    //套接字加载时错误提示
    int err;
    //版本 2.2
    wVersionRequested = MAKEWORD(2, 2);//指定要加载的Winsock库的版本（必须）
    //加载 dll 文件 Scoket 库
    err = WSAStartup(wVersionRequested, &wsaData);//调用成功则返回0
    if (err != 0)
    {
        //找不到 winsock.dll
        printf("加载 winsock 失败，错误代码为: %d\n", WSAGetLastError());
        return FALSE;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)//版本不是2.2
    {
        printf("不能找到正确的 winsock 版本\n");
        WSACleanup();//调用WSACleanup()函数来清理Windows Socket库
        return FALSE;
    }
    //创建套接字
    ProxyServer = socket(AF_INET, SOCK_STREAM, 0);//AF_INET 参数表示使用 IPv4 地址族,SOCK_STREAM 参数使用 TCP 协议进行通信,0 参数表示使用默认的传输协议
    if (INVALID_SOCKET == ProxyServer)//INVALID_SOCKET 是一个宏，用于表示无效的套接字。INVALID_SOCKET 的值为 -1
    {
        printf("创建套接字失败，错误代码为： %d\n", WSAGetLastError());
        return FALSE;//WSAGetLastError() 函数获取最近一次发生的 Winsock 错误代码
    }
    //设置代理服务器的地址信息，包括地址族、端口号和 IP 地址
    ProxyServerAddr.sin_family = AF_INET;//地址族ipv4
    ProxyServerAddr.sin_port = htons(ProxyPort); // 将代理端口转换为网络字节顺序，并将其设置为ProxyServerAddr结构体的sin_port字段
    ProxyServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//设置IP地址
    //将一个套接字（ProxyServer）绑定到一个指定的地址（ProxyServerAddr）。如果绑定失败，程序将输出“绑定套接字失败”并返回FALSE
    if (bind(ProxyServer, (SOCKADDR*)&ProxyServerAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        printf("绑定套接字失败\n");
        return FALSE;
    }
    //函数的第一个参数是用于监听的套接字，第二个参数是请求队列长度，即等待连接的客户端数量。如果请求队列已满，新的连接请求将被拒绝
    //listen监听，SOMAXCONN由系统来决定请求队列长度,5
    if (listen(ProxyServer, SOMAXCONN) == SOCKET_ERROR) {
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
//************************************
unsigned int __stdcall ProxyThread(LPVOID lpParameter)
{
    ThreadPoolTask* task;
    if (tt)
    {
        task = (ThreadPoolTask*)lpParameter;

    }
    char Buffer[MAXSIZE];//存储客户端发送的 HTTP 数据报文
    char* CacheBuffer;//指向缓存区。缓存区用于存储从网络数据包中解析出的数据
    char* DateBuffer;//指向日期缓冲区。日期缓冲区用于存储从网络数据包中解析出的日期值
    char filename[100] = { 0 };
    _Post_ _Notnull_ FILE* in;//定义了一个文件指针，用于指向输入文件。_Post_ _Notnull_是一个宏，表示在函数返回之后，in指针必须不为空
    char date_str[30];  //定义了一个字符数组，用于存储日期字符串。日期字符串通常由年、月、日组成，例如 "2021-08-01"
    ZeroMemory(Buffer, MAXSIZE);//将Buffer数组的所有元素设置为0
    SOCKADDR_IN clientAddr;//定义了一个SOCKADDR_IN结构体变量，用于存储客户端的IP地址和端口号
    int length = sizeof(SOCKADDR_IN);
    int recvSize;//用于存储接收到的网络数据包的大小
    int ret;//用于存储函数返回值
    FILE* fp;
    //第一次接收客户端请求，将该请求缓存下来，存到本地文件中
    //使用recv函数从客户端套接字((ProxyParam*)lpParameter)->clientSocket中接收数据
    //数据存储在Buffer数组中，最大接收长度为MAXSIZE，阻塞模式为0。recvSize表示接收到的数据大小
    recvSize = recv(((ProxyParam*)lpParameter)->clientSocket, Buffer, MAXSIZE, 0);
    HttpHeader* httpHeader = new HttpHeader();
    if (recvSize <= 0) //判断接收到的数据大小是否小于等于0，如果是，则表示接收数据失败
    {
        goto error;
    }
    CacheBuffer = new char[recvSize + 1];
    ZeroMemory(CacheBuffer, recvSize + 1);
    memcpy(CacheBuffer, Buffer, recvSize);//将buffer接收到的数据复制到CacheBuffer中

    ParseHttpHead(CacheBuffer, httpHeader);     //解析HTTP报文头部
    //printf("HTTP请求报文如下：\n%s\n", Buffer);
    ZeroMemory(date_str, 30);
    printf("httpHeader->url : %s\n", httpHeader->url);
    makeFilename(httpHeader->url, filename);
    //printf("filename是 %s\n", filename);
    if ((fopen_s(&in, filename, "r")) == 0)
    {
        printf("\n有缓存\n");

        getfileDate(in, date_str);//得到本地缓存文件中的日期date_str
        fclose(in);
        //printf("date_str:%s\n", date_str);
        sendnewHTTP(Buffer, date_str);
        //向服务器发送一个请求，该请求需要增加 “If-Modified-Since” 字段
        //服务器通过对比时间来判断缓存是否过期
        haveCache = TRUE;
    }
    //printf("httpHeader的url是%s，不允许访问的是%s\n", httpHeader->url, Invilid_web);
    //网站过滤功能
    if (strcmp(httpHeader->url, Invilid_web) == 0) {
        printf("%s网站被拒绝访问\n", Invilid_web);
        goto error;
    }
    //添加钓鱼功能
    if (strstr(httpHeader->url, Target_web) != NULL) {
        printf("%s网站钓鱼成功，被转移至%s\n", Target_web, Fish_web);
        memcpy(httpHeader->host, Fish_host, strlen(Fish_host) + 1);//替换主机名
        memcpy(httpHeader->url, Fish_web, strlen(Fish_web) + 1);//替换url
    }
    //此时数据报存储在了httpHeader中
    delete CacheBuffer;
    //连接发送数据报所在的服务器
    if (!ConnectToServer(&((ProxyParam*)lpParameter)->serverSocket, httpHeader->host)) {
        printf("连接目的服务器失败\n");
        goto error;
    }
    printf("代理连接主机 %s 成功\n", httpHeader->host);
    //将客户端发送的 HTTP 数据报文直接转发给目标服务器
    ret = send(((ProxyParam*)lpParameter)->serverSocket, Buffer, strlen(Buffer) + 1, 0);
    //等待目标服务器返回数据
    recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buffer, MAXSIZE, 0);
    if (recvSize <= 0) {
        goto error;
    }
    //printf("服务器响应报文如下：\n%s\n", Buffer);
    if (haveCache == true) {
        checkfileCache(Buffer, httpHeader->url);
    }
    if (needCache == true) {
        storefileCache(Buffer, httpHeader->url);
    }
    //将目标服务器返回的数据直接转发给客户端
    ret = send(((ProxyParam*)lpParameter)->clientSocket, Buffer, sizeof(Buffer), 0);
    if (tt)
    {
        ReleaseThreadResource(task); // 释放线程资源
    }
    //错误处理
error:
    printf("关闭套接字\n\n");
    Sleep(200);
    closesocket(((ProxyParam*)lpParameter)->clientSocket);
    closesocket(((ProxyParam*)lpParameter)->serverSocket);
    delete lpParameter;
    _endthreadex(0); //终止线程
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
    char* p;//存储提取到的每一行字符串
    char* ptr;//指向当前行的指针
    const char* delim = "\r\n";//分隔符
    p = strtok_s(buffer, delim, &ptr);//提取第一行
    printf("%s\n", p);
    if (p[0] == 'G') {//GET 方式
        //将GET复制到httpHeader->method中，并将字符串p中从第5个字符开始的子串复制到httpHeader->url中
        memcpy(httpHeader->method, "GET", 3);
        memcpy(httpHeader->url, &p[4], strlen(p) - 13);
    }
    else if (p[0] == 'P') {//POST 方式
        //将POST复制到httpHeader->method中，并将字符串p中从第6个字符开始的子串复制到httpHeader->url中
        memcpy(httpHeader->method, "POST", 4);
        memcpy(httpHeader->url, &p[5], strlen(p) - 14);
    }
    printf("%s\n", httpHeader->url);
    p = strtok_s(NULL, delim, &ptr);//提取第二行
    while (p) {
        switch (p[0]) {
        case 'H'://Host
            memcpy(httpHeader->host, &p[6], strlen(p) - 6);//将Host复制到httpHeader->host中
            break;
        case 'C'://Cookie
            if (strlen(p) > 8) //
            {
                char header[8];
                ZeroMemory(header, sizeof(header));
                memcpy(header, p, 6);//将p的前6个字符复制到header中
                if (!strcmp(header, "Cookie")) //head与cookie相等
                {
                    memcpy(httpHeader->cookie, &p[8], strlen(p) - 8);
                }
            }
            break;
        default:
            break;
        }
        p = strtok_s(NULL, delim, &ptr);//读取下一行
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
BOOL ConnectToServer(SOCKET* serverSocket, char* host) {
    sockaddr_in serverAddr;//定义了一个sockaddr_in结构体变量，用于存储服务器端的IP地址和端口号
    serverAddr.sin_family = AF_INET;//使用IPv4协议
    serverAddr.sin_port = htons(HTTP_PORT);//将端口号从主机字节序转换为网络字节序
    HOSTENT* hostent = gethostbyname(host);//获取主机信息
    if (!hostent) {
        return FALSE;
    }
    //将HOSTENT结构体中的IP地址转换为in_addr类型，并将其赋值给serverAddr.sin_addr.s_addr
    in_addr Inaddr = *((in_addr*)*hostent->h_addr_list);
    serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));//IP地址从in_addr类型转换为s_addr类型
    *serverSocket = socket(AF_INET, SOCK_STREAM, 0);//创建套接字
    if (*serverSocket == INVALID_SOCKET)//判断套接字是否有效
    {
        return FALSE;
    }
    if (connect(*serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) //判断套接字是否连接到地址
    {
        closesocket(*serverSocket);
        return FALSE;
    }
    return TRUE;
}
//访问本地文件，获取本地缓存中的日期
void getfileDate(FILE* in, char* tempDate)
{
    char field[5] = "Date";
    //ptr，用于存储strtok_s函数的返回值
    char* p, * ptr, temp[5];//p，用于存储从文件中读取的字符串

    char buffer[MAXSIZE];//存储从文件中读取的字符串
    ZeroMemory(buffer, MAXSIZE);
    fread(buffer, sizeof(char), MAXSIZE, in);//从文件中读取字符串，并将其存储在buffer数组中
    const char* delim = "\r\n";//换行符
    ZeroMemory(temp, 5);
    p = strtok_s(buffer, delim, &ptr);//使用strtok_s函数从buffer数组按行分割字符串，并将第一行存储在p中
    //printf("p = %s\n", p);
    int len = strlen(field) + 2;
    while (p) //如果p指针指向的字符串包含"Date"字符串，则使用memcpy函数将日期信息复制到tempDate数组中，并返回。
        //如果p指针指向的字符串不包含"Date"字符串，则继续遍历下一个字符串
    {
        if (strstr(p, field) != NULL) {//调用strstr后指针会指向匹配剩余的第一个字符
            memcpy(tempDate, &p[len], strlen(p) - len);
            return;
        }
        p = strtok_s(NULL, delim, &ptr);
    }
}

//改造HTTP请求报文
void sendnewHTTP(char* buffer, char* datestring) {
    const char* field = "Host";
    const char* newfield = "If-Modified-Since: ";//分别用于表示请求报文段中的Host字段和要插入的新字段
    //const char *delim = "\r\n";
    char temp[MAXSIZE];//存储插入新字段后的请求报文
    ZeroMemory(temp, MAXSIZE);
    char* pos = strstr(buffer, field);//获取请求报文段中Host后的部分信息
    int i = 0;
    for (i = 0; i < strlen(pos); i++) {
        temp[i] = pos[i];//将pos复制给temp
    }
    *pos = '\0';
    //将pos指针指向Host字段后的第一个字符，然后遍历新字段，将新字段中的每个字符插入到pos指针指向的位置
    while (*newfield != '\0') {  //插入If-Modified-Since字段
        *pos++ = *newfield++;
    }
    while (*datestring != '\0') {//插入对象文件的最新被修改时间
        *pos++ = *datestring++;
    }
    *pos++ = '\r';//符合报文格式
    *pos++ = '\n';
    for (i = 0; i < strlen(temp); i++)//将原始请求报文段中的剩余字符复制到新字段之后
    {
        *pos++ = temp[i];
    }
}

//根据url构造文件名
void makeFilename(char* url, char* filename) {
    while (*url != '\0') {
        if ('a' <= *url && *url <= 'z') {
            *filename++ = *url;//如果当前字符在'a'到'z'的范围内，将其复制到文件名字符串中
        }
        url++;
    }
    strcat_s(filename, strlen(filename) + 9, "cac.txt");
}

//检测主机返回的状态码，如果是200则把数据进行本地更新缓存
void storefileCache(char* buffer, char* url) {
    char* p, * ptr, tempBuffer[MAXSIZE + 1];

    const char* delim = "\r\n";
    ZeroMemory(tempBuffer, MAXSIZE + 1);
    memcpy(tempBuffer, buffer, strlen(buffer));
    p = strtok_s(tempBuffer, delim, &ptr);//提取第一行

    if (strstr(tempBuffer, "200") != NULL) {  //状态码是200时缓存
        char filename[100] = { 0 };
        makeFilename(url, filename);
        printf("filename : %s\n", filename);
        FILE* out;
        fopen_s(&out, filename, "w+");
        fwrite(buffer, sizeof(char), strlen(buffer), out);//使用fopen_s函数以写入模式打开文件，并将响应内容写入文件
        fclose(out);
        printf("\n===================更新缓存ok==================\n");
    }
}

//检测主机返回的状态码，如果是304则从本地获取缓存进行转发，否则需要更新缓存
void checkfileCache(char* buffer, char* filename)
{
    char* p, * ptr, tempBuffer[MAXSIZE + 1];
    const char* delim = "\r\n";
    ZeroMemory(tempBuffer, MAXSIZE + 1);
    memcpy(tempBuffer, buffer, strlen(buffer));//将buffer复制到tempBuffer中
    p = strtok_s(tempBuffer, delim, &ptr);//提取状态码所在行
    //主机返回的报文中的状态码为304时返回已缓存的内容
    if (strstr(p, "304") != NULL) {
        printf("\n=================从本机获得缓存====================\n");
        ZeroMemory(buffer, strlen(buffer));
        FILE* in = NULL;
        if ((fopen_s(&in, filename, "r")) == 0) {
            fread(buffer, sizeof(char), MAXSIZE, in);//使用fopen_s函数以读取模式打开文件，并将文件内容存储在buffer数组中
            fclose(in);
        }
        needCache = FALSE;
    }
}