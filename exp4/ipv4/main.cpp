/*
* THIS FILE IS FOR IP TEST
*/
// system support
//#include "sysInclude.h"
#include<string>
#include<stdio.h>
#include<malloc.h>
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace std;
//#include<pcap.h>





//1.Pcap Header
//
//�ļ�ͷ��ÿһ��pcap�ļ�ֻ��һ���ļ�ͷ���ܹ�ռ24��B���ֽڣ��������ܹ�7���ֶεĺ��塣
//
//Magic(4B)������ļ���ʼ��������ʶ���ļ����ֽ�˳��ֵ����Ϊ0xa1b2c3d4����0xd4c3b2a1�������0xa1b2c3d4��ʾ�Ǵ��ģʽ������ԭ����˳��һ���ֽ�һ���ֽڵĶ��������0xd4c3b2a1��ʾС��ģʽ��������ֽڶ�Ҫ����˳�����ڵĵ��Դ󲿷���С��ģʽ��
//
//Major(2B)����ǰ�ļ�����Ҫ�汾�ţ�һ��Ϊ0x0200
//
//Minor(2B)����ǰ�ļ��Ĵ�Ҫ�汾�ţ�һ��Ϊ0x0400
//
//ThisZone(4B)�����صı�׼�¼�������õ���GMT��ȫ�㣬һ��ȫ��
//
//SigFigs(4B)��ʱ����ľ��ȣ�һ��Ϊȫ��
//
//SnapLen(4B)�����Ĵ洢���ȣ�������ץ������ݰ�����󳤶ȣ�����������ݰ���Ҫץ�񣬽�ֵ����Ϊ65535
//
//LinkType(4B)����·���͡��������ݰ�����Ҫ�ж�����LinkType���������ֵ����Ҫ��һ���ֵΪ1������̫��
// 
// 
//2.Packet Header
//
//���ݰ�ͷ�����ж����ÿ�����ݰ�ͷ���涼�������������ݰ���������Packet Header��4���ֶκ���
//
//Timestamp(4B)��ʱ�����λ����ȷ��seconds������Unixʱ������������ݰ���ʱ��һ���Ǹ������ֵ
//
//Timestamp(4B)��ʱ�����λ���ܹ���ȷ��microseconds
//
//Caplen(4B)����ǰ�������ĳ��ȣ���ץȡ��������֡���ȣ��ɴ˿��Եõ���һ������֡��λ�á�
//
//Len(4B)���������ݳ��ȣ���·��ʵ������֡�ĳ��ȣ�һ�㲻����Caplen����������º�Caplenֵһ��
//��������������������������������
//
//��Ȩ����������Ϊ����ԭ�����£���ѭ CC 4.0 BY - SA ��ȨЭ�飬ת���븽��ԭ�ĳ������Ӻͱ�������
//
//ԭ�����ӣ�https ://blog.csdn.net/ytx2014214081/article/details/80112277







int main() {
	// �򿪶������ļ�
	ifstream file("2022112266/pro0.pcap", ios::binary);

	// ����ļ��Ƿ�ɹ���
	if (!file.is_open()) {
		cerr << "Failed to open file." << endl;
		return 1;
	}

	// ��ȡ�ļ�����
	char buffer[1024]; // ���ڴ洢��ȡ�����ݵĻ�����
	file.read(buffer, sizeof(buffer)); // ���ļ��ж�ȡ���ݵ�������
	unsigned short length = file.gcount(); // ��ȡʵ�ʶ�ȡ���ֽ���


	// �ر��ļ�
	file.close();

	return 0;
}
std::string binaryToIPv4(const char* bytes) {
	// �������ĸ��ֽڵĶ����ƽ���Ϊһ�� 32 λ����
	unsigned int ipInt = 0;
	for (int i = 0; i < 4; ++i) {
		ipInt = (ipInt << 8) | static_cast<unsigned char>(bytes[i]);
	}

	// ������ת��Ϊ IPv4 ��ַ�ĵ��ʮ���Ʊ�ʾ
	std::string ipAddress;
	for (int i = 0; i < 4; ++i) {
		if (i > 0) ipAddress += '.';
		ipAddress += std::to_string((ipInt >> (8 * (3 - i))) & 0xFF);
	}
	return ipAddress;
}

int ip_recv(char* buffer, unsigned short length)
{
	//pcap��ǰ32λ����ipv4����
	copy(buffer + 32, buffer + length, buffer);

	int version = buffer[0] >> 4;
	int headLength = buffer[0] & 0xf;//ȡ���16λ
	int TTL = (unsigned short)buffer[8];
	int checkSum = ntohs(*(unsigned short*)(pbuffer + 10));
	string dstAddr = binaryToIPv4(buffer[16])

	//�жϰ汾���Ƿ����
	if (version != 4)
	{
		cout << "�汾�Ŵ�\n";
		return 1;
	}
	//�ж�ͷ�������Ƿ����
	if (headLength < 5)
	{
		cout << "ͷ�����ȴ�\n";
		return 1;
	}
	//�ж�TTL�Ƿ����
	if (TTL <= 0)
	{
		cout << "TTL��\n";
		return 1;
	}
	//�ж�Ŀ�ĵ�ַ�Ƿ����/�������߹㲥
	if (dstAddr != "192.168.214.138" && dstAddr != 0xffff) {
		cout << "����Ŀ���ַ";
		return 1;
	}
	//�ж�У����Ƿ����
	unsigned int sum = 0;
	for (int i = 0; i < 10; i++)
	{
		sum += ((unsigned short*)pBuffer)[i];
	}
	sum = (sum >> 16) + (sum & 0xFFFF);

	if (sum != 0xffff) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
		return 1;
	}
	//�޴���
	ip_SendtoUp(pBuffer, length);
	return 0;
}









//extern void ip_DiscardPkt(char* pBuffer, int type);
//
//extern void ip_SendtoLower(char* pBuffer, int length);
//
//extern void ip_SendtoUp(char* pBuffer, int length);
//
//extern unsigned int getIpv4Address();

// implemented by students
/**
 * @brief �ж��Ƿ��ܳɹ�����
 *
 * @param pBuffer ָ����ջ�������ָ�룬ָ�� IPv4 ����ͷ��
 * @param length IPv4 ���鳤��
 * @return 0���ɹ����� IP ���鲢�����ϲ㴦��
		   1�� IP �������ʧ��
 */
//int stud_ip_recv(char* pBuffer, unsigned short length)
//{
//	int version = pBuffer[0] >> 4;
//	int headLength = pBuffer[0] & 0xf;//ȡ���16λ
//	int TTL = (unsigned short)pBuffer[8];
//	int checkSum = ntohs(*(unsigned short*)(pBuffer + 10));
//	int dstAddr = ntohl(*(unsigned int*)(pBuffer + 16));
//
//	//�жϰ汾���Ƿ����
//	if (version != 4)
//	{
//		ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
//		return 1;
//	}
//	//�ж�ͷ�������Ƿ����
//	if (headLength < 5)
//	{
//		ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
//		return 1;
//	}
//	//�ж�TTL�Ƿ����
//	if (TTL <= 0)
//	{
//		ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
//		return 1;
//	}
//	//�ж�Ŀ�ĵ�ַ�Ƿ����/�������߹㲥
//	if (dstAddr != getIpv4Address() && dstAddr != 0xffff) {
//		ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR);
//		return 1;
//	}
//	//�ж�У����Ƿ����
//	unsigned int sum = 0;
//	for (int i = 0; i < 10; i++)
//	{
//		sum += ((unsigned short*)pBuffer)[i];
//	}
//	sum = (sum >> 16) + (sum & 0xFFFF);
//
//	if (sum != 0xffff) {
//		ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
//		return 1;
//	}
//	//�޴���
//	ip_SendtoUp(pBuffer, length);
//	return 0;
//}
/**
 * @brief ���ͽӿڣ���װIPV4���ݱ�
 *
 * @param pBuffer ָ���ͻ�������ָ�룬ָ�� IPv4 �ϲ�Э������ͷ��
 * @param len IPv4 �ϲ�Э�����ݳ���
 * @param srcAddr Դ IPv4 ��ַ
 * @param dstAddr Ŀ�� IPv4 ��ַ
 * @param protocol IPv4 �ϲ�Э���
 * @param ttl ����ʱ�䣨Time To Live��
 * @return ����ֵ��
			0���ɹ����� IP ����
			1������ IP ����ʧ��
 */
//int stud_ip_Upsend(char* pBuffer, unsigned short len, unsigned int srcAddr,
//	unsigned int dstAddr, byte protocol, byte ttl)
//{
//	char* IPBuffer = (char*)malloc((20 + len) * sizeof(char));
//	memset(IPBuffer, 0, len + 20);
//	//�汾��+ͷ������
//	IPBuffer[0] = 0x45;
//	//�ܳ���
//	unsigned short totalLength = htons(len + 20);//ת���ֽ���
//	memcpy(IPBuffer + 2, &totalLength, 2);
//	//TTL
//	IPBuffer[8] = ttl;
//	//Э��
//	IPBuffer[9] = protocol;
//	//Դ��ַ��Ŀ�ĵ�ַ
//	unsigned int src = htonl(srcAddr);
//	unsigned int dis = htonl(dstAddr);
//	memcpy(IPBuffer + 12, &src, 4);  //Դ��Ŀ��IP��ַ
//	memcpy(IPBuffer + 16, &dis, 4);
//
//
//	//����checksum
//	unsigned int sum = 0;
//	unsigned short checksum = 0;
//	for (int i = 0; i < 10; i++)
//	{
//		sum += ((unsigned short*)IPBuffer)[i];
//	}
//	sum = (sum >> 16) + (sum & 0xFFFF);
//
//	checksum = ~sum;
//
//	memcpy(IPBuffer + 10, &checksum, 2);
//	memcpy(IPBuffer + 20, pBuffer, len);
//	ip_SendtoLower(IPBuffer, len + 20);
//	return 0;
//}
