/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysInclude.h"
#include<string.h>
#include <vector>
using namespace std;
// system support
extern void fwd_LocalRcv(char* pBuffer, int length);

extern void fwd_SendtoLower(char* pBuffer, int length, unsigned int nexthop);

extern void fwd_DiscardPkt(char* pBuffer, int type);

extern unsigned int getIpv4Address();

// implemented by students
struct Route//·�ɱ����
{
    unsigned int mask;//����
    unsigned int dest;//Ŀ�ĵ�ַ
    unsigned int masklen;//���볤��
    unsigned int nexthop;//��һ����ַ

    Route(unsigned int dest, unsigned int masklen, unsigned int nexthop)
    {
        this->dest = ntohl(dest);
        this->masklen = ntohl(masklen);
        this->nexthop = ntohl(nexthop);
        this->mask = 0;
        if (this->masklen)
        {
            this->mask = (int)0x80000000 >> (this->masklen - 1);
        }
    }
};

vector<Route> routeTable; // ·�ɱ�

/**
 * @brief ��·�ɱ����Ƿ��з��ϵı���
 *
 * @param dstAddr Ŀ�ĵ�ַ
 * @param nexthop ��һ����ַ
 * @return bool �Ƿ����
 */
bool find_Next(unsigned dstAddr, unsigned int* nexthop)
{
    int size = routeTable.size();
    for (int i = 0; i < size; i++)
    {
        Route route = routeTable[i];
        if ((route.mask & dstAddr) == route.dest)
        {
            *nexthop = route.nexthop;
            return true;
        }
    }
    return false;
}
/**
 * @brief ����checksum
 *
 * @param pBuffer IP����ͷ
 * @return unsigned short checksumֵ
 */
unsigned short cal_Checksum(char* pBuffer)
{
    unsigned int checkSum = 0;
    for (int i = 0; i < 10; i++)
    {
        checkSum += ((unsigned short*)pBuffer)[i];
    }
    checkSum = (checkSum >> 16) + (checkSum & 0xFFFF);
    checkSum = ~checkSum;
    return checkSum;
}
/**
 * @brief ��ʼ��·�ɱ�(��ʵ�ϱ�ʵ���о������·�ɱ�)
 *
 */
void stud_Route_Init()
{
    routeTable.clear();
}
/**
 * @brief ·�ɱ����ýӿ�
 *
 * @param proute ָ����Ҫ���·����Ϣ�Ľṹ��ͷ��
 */
void stud_route_add(stud_route_msg* proute)
{
    Route route(proute->dest, proute->masklen, proute->nexthop);//����·����A
    routeTable.push_back(route);
}

/**
 * @brief ת������
 *
 * @param pBuffer ָ����յ��� IPv4 ����ͷ��
 * @param length IPv4 ����ĳ���
 * @return int 0 Ϊ�ɹ��� 1 Ϊʧ�ܣ�
 */
int stud_fwd_deal(char* pBuffer, int length)
{
    unsigned char ttl = pBuffer[0];
    //ttl�Ѿ�Ϊ0��
    if (ttl == 0)
    {
        fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR);
        return 1;
    }
    //����Ǳ�����ֱַ�ӷ���
    unsigned int dstAddr = ntohl(((unsigned int*)pBuffer)[4]);
    if (dstAddr == getIpv4Address())
    {
        fwd_LocalRcv(pBuffer, length);
        return 0;
    }
    //���Ǳ�����ַ������һ��
    unsigned int* nexthop = new unsigned int;
    //�����д��ڣ�����
    if (find_Next(dstAddr, nexthop))
    {
        //ע��Ҫ�޸�ttl��checksum
        pBuffer[8]--;//ttl-=1
        ((unsigned short*)pBuffer)[5] = 0;
        ((unsigned short*)pBuffer)[5] = cal_Checksum(pBuffer);
        fwd_SendtoLower(pBuffer, length, *nexthop);
        delete nexthop;
        return 0;
    }
    delete nexthop;
    fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE);
    return 1;
}

