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
#include <iomanip> // �������������ʽ
#include <vector>
#include <sstream> // ���� std::ostringstream
#include <iomanip> // ���� std::setw �� std::setfill
using namespace std;
FILE* fp;






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



// 1.3.1Ethernet֡:
// ����pcap������ȡ��һ�����ݿ�ʱ��������ݿ������̫֡��һ��ṹ���£�


// 1.Ŀ���ַ(destination address,DA):6 �ֽ�

// 2.Դ��ַ(source address,SA):6 �ֽ�

// 3.����(type)�ֶ�:���ڱ���ϲ�Э��,2 �ֽڣ�0x08 0x00 ΪIPv4��

// 4.����(data):64 ��1500 �ֽ�

//��������������������������������
//
//��Ȩ����������Ϊ����ԭ�����£���ѭ CC 4.0 BY - SA ��ȨЭ�飬ת���븽��ԭ�ĳ������Ӻͱ�������
//
//ԭ�����ӣ�https ://blog.csdn.net/ytx2014214081/article/details/80112277


void printCharArrayAsHex(const char* arr, int size) {
	for (int i = 0; i < size; ++i) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(arr[i])) << " ";
	}
	std::cout << std::endl;
}

string binaryToIPv4(char* buffer) {
	// �������ĸ��ֽڵĶ����ƽ���Ϊһ�� 32 λ����
	char bytes[4];
	copy(buffer + 16, buffer + 20, bytes);
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
string charArrayToHexString(const char* data, int startIdx, int length) {
	std::string result;

	// ���ý���ַ���Ϊʮ������
	result.reserve(length * 2); // һ���ֽڶ�Ӧ����ʮ�������ַ�

	// ��ָ�����ȵ� char ���鲿��ת��Ϊʮ�������ַ���
	for (int i = startIdx; i < startIdx + length; ++i) {
		result += "0123456789ABCDEF"[((data[i] >> 4) & 0xF)]; // ����λ
		result += "0123456789ABCDEF"[(data[i] & 0xF)];        // ����λ
	}

	return "0x"+result;
}
// ����У���
unsigned short calculateIPv4Checksum(const char* buffer, size_t headerLength) {
	unsigned int sum = 0;
	char header[80];
	copy(buffer, buffer + headerLength, header);
	header[10] = 0;
	header[11] = 0;
	//printCharArrayAsHex(header, headerLength);
	// ��ÿ�����ֽںϲ���һ�� 16 λ�֣������
	for (size_t i = 0; i < headerLength - 1; i += 2) {
		sum += (static_cast<unsigned char>(header[i]) << 8) + static_cast<unsigned char>(header[i + 1]);
	}

	//// ����ֽ���Ϊ�����������һ���ֽڼӵ�����
	//if (headerLength % 2 != 0) {
	//    sum += static_cast<unsigned char>(header[headerLength - 1]) << 8;
	//}


	 //���� 16 λ��� 16 λ���
	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}
	sum += (static_cast<unsigned char>(buffer[10]) << 8) + static_cast<unsigned char>(buffer[11]);

	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}
	// ȡ���õ�У���
	//cout<<static_cast<unsigned short>(~sum)<<"check\n";
	return static_cast<unsigned short>(~sum);
}

string newIPv4Checksum(const char* buffer, size_t headerLength) {
	unsigned int sum = 0;
	char header[80];
	std::copy(buffer, buffer + headerLength, header);
	header[10] = 0;
	header[11] = 0;

	// ��ÿ�����ֽںϲ���һ�� 16 λ�֣������
	for (size_t i = 0; i < headerLength - 1; i += 2) {
		sum += (static_cast<unsigned char>(header[i]) << 8) + static_cast<unsigned char>(header[i + 1]);
	}

	// ���� 16 λ��� 16 λ���
	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	// ȡ���õ�У���
	unsigned short checksum = static_cast<unsigned short>(~sum);

	// ��У���ת��Ϊ�ַ�����ʽ
	std::ostringstream oss;
	oss <<"0x"<< std::hex << std::setw(4) << std::setfill('0') << checksum;
	return oss.str();
}




int ip_recv(std::ofstream& outputFile,char* buffer, unsigned short length, const std::vector<std::string>& forward)
{
	//pcap��ǰ54λ����ipv4����
	copy(buffer + 54, buffer + length, buffer);
	//printCharArrayAsHex(buffer, length);

	int version = buffer[0] >> 4;
	int headLength = buffer[0] & 0xf;//ȡ���16λ
	int TTL = (unsigned short)buffer[8];
	string dstAddr = binaryToIPv4(buffer);
	//cout << version<<"v\n";
	cout << headLength << "headlength\n";
	//cout << TTL<<"TTL\n";
	//cout << dstAddr<<"dstaddr\n";
	//�жϰ汾���Ƿ����
	if (version != 4)
	{
		cout << "�汾�Ŵ� ����\n";
		outputFile << u8"�汾�Ŵ� ����\n";

		return 1;
	}
	//�ж�ͷ�������Ƿ����
	if (headLength != 5)
	{
		cout << "ͷ�����ȴ� ����\n";
		outputFile << u8"ͷ�����ȴ� ����\n";

		return 1;
	}
	//�ж�TTL�Ƿ����
	if (TTL <= 0)
	{
		cout << "TTL�� ����\n";
		outputFile << u8"TTL�� ����\n";

		return 1;
	}
	if ((calculateIPv4Checksum(buffer, headLength * 4) & 0xffff) != 0) {
		cout << "У��ʹ� ����\n";
		outputFile << u8"У��ʹ� ����\n";

		return 1;
	}

	// ʹ�� std::find ������
	auto it = std::find(forward.begin(), forward.end(), dstAddr);

	// ������Ƿ��ҵ�
	if (it != forward.end()) {
		cout <<charArrayToHexString(buffer, 16, 4).data()<< "��ȷ ת��\n";
		//outputFile << u8"��ȷ ת�� " << TTL - 1 << ' ' << u8"\n";
		outputFile << u8"��ȷ ת�� "<<TTL - 1<< ' '<< newIPv4Checksum(buffer,20)<< u8"\n";
		return 0;
	}

	//�ж�Ŀ�ĵ�ַ�Ƿ����/�������߹㲥
	if (dstAddr != "192.168.110.138") {
		//if (dstAddr != "192.168.214.138" && dstAddr != 0xffff) {
		cout << "����Ŀ���ַ ����\n";
		outputFile << u8"����Ŀ���ַ ����\n";

		return 1;
	}

	//ip_SendtoUp(pBuffer, length);
	cout << "��ȷ\n";
	outputFile << u8"��ȷ\n";

	return 0;
}
int checkfile(std::ofstream& outputFile,string filename, const std::vector<std::string>& forward) {
	ifstream file(filename, ios::binary);

	// ����ļ��Ƿ�ɹ���
	if (!file.is_open()) {
		cerr << "Failed to open file." << endl;
		return 1;
	}
	// ��ȡ�ļ�����
	char buffer[1024]; // ���ڴ洢��ȡ�����ݵĻ�����
	file.read(buffer, sizeof(buffer)); // ���ļ��ж�ȡ���ݵ�������
	unsigned short length = file.gcount(); // ��ȡʵ�ʶ�ȡ���ֽ���
	//printCharArrayAsHex(buffer, length);
	ip_recv(outputFile,buffer, length,forward);
	// �ر��ļ�
	file.close();
	return 0;
}



int main() {
	// �򿪶������ļ�
	errno_t err;
	std::ofstream outputFile("result.txt", std::ios::out | std::ios::binary);
	std::string baseFilename = "2022112266/pro"; // �����ļ���
	const int numFiles = 100; // �ļ�����

	std::vector<std::string> forward = { "192.168.214.2", "192.168.214.1", " 192.168.214.3", "43.168.142.77", "192.168.213.2", "192.168.142.2", "192.168.142.77", " 43.168.142.78", "43.168.142.65", "43.168.142.66", " 97.43.142.226", "97.43.142.225", "97.43.142.229" };

	for (int i = 0; i < numFiles; ++i) {
		cout << i << endl;
		std::string filename = baseFilename + std::to_string(i) + ".pcap";
		checkfile(outputFile,filename,forward);
	}





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
