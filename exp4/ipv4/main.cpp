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
//文件头，每一个pcap文件只有一个文件头，总共占24（B）字节，以下是总共7个字段的含义。
//
//Magic(4B)：标记文件开始，并用来识别文件和字节顺序。值可以为0xa1b2c3d4或者0xd4c3b2a1，如果是0xa1b2c3d4表示是大端模式，按照原来的顺序一个字节一个字节的读，如果是0xd4c3b2a1表示小端模式，下面的字节都要交换顺序。现在的电脑大部分是小端模式。
//
//Major(2B)：当前文件的主要版本号，一般为0x0200
//
//Minor(2B)：当前文件的次要版本号，一般为0x0400
//
//ThisZone(4B)：当地的标准事件，如果用的是GMT则全零，一般全零
//
//SigFigs(4B)：时间戳的精度，一般为全零
//
//SnapLen(4B)：最大的存储长度，设置所抓获的数据包的最大长度，如果所有数据包都要抓获，将值设置为65535
//
//LinkType(4B)：链路类型。解析数据包首先要判断它的LinkType，所以这个值很重要。一般的值为1，即以太网
// 
// 
//2.Packet Header
//
//数据包头可以有多个，每个数据包头后面都跟着真正的数据包。以下是Packet Header的4个字段含义
//
//Timestamp(4B)：时间戳高位，精确到seconds，这是Unix时间戳。捕获数据包的时间一般是根据这个值
//
//Timestamp(4B)：时间戳低位，能够精确到microseconds
//
//Caplen(4B)：当前数据区的长度，即抓取到的数据帧长度，由此可以得到下一个数据帧的位置。
//
//Len(4B)：离线数据长度，网路中实际数据帧的长度，一般不大于Caplen，多数情况下和Caplen值一样
//————————————————
//
//版权声明：本文为博主原创文章，遵循 CC 4.0 BY - SA 版权协议，转载请附上原文出处链接和本声明。
//
//原文链接：https ://blog.csdn.net/ytx2014214081/article/details/80112277







int main() {
	// 打开二进制文件
	ifstream file("2022112266/pro0.pcap", ios::binary);

	// 检查文件是否成功打开
	if (!file.is_open()) {
		cerr << "Failed to open file." << endl;
		return 1;
	}

	// 读取文件内容
	char buffer[1024]; // 用于存储读取的数据的缓冲区
	file.read(buffer, sizeof(buffer)); // 从文件中读取数据到缓冲区
	unsigned short length = file.gcount(); // 获取实际读取的字节数


	// 关闭文件
	file.close();

	return 0;
}
std::string binaryToIPv4(const char* bytes) {
	// 将连续四个字节的二进制解释为一个 32 位整数
	unsigned int ipInt = 0;
	for (int i = 0; i < 4; ++i) {
		ipInt = (ipInt << 8) | static_cast<unsigned char>(bytes[i]);
	}

	// 将整数转换为 IPv4 地址的点分十进制表示
	std::string ipAddress;
	for (int i = 0; i < 4; ++i) {
		if (i > 0) ipAddress += '.';
		ipAddress += std::to_string((ipInt >> (8 * (3 - i))) & 0xFF);
	}
	return ipAddress;
}

int ip_recv(char* buffer, unsigned short length)
{
	//pcap包前32位不是ipv4内容
	copy(buffer + 32, buffer + length, buffer);

	int version = buffer[0] >> 4;
	int headLength = buffer[0] & 0xf;//取最后16位
	int TTL = (unsigned short)buffer[8];
	int checkSum = ntohs(*(unsigned short*)(pbuffer + 10));
	string dstAddr = binaryToIPv4(buffer[16])

	//判断版本号是否出错
	if (version != 4)
	{
		cout << "版本号错\n";
		return 1;
	}
	//判断头部长度是否出错
	if (headLength < 5)
	{
		cout << "头部长度错\n";
		return 1;
	}
	//判断TTL是否出错
	if (TTL <= 0)
	{
		cout << "TTL错\n";
		return 1;
	}
	//判断目的地址是否出错/本机或者广播
	if (dstAddr != "192.168.214.138" && dstAddr != 0xffff) {
		cout << "错误目标地址";
		return 1;
	}
	//判断校验和是否出错
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
	//无错误
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
 * @brief 判断是否能成功接收
 *
 * @param pBuffer 指向接收缓冲区的指针，指向 IPv4 分组头部
 * @param length IPv4 分组长度
 * @return 0：成功接收 IP 分组并交给上层处理
		   1： IP 分组接收失败
 */
//int stud_ip_recv(char* pBuffer, unsigned short length)
//{
//	int version = pBuffer[0] >> 4;
//	int headLength = pBuffer[0] & 0xf;//取最后16位
//	int TTL = (unsigned short)pBuffer[8];
//	int checkSum = ntohs(*(unsigned short*)(pBuffer + 10));
//	int dstAddr = ntohl(*(unsigned int*)(pBuffer + 16));
//
//	//判断版本号是否出错
//	if (version != 4)
//	{
//		ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
//		return 1;
//	}
//	//判断头部长度是否出错
//	if (headLength < 5)
//	{
//		ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
//		return 1;
//	}
//	//判断TTL是否出错
//	if (TTL <= 0)
//	{
//		ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
//		return 1;
//	}
//	//判断目的地址是否出错/本机或者广播
//	if (dstAddr != getIpv4Address() && dstAddr != 0xffff) {
//		ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR);
//		return 1;
//	}
//	//判断校验和是否出错
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
//	//无错误
//	ip_SendtoUp(pBuffer, length);
//	return 0;
//}
/**
 * @brief 发送接口，封装IPV4数据报
 *
 * @param pBuffer 指向发送缓冲区的指针，指向 IPv4 上层协议数据头部
 * @param len IPv4 上层协议数据长度
 * @param srcAddr 源 IPv4 地址
 * @param dstAddr 目的 IPv4 地址
 * @param protocol IPv4 上层协议号
 * @param ttl 生存时间（Time To Live）
 * @return 返回值：
			0：成功发送 IP 分组
			1：发送 IP 分组失败
 */
//int stud_ip_Upsend(char* pBuffer, unsigned short len, unsigned int srcAddr,
//	unsigned int dstAddr, byte protocol, byte ttl)
//{
//	char* IPBuffer = (char*)malloc((20 + len) * sizeof(char));
//	memset(IPBuffer, 0, len + 20);
//	//版本号+头部长度
//	IPBuffer[0] = 0x45;
//	//总长度
//	unsigned short totalLength = htons(len + 20);//转换字节序
//	memcpy(IPBuffer + 2, &totalLength, 2);
//	//TTL
//	IPBuffer[8] = ttl;
//	//协议
//	IPBuffer[9] = protocol;
//	//源地址与目的地址
//	unsigned int src = htonl(srcAddr);
//	unsigned int dis = htonl(dstAddr);
//	memcpy(IPBuffer + 12, &src, 4);  //源与目的IP地址
//	memcpy(IPBuffer + 16, &dis, 4);
//
//
//	//计算checksum
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
