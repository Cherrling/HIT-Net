#include <stdio.h>
#include <Windows.h>
#include <process.h>
#include <string.h>
#include <tchar.h> 
#pragma comment(lib,"Ws2_32.lib")
#define MAXSIZE 65507 //�������ݱ��ĵ���󳤶�
#define HTTP_PORT 80 //http �������˿�
#define tt 1
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
char Invilid_web[1024] = "http://hit.edu.cn/";//��������ʵ���վ

char Target_web[1024] = "http://ivpn.hit.edu.cn";//����ԭ��վ
char Fish_web[1024] = "http://jwts.hit.edu.cn/";//������վ
char Fish_host[1024] = "jwts.hit.edu.cn"; //����������

char InvalidIP[] = "128.0.0.1";//���ε��û�IP
BOOL InitSocket();//��ʼ���׽���
void ParseHttpHead(char* buffer, HttpHeader* httpHeader);//����HTTP����ͷ��
BOOL ConnectToServer(SOCKET* serverSocket, char* host);//���ӵ�������
unsigned int __stdcall ProxyThread(LPVOID lpParameter);//�����߳�ִ�к���,__stdcall�ؼ��֣���ʾʹ�ñ�׼����Լ��
//������ز���
SOCKET ProxyServer;//���������
sockaddr_in ProxyServerAddr;//�����������ַ
const int ProxyPort = 65432;//���ô�����
int addrlen = 32;

//������ز���
boolean haveCache = false;//��ʾ�Ƿ��Ѿ��������ļ�
boolean needCache = true;
void getfileDate(FILE* in, char* tempDate);//���ļ��ж�ȡ������Ϣ������������Ϣ�洢��tempDate��
void sendnewHTTP(char* buffer, char* datestring);//����һ���µ�HTTP���󣬲�����Ӧ�洢��buffer��
void makeFilename(char* url, char* filename);//����URL�����ļ���
void storefileCache(char* buffer, char* url);//���ļ����ݴ洢��������
void checkfileCache(char* buffer, char* filename);//��黺�����Ƿ���ڸ��ļ���������ڣ����ļ����ݷ���
//�����µ����Ӷ�ʹ�����߳̽��д������̵߳�Ƶ���Ĵ����������ر��˷���Դ
//����ʹ���̳߳ؼ�����߷�����Ч��
//const int ProxyThreadMaxNum = 20;
//HANDLE ProxyThreadHandle[ProxyThreadMaxNum] = {0};
//DWORD ProxyThreadDW[ProxyThreadMaxNum] = {0};;;



//�̳߳���ؽṹ��ͺ���
const int MAX_THREADS = 20; // ����߳�����
HANDLE ThreadPool[MAX_THREADS]; // �̳߳�����
unsigned int ThreadPoolID[MAX_THREADS]; // �̳߳��߳�ID����
HANDLE ThreadPoolMutex; // �����������ڱ����̳߳صĲ���
int ThreadPoolIndex = 0; // �̳߳�����


// �����߳�״̬��ö��
enum ThreadStatus {
    RUNNING,
    IDLE,
    TERMINATED
};
struct ThreadPoolTask {
    SOCKET clientSocket;
    SOCKET serverSocket;
    ThreadStatus status; // �߳�״̬
};

// �̳߳س�ʼ��
void InitThreadPool() {
    ThreadPoolMutex = CreateMutex(NULL, FALSE, NULL);
}

// �ύ�����̳߳�
void SubmitTaskToThreadPool(ThreadPoolTask* task) {
    WaitForSingleObject(ThreadPoolMutex, INFINITE); // ��ȡ������
    if (ThreadPoolIndex < MAX_THREADS) {
        task->status = RUNNING; // ��������״̬Ϊ������
        ThreadPool[ThreadPoolIndex] = (HANDLE)_beginthreadex(NULL, 0, ProxyThread, (LPVOID)task, 0, &ThreadPoolID[ThreadPoolIndex]);
        ThreadPoolIndex++;
    }
    ReleaseMutex(ThreadPoolMutex); // �ͷŻ�����
}

// �ͷ��߳���Դ
void ReleaseThreadResource(ThreadPoolTask* task) {
    task->status = TERMINATED; // ��������״̬Ϊ��ֹ
    _endthreadex(0); // ��ֹ�߳�
    WaitForSingleObject(ThreadPoolMutex, INFINITE); // ��ȡ������
    ThreadPoolIndex--; // �����̳߳�����
    ReleaseMutex(ThreadPoolMutex); // �ͷŻ�����
}



struct ProxyParam {
    SOCKET clientSocket;
    SOCKET serverSocket;
};//�洢�����������ͻ��˺ͷ�����֮���������Ϣ

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
    SOCKET acceptSocket = INVALID_SOCKET;//���ڴ洢���ܵ��Ŀͻ����׽��֣���ʼֵΪ INVALID_SOCKET.�����������յ��ͻ��˵���������ʱ���ᴴ��һ���µ��׽��֣������丳ֵ�� acceptSocket
    //��socket���ó���Ч�׽���
    SOCKADDR_IN acceptAddr; //�Զ����������������û���IP�����������һ���ṹ�壬���ڴ洢�ͻ��˵�IP��ַ�Ͷ˿ں�
    ProxyParam* lpProxyParam;
    HANDLE hThread;//���ڴ洢���ܿͻ������ӵ��߳̾��
    DWORD dwThreadID;//unsigned long���޷���32λ����,�߳� ID ���̵߳�Ψһ��ʶ��,�洢���ܿͻ������ӵ��߳� ID
    //������������ϼ���
    while (TRUE) {//ʹ��accept()�������ܿͻ��˵��������󣬲�����һ���µ��׽���acceptSocket����ͨ�š�
        //ͬʱ�����ͻ��˵�IP��ַ�洢��acceptAddr�ṹ���У�����ȡ�ͻ��˵�IP��ַ�ַ���
        acceptSocket = accept(ProxyServer, (SOCKADDR*)&acceptAddr, &addrlen);
        printf("===============�û�IP��ַΪ%s===============\n", inet_ntoa(acceptAddr.sin_addr));
        if (strcmp(inet_ntoa(acceptAddr.sin_addr), InvalidIP) == 0)//�����û�ip
        {
            printf("\n\n===============���û����������===============\n\n");
        }
        else {
            if (tt==0)
            {

                lpProxyParam = new ProxyParam;//ÿ�����󶼴���һ���µ��߳�������
                if (lpProxyParam == NULL) //���lpProxyParamΪ�գ���������ǰѭ�������������µ���������
                {
                    continue;
                }
                lpProxyParam->clientSocket = acceptSocket;
                //�߳̿�ʼ
                //����һ���µ��߳�����������ܣ����� ProxyParam �ṹ����Ϊ����
                hThread = (HANDLE)_beginthreadex(NULL, 0, &ProxyThread, (LPVOID)lpProxyParam, 0, 0);
                CloseHandle(hThread);//�ر��߳̾��
            }
            else
            {

                ThreadPoolTask* task = new ThreadPoolTask;
                task->clientSocket = acceptSocket;
                SubmitTaskToThreadPool(task); // �ύ�����̳߳ش���

            }


        }
        Sleep(200);
    }
    closesocket(ProxyServer);//�رմ�����������׽���
    WSACleanup();//���� Winsock �����Դ
    return 0;
}
//************************************
// Method: InitSocket
// FullName: InitSocket
// Access: public
// Returns: BOOL
// Qualifier: ��ʼ���׽���
//************************************

BOOL InitSocket() //��ʼ���׽���
{
    //�����׽��ֿ⣨���룩
    WORD wVersionRequested;//16λ�������ͣ����ڱ�ʾ�汾��
    WSADATA wsaData;//����һ���ṹ�����ͣ����ڴ洢Winsock��İ汾��Ϣ���׽���ѡ���
    //�׽��ּ���ʱ������ʾ
    int err;
    //�汾 2.2
    wVersionRequested = MAKEWORD(2, 2);//ָ��Ҫ���ص�Winsock��İ汾�����룩
    //���� dll �ļ� Scoket ��
    err = WSAStartup(wVersionRequested, &wsaData);//���óɹ��򷵻�0
    if (err != 0)
    {
        //�Ҳ��� winsock.dll
        printf("���� winsock ʧ�ܣ��������Ϊ: %d\n", WSAGetLastError());
        return FALSE;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)//�汾����2.2
    {
        printf("�����ҵ���ȷ�� winsock �汾\n");
        WSACleanup();//����WSACleanup()����������Windows Socket��
        return FALSE;
    }
    //�����׽���
    ProxyServer = socket(AF_INET, SOCK_STREAM, 0);//AF_INET ������ʾʹ�� IPv4 ��ַ��,SOCK_STREAM ����ʹ�� TCP Э�����ͨ��,0 ������ʾʹ��Ĭ�ϵĴ���Э��
    if (INVALID_SOCKET == ProxyServer)//INVALID_SOCKET ��һ���꣬���ڱ�ʾ��Ч���׽��֡�INVALID_SOCKET ��ֵΪ -1
    {
        printf("�����׽���ʧ�ܣ��������Ϊ�� %d\n", WSAGetLastError());
        return FALSE;//WSAGetLastError() ������ȡ���һ�η����� Winsock �������
    }
    //���ô���������ĵ�ַ��Ϣ��������ַ�塢�˿ںź� IP ��ַ
    ProxyServerAddr.sin_family = AF_INET;//��ַ��ipv4
    ProxyServerAddr.sin_port = htons(ProxyPort); // ������˿�ת��Ϊ�����ֽ�˳�򣬲���������ΪProxyServerAddr�ṹ���sin_port�ֶ�
    ProxyServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//����IP��ַ
    //��һ���׽��֣�ProxyServer���󶨵�һ��ָ���ĵ�ַ��ProxyServerAddr���������ʧ�ܣ�������������׽���ʧ�ܡ�������FALSE
    if (bind(ProxyServer, (SOCKADDR*)&ProxyServerAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        printf("���׽���ʧ��\n");
        return FALSE;
    }
    //�����ĵ�һ�����������ڼ������׽��֣��ڶ���������������г��ȣ����ȴ����ӵĿͻ��������������������������µ��������󽫱��ܾ�
    //listen������SOMAXCONN��ϵͳ������������г���,5
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
unsigned int __stdcall ProxyThread(LPVOID lpParameter)
{
    ThreadPoolTask* task;
    if (tt)
    {
        task = (ThreadPoolTask*)lpParameter;

    }
    char Buffer[MAXSIZE];//�洢�ͻ��˷��͵� HTTP ���ݱ���
    char* CacheBuffer;//ָ�򻺴��������������ڴ洢���������ݰ��н�����������
    char* DateBuffer;//ָ�����ڻ����������ڻ��������ڴ洢���������ݰ��н�����������ֵ
    char filename[100] = { 0 };
    _Post_ _Notnull_ FILE* in;//������һ���ļ�ָ�룬����ָ�������ļ���_Post_ _Notnull_��һ���꣬��ʾ�ں�������֮��inָ����벻Ϊ��
    char date_str[30];  //������һ���ַ����飬���ڴ洢�����ַ����������ַ���ͨ�����ꡢ�¡�����ɣ����� "2021-08-01"
    ZeroMemory(Buffer, MAXSIZE);//��Buffer���������Ԫ������Ϊ0
    SOCKADDR_IN clientAddr;//������һ��SOCKADDR_IN�ṹ����������ڴ洢�ͻ��˵�IP��ַ�Ͷ˿ں�
    int length = sizeof(SOCKADDR_IN);
    int recvSize;//���ڴ洢���յ����������ݰ��Ĵ�С
    int ret;//���ڴ洢��������ֵ
    FILE* fp;
    //��һ�ν��տͻ������󣬽������󻺴��������浽�����ļ���
    //ʹ��recv�����ӿͻ����׽���((ProxyParam*)lpParameter)->clientSocket�н�������
    //���ݴ洢��Buffer�����У������ճ���ΪMAXSIZE������ģʽΪ0��recvSize��ʾ���յ������ݴ�С
    recvSize = recv(((ProxyParam*)lpParameter)->clientSocket, Buffer, MAXSIZE, 0);
    HttpHeader* httpHeader = new HttpHeader();
    if (recvSize <= 0) //�жϽ��յ������ݴ�С�Ƿ�С�ڵ���0������ǣ����ʾ��������ʧ��
    {
        goto error;
    }
    CacheBuffer = new char[recvSize + 1];
    ZeroMemory(CacheBuffer, recvSize + 1);
    memcpy(CacheBuffer, Buffer, recvSize);//��buffer���յ������ݸ��Ƶ�CacheBuffer��

    ParseHttpHead(CacheBuffer, httpHeader);     //����HTTP����ͷ��
    //printf("HTTP���������£�\n%s\n", Buffer);
    ZeroMemory(date_str, 30);
    printf("httpHeader->url : %s\n", httpHeader->url);
    makeFilename(httpHeader->url, filename);
    //printf("filename�� %s\n", filename);
    if ((fopen_s(&in, filename, "r")) == 0)
    {
        printf("\n�л���\n");

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
    ret = send(((ProxyParam*)lpParameter)->serverSocket, Buffer, strlen(Buffer) + 1, 0);
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
    if (tt)
    {
        ReleaseThreadResource(task); // �ͷ��߳���Դ
    }
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
void ParseHttpHead(char* buffer, HttpHeader* httpHeader)
{
    char* p;//�洢��ȡ����ÿһ���ַ���
    char* ptr;//ָ��ǰ�е�ָ��
    const char* delim = "\r\n";//�ָ���
    p = strtok_s(buffer, delim, &ptr);//��ȡ��һ��
    printf("%s\n", p);
    if (p[0] == 'G') {//GET ��ʽ
        //��GET���Ƶ�httpHeader->method�У������ַ���p�дӵ�5���ַ���ʼ���Ӵ����Ƶ�httpHeader->url��
        memcpy(httpHeader->method, "GET", 3);
        memcpy(httpHeader->url, &p[4], strlen(p) - 13);
    }
    else if (p[0] == 'P') {//POST ��ʽ
        //��POST���Ƶ�httpHeader->method�У������ַ���p�дӵ�6���ַ���ʼ���Ӵ����Ƶ�httpHeader->url��
        memcpy(httpHeader->method, "POST", 4);
        memcpy(httpHeader->url, &p[5], strlen(p) - 14);
    }
    printf("%s\n", httpHeader->url);
    p = strtok_s(NULL, delim, &ptr);//��ȡ�ڶ���
    while (p) {
        switch (p[0]) {
        case 'H'://Host
            memcpy(httpHeader->host, &p[6], strlen(p) - 6);//��Host���Ƶ�httpHeader->host��
            break;
        case 'C'://Cookie
            if (strlen(p) > 8) //
            {
                char header[8];
                ZeroMemory(header, sizeof(header));
                memcpy(header, p, 6);//��p��ǰ6���ַ����Ƶ�header��
                if (!strcmp(header, "Cookie")) //head��cookie���
                {
                    memcpy(httpHeader->cookie, &p[8], strlen(p) - 8);
                }
            }
            break;
        default:
            break;
        }
        p = strtok_s(NULL, delim, &ptr);//��ȡ��һ��
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
    sockaddr_in serverAddr;//������һ��sockaddr_in�ṹ����������ڴ洢�������˵�IP��ַ�Ͷ˿ں�
    serverAddr.sin_family = AF_INET;//ʹ��IPv4Э��
    serverAddr.sin_port = htons(HTTP_PORT);//���˿ںŴ������ֽ���ת��Ϊ�����ֽ���
    HOSTENT* hostent = gethostbyname(host);//��ȡ������Ϣ
    if (!hostent) {
        return FALSE;
    }
    //��HOSTENT�ṹ���е�IP��ַת��Ϊin_addr���ͣ������丳ֵ��serverAddr.sin_addr.s_addr
    in_addr Inaddr = *((in_addr*)*hostent->h_addr_list);
    serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));//IP��ַ��in_addr����ת��Ϊs_addr����
    *serverSocket = socket(AF_INET, SOCK_STREAM, 0);//�����׽���
    if (*serverSocket == INVALID_SOCKET)//�ж��׽����Ƿ���Ч
    {
        return FALSE;
    }
    if (connect(*serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) //�ж��׽����Ƿ����ӵ���ַ
    {
        closesocket(*serverSocket);
        return FALSE;
    }
    return TRUE;
}
//���ʱ����ļ�����ȡ���ػ����е�����
void getfileDate(FILE* in, char* tempDate)
{
    char field[5] = "Date";
    //ptr�����ڴ洢strtok_s�����ķ���ֵ
    char* p, * ptr, temp[5];//p�����ڴ洢���ļ��ж�ȡ���ַ���

    char buffer[MAXSIZE];//�洢���ļ��ж�ȡ���ַ���
    ZeroMemory(buffer, MAXSIZE);
    fread(buffer, sizeof(char), MAXSIZE, in);//���ļ��ж�ȡ�ַ�����������洢��buffer������
    const char* delim = "\r\n";//���з�
    ZeroMemory(temp, 5);
    p = strtok_s(buffer, delim, &ptr);//ʹ��strtok_s������buffer���鰴�зָ��ַ�����������һ�д洢��p��
    //printf("p = %s\n", p);
    int len = strlen(field) + 2;
    while (p) //���pָ��ָ����ַ�������"Date"�ַ�������ʹ��memcpy������������Ϣ���Ƶ�tempDate�����У������ء�
        //���pָ��ָ����ַ���������"Date"�ַ����������������һ���ַ���
    {
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
    const char* newfield = "If-Modified-Since: ";//�ֱ����ڱ�ʾ�����Ķ��е�Host�ֶκ�Ҫ��������ֶ�
    //const char *delim = "\r\n";
    char temp[MAXSIZE];//�洢�������ֶκ��������
    ZeroMemory(temp, MAXSIZE);
    char* pos = strstr(buffer, field);//��ȡ�����Ķ���Host��Ĳ�����Ϣ
    int i = 0;
    for (i = 0; i < strlen(pos); i++) {
        temp[i] = pos[i];//��pos���Ƹ�temp
    }
    *pos = '\0';
    //��posָ��ָ��Host�ֶκ�ĵ�һ���ַ���Ȼ��������ֶΣ������ֶ��е�ÿ���ַ����뵽posָ��ָ���λ��
    while (*newfield != '\0') {  //����If-Modified-Since�ֶ�
        *pos++ = *newfield++;
    }
    while (*datestring != '\0') {//��������ļ������±��޸�ʱ��
        *pos++ = *datestring++;
    }
    *pos++ = '\r';//���ϱ��ĸ�ʽ
    *pos++ = '\n';
    for (i = 0; i < strlen(temp); i++)//��ԭʼ�����Ķ��е�ʣ���ַ����Ƶ����ֶ�֮��
    {
        *pos++ = temp[i];
    }
}

//����url�����ļ���
void makeFilename(char* url, char* filename) {
    while (*url != '\0') {
        if ('a' <= *url && *url <= 'z') {
            *filename++ = *url;//�����ǰ�ַ���'a'��'z'�ķ�Χ�ڣ����临�Ƶ��ļ����ַ�����
        }
        url++;
    }
    strcat_s(filename, strlen(filename) + 9, "cac.txt");
}

//����������ص�״̬�룬�����200������ݽ��б��ظ��»���
void storefileCache(char* buffer, char* url) {
    char* p, * ptr, tempBuffer[MAXSIZE + 1];

    const char* delim = "\r\n";
    ZeroMemory(tempBuffer, MAXSIZE + 1);
    memcpy(tempBuffer, buffer, strlen(buffer));
    p = strtok_s(tempBuffer, delim, &ptr);//��ȡ��һ��

    if (strstr(tempBuffer, "200") != NULL) {  //״̬����200ʱ����
        char filename[100] = { 0 };
        makeFilename(url, filename);
        printf("filename : %s\n", filename);
        FILE* out;
        fopen_s(&out, filename, "w+");
        fwrite(buffer, sizeof(char), strlen(buffer), out);//ʹ��fopen_s������д��ģʽ���ļ���������Ӧ����д���ļ�
        fclose(out);
        printf("\n===================���»���ok==================\n");
    }
}

//����������ص�״̬�룬�����304��ӱ��ػ�ȡ�������ת����������Ҫ���»���
void checkfileCache(char* buffer, char* filename)
{
    char* p, * ptr, tempBuffer[MAXSIZE + 1];
    const char* delim = "\r\n";
    ZeroMemory(tempBuffer, MAXSIZE + 1);
    memcpy(tempBuffer, buffer, strlen(buffer));//��buffer���Ƶ�tempBuffer��
    p = strtok_s(tempBuffer, delim, &ptr);//��ȡ״̬��������
    //�������صı����е�״̬��Ϊ304ʱ�����ѻ��������
    if (strstr(p, "304") != NULL) {
        printf("\n=================�ӱ�����û���====================\n");
        ZeroMemory(buffer, strlen(buffer));
        FILE* in = NULL;
        if ((fopen_s(&in, filename, "r")) == 0) {
            fread(buffer, sizeof(char), MAXSIZE, in);//ʹ��fopen_s�����Զ�ȡģʽ���ļ��������ļ����ݴ洢��buffer������
            fclose(in);
        }
        needCache = FALSE;
    }
}