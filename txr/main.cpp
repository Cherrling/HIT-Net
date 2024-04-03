#include <stdio.h>
#include <Windows.h>
#include <process.h>
#include <string.h>
#include <tchar.h> 
#pragma comment(lib,"Ws2_32.lib")
#define MAXSIZE 65507 //�������ݱ��ĵ���󳤶�
#define HTTP_PORT 80 //http �������˿�
//Http��Ҫͷ������
struct HttpHeader {
    char method[4]; // POST ���� GET��ע����ЩΪ CONNECT����ʵ���ݲ�����
    char url[1024]; // ����� url
    char host[1024]; // Ŀ������
    char cookie[1024 * 10]; //cookie
    HttpHeader() {
        ZeroMemory(this, sizeof(HttpHeader));
    };
};
//��ֹ���ʵ���վ�͵�����վ�Ƿ��������ѡ��
char Invilid_web[1024] = "http://today.hit.edu.cn/";//��������ʵ���վ

char Target_web[1024] = "http://ids-hit-edu-cn.ivpn.hit.edu.cn";//����ԭ��վ
char Fish_web[1024] = "http://jwes.hit.edu.cn/";//������վ
char Fish_host[1024] = "jwes.hit.edu.cn"; //����������

char InvalidIP[] = "127.0.0.1";//���ε��û�IP
BOOL InitSocket();
void ParseHttpHead(char* buffer, HttpHeader* httpHeader);
BOOL ConnectToServer(SOCKET* serverSocket, char* host);
unsigned int __stdcall ProxyThread(LPVOID lpParameter);
//������ز���
SOCKET ProxyServer;//���������
sockaddr_in ProxyServerAddr;//�����������ַ
const int ProxyPort = 10240;//���ô�����
int addrlen = 32;

//������ز���
boolean haveCache = false;
boolean needCache = true;
void getfileDate(FILE* in, char* tempDate);
void sendnewHTTP(char* buffer, char* datestring);
void makeFilename(char* url, char* filename);
void storefileCache(char* buffer, char* url);
void checkfileCache(char* buffer, char* filename);
//�����µ����Ӷ�ʹ�����߳̽��д������̵߳�Ƶ���Ĵ����������ر��˷���Դ
//����ʹ���̳߳ؼ�����߷�����Ч��
//const int ProxyThreadMaxNum = 20;
//HANDLE ProxyThreadHandle[ProxyThreadMaxNum] = {0};
//DWORD ProxyThreadDW[ProxyThreadMaxNum] = {0};;;

struct ProxyParam {
    SOCKET clientSocket;
    SOCKET serverSocket;
};

//������
int _tmain(int argc, _TCHAR* argv[])
{
    printf("�����������������\n");
    printf("��ʼ��...\n");
    if (!InitSocket()) {
        printf("socket ��ʼ��ʧ��\n");
        return -1;
    }
    printf("����������������У������˿� %d\n", ProxyPort);
    SOCKET acceptSocket = INVALID_SOCKET;
    //��socket���ó���Ч�׽���
    SOCKADDR_IN acceptAddr; //�Զ����������������û���IP
    ProxyParam* lpProxyParam;
    HANDLE hThread;
    DWORD dwThreadID;//unsigned long���޷���32λ����
    //������������ϼ���
    while (TRUE) {
        acceptSocket = accept(ProxyServer, (SOCKADDR*)&acceptAddr, &addrlen);
        printf("===============�û�IP��ַΪ%s===============\n", inet_ntoa(acceptAddr.sin_addr));
        // if (strcmp(inet_ntoa(acceptAddr.sin_addr), InvalidIP) == 0)//�����û�ip
        // {
        //     printf("\n\n===============���û��Ѿ�������===============\n\n");
        // }
        // else {
        lpProxyParam = new ProxyParam;//ÿ�����󶼴���һ���µ��߳�������
        if (lpProxyParam == NULL) {
            continue;
        }
        lpProxyParam->clientSocket = acceptSocket;
        //�߳̿�ʼ
        hThread = (HANDLE)_beginthreadex(NULL, 0, &ProxyThread, (LPVOID)lpProxyParam, 0, 0);
        CloseHandle(hThread);
        // }
        Sleep(2000);
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
BOOL InitSocket() {
    //�����׽��ֿ⣨���룩

    WORD wVersionRequested;
    WSADATA wsaData;
    //�׽��ּ���ʱ������ʾ
    int err;
    //�汾 2.2
    wVersionRequested = MAKEWORD(2, 2);
    //���� dll �ļ� Scoket ��
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        //�Ҳ��� winsock.dll
        printf("���� winsock ʧ�ܣ��������Ϊ: %d\n", WSAGetLastError());
        return FALSE;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        printf("�����ҵ���ȷ�� winsock �汾\n");
        WSACleanup();
        return FALSE;
    }
    //�����׽���
    ProxyServer = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == ProxyServer) {
        printf("�����׽���ʧ�ܣ��������Ϊ�� %d\n", WSAGetLastError());
        return FALSE;
    }
    ProxyServerAddr.sin_family = AF_INET;//��ַ��
    ProxyServerAddr.sin_port = htons(ProxyPort); // ���ô���˿�
    ProxyServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//����IP��ַ
    //bind��
    if (bind(ProxyServer, (SOCKADDR*)&ProxyServerAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        printf("���׽���ʧ��\n");
        return FALSE;
    }
    //listen������SOMAXCONN��ϵͳ������������г���
    if (listen(ProxyServer, SOMAXCONN) == SOCKET_ERROR) {
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
//************************************
unsigned int __stdcall ProxyThread(LPVOID lpParameter) {
    char Buffer[MAXSIZE];
    char* CacheBuffer;
    char* DateBuffer;
    char filename[100] = { 0 };
    // _Post_ _Notnull_ 
    FILE* in;
    char date_str[30];  //�����ֶ�Date��ֵ
    ZeroMemory(Buffer, MAXSIZE);
    SOCKADDR_IN clientAddr;
    int length = sizeof(SOCKADDR_IN);
    int recvSize;
    int ret;
    FILE* fp;
    //��һ�ν��տͻ������󣬽������󻺴��������浽�����ļ���
    recvSize = recv(((ProxyParam*)lpParameter)->clientSocket, Buffer, MAXSIZE, 0);
    HttpHeader* httpHeader = new HttpHeader();
    if (recvSize <= 0) {
        goto error;
    }
    CacheBuffer = new char[recvSize + 1];
    ZeroMemory(CacheBuffer, recvSize + 1);
    memcpy(CacheBuffer, Buffer, recvSize);

    ParseHttpHead(CacheBuffer, httpHeader);     //����HTTP����ͷ��
    //printf("HTTP���������£�\n%s\n", Buffer);
    ZeroMemory(date_str, 30);
    printf("httpHeader->url : %s\n", httpHeader->url);
    makeFilename(httpHeader->url, filename);
    //printf("filename�� %s\n", filename);
    if ((fopen_s(&in, filename, "r")) == 0) {
        printf("\n�л���\n");
        //fread_s(fileBuffer, MAXSIZE, sizeof(char), MAXSIZE, in);
        getfileDate(in, date_str);//�õ����ػ����ļ��е�����date_str
        fclose(in);
        //printf("date_str:%s\n", date_str);
        sendnewHTTP(Buffer, date_str);
        //�����������һ�����󣬸�������Ҫ���� ��If-Modified-Since�� �ֶ�
        //������ͨ���Ա�ʱ�����жϻ����Ƿ����
        haveCache = TRUE;
    }
    //printf("httpHeader��url��%s����������ʵ���%s\n", httpHeader->url, Invilid_web);
    //��վ���˹���
    if (strcmp(httpHeader->url, Invilid_web) == 0) {
        printf("%s��վ���ܾ�����\n", Invilid_web);
        goto error;
    }
    //��ӵ��㹦��
    if (strstr(httpHeader->url, Target_web) != NULL) {
        printf("%s��վ����ɹ�����ת����%s\n", Target_web, Fish_web);
        memcpy(httpHeader->host, Fish_host, strlen(Fish_host) + 1);//�滻������
        memcpy(httpHeader->url, Fish_web, strlen(Fish_web) + 1);//�滻url
    }
    //��ʱ���ݱ��洢����httpHeader��
    delete CacheBuffer;
    //���ӷ������ݱ����ڵķ�����
    if (!ConnectToServer(&((ProxyParam*)lpParameter)->serverSocket, httpHeader->host)) {
        printf("����Ŀ�ķ�����ʧ��\n");
        goto error;
    }
    printf("������������ %s �ɹ�\n", httpHeader->host);
    //���ͻ��˷��͵� HTTP ���ݱ���ֱ��ת����Ŀ�������
    ret = send(((ProxyParam*)lpParameter)->serverSocket, Buffer, strlen(Buffer)
        + 1, 0);
    //�ȴ�Ŀ���������������
    recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buffer, MAXSIZE, 0);
    if (recvSize <= 0) {
        goto error;
    }
    //printf("��������Ӧ�������£�\n%s\n", Buffer);
    if (haveCache == true) {
        checkfileCache(Buffer, httpHeader->url);
    }
    if (needCache == true) {
        storefileCache(Buffer, httpHeader->url);
    }
    //��Ŀ����������ص�����ֱ��ת�����ͻ���
    ret = send(((ProxyParam*)lpParameter)->clientSocket, Buffer, sizeof(Buffer), 0);
    //������
error:
    printf("�ر��׽���\n\n");
    Sleep(200);
    closesocket(((ProxyParam*)lpParameter)->clientSocket);
    closesocket(((ProxyParam*)lpParameter)->serverSocket);
    delete lpParameter;
    _endthreadex(0); //��ֹ�߳�
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
void ParseHttpHead(char* buffer, HttpHeader* httpHeader) {
    char* p;
    char* ptr;
    const char* delim = "\r\n";
    p = strtok_s(buffer, delim, &ptr);//��ȡ��һ��
    printf("%s\n", p);
    if (p[0] == 'G') {//GET ��ʽ
        memcpy(httpHeader->method, "GET", 3);
        memcpy(httpHeader->url, &p[4], strlen(p) - 13);
    }
    else if (p[0] == 'P') {//POST ��ʽ
        memcpy(httpHeader->method, "POST", 4);
        memcpy(httpHeader->url, &p[5], strlen(p) - 14);
    }
    printf("%s\n", httpHeader->url);
    p = strtok_s(NULL, delim, &ptr);
    while (p) {
        switch (p[0]) {
        case 'H'://Host
            memcpy(httpHeader->host, &p[6], strlen(p) - 6);
            break;
        case 'C'://Cookie
            if (strlen(p) > 8) {
                char header[8];
                ZeroMemory(header, sizeof(header));
                memcpy(header, p, 6);
                if (!strcmp(header, "Cookie")) {
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
BOOL ConnectToServer(SOCKET* serverSocket, char* host) {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(HTTP_PORT);
    HOSTENT* hostent = gethostbyname(host);
    if (!hostent) {
        return FALSE;
    }
    in_addr Inaddr = *((in_addr*)*hostent->h_addr_list);
    serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));
    *serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (*serverSocket == INVALID_SOCKET) {
        return FALSE;
    }
    if (connect(*serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr))
        == SOCKET_ERROR) {
        closesocket(*serverSocket);
        return FALSE;
    }
    return TRUE;
}
//���ʱ����ļ�����ȡ���ػ����е�����
void getfileDate(FILE* in, char* tempDate) {
    char field[5] = "Date";
    char* p, * ptr, temp[5];
    char buffer[MAXSIZE];
    ZeroMemory(buffer, MAXSIZE);
    fread(buffer, sizeof(char), MAXSIZE, in);
    const char* delim = "\r\n";//���з�
    ZeroMemory(temp, 5);
    p = strtok_s(buffer, delim, &ptr);
    int len = strlen(field) + 2;
    while (p) {
        if (strstr(p, field) != NULL) {//����strstr��ָ���ָ��ƥ��ʣ��ĵ�һ���ַ�
            memcpy(tempDate, &p[len], strlen(p) - len);
            return;
        }
        p = strtok_s(NULL, delim, &ptr);
    }
}

//����HTTP������
void sendnewHTTP(char* buffer, char* datestring) {
    const char* field = "Host";
    const char* newfield = "If-Modified-Since: ";
    //const char *delim = "\r\n";
    char temp[MAXSIZE];
    ZeroMemory(temp, MAXSIZE);
    char* pos = strstr(buffer, field);//��ȡ�����Ķ���Host��Ĳ�����Ϣ
    int i = 0;
    for (i = 0; i < strlen(pos); i++) {
        temp[i] = pos[i];//��pos���Ƹ�temp
    }
    *pos = '\0';
    while (*newfield != '\0') {  //����If-Modified-Since�ֶ�
        *pos++ = *newfield++;
    }
    while (*datestring != '\0') {//��������ļ������±��޸�ʱ��
        *pos++ = *datestring++;
    }
    *pos++ = '\r';
    *pos++ = '\n';
    for (i = 0; i < strlen(temp); i++) {
        *pos++ = temp[i];
    }
}

//����url�����ļ���
void makeFilename(char* url, char* filename) {
    while (*url != '\0') {
        if ('a' <= *url && *url <= 'z') {
            *filename++ = *url;
        }
        url++;
    }
    strcat_s(filename, strlen(filename) + 9, "110.txt");
}

//����������ص�״̬�룬�����200�򱾵ػ�ȡ����
void storefileCache(char* buffer, char* url) {
    char* p, * ptr, tempBuffer[MAXSIZE + 1];
    //num����״̬��
    const char* delim = "\r\n";
    ZeroMemory(tempBuffer, MAXSIZE + 1);
    memcpy(tempBuffer, buffer, strlen(buffer));
    p = strtok_s(tempBuffer, delim, &ptr);//��ȡ��һ��
    //printf("tempbuffer = %s\n", p);
    if (strstr(tempBuffer, "200") != NULL) {  //״̬����200ʱ����
        char filename[100] = { 0 };
        makeFilename(url, filename);
        printf("filename : %s\n", filename);
        FILE* out;
        fopen_s(&out, filename, "w+");
        fwrite(buffer, sizeof(char), strlen(buffer), out);
        fclose(out);
        printf("\n===================��ҳ�Ѿ�������==================\n");
    }
}

//����������ص�״̬�룬�����304��ӱ��ػ�ȡ�������ת����������Ҫ���»���
void checkfileCache(char* buffer, char* filename) {
    char* p, * ptr, tempBuffer[MAXSIZE + 1];
    const char* delim = "\r\n";
    ZeroMemory(tempBuffer, MAXSIZE + 1);
    memcpy(tempBuffer, buffer, strlen(buffer));
    p = strtok_s(tempBuffer, delim, &ptr);//��ȡ״̬��������
    //�������صı����е�״̬��Ϊ304ʱ�����ѻ��������
    if (strstr(p, "304") != NULL) {
        printf("\n=================�ӱ�����û���====================\n");
        ZeroMemory(buffer, strlen(buffer));
        FILE* in = NULL;
        if ((fopen_s(&in, filename, "r")) == 0) {
            fread(buffer, sizeof(char), MAXSIZE, in);
            fclose(in);
        }
        needCache = FALSE;
    }
}