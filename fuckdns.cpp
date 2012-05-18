#include <stdio.h>
#include <time.h>
#include <Winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

//DNS服务器端口
#define DNS_PORT 53

//缓冲区大小
#define BUFF_SIZE 2048

#define DNS_SERV_NUM 4
//DNS服务器ip
const char dns_ip[DNS_SERV_NUM][16] = {
	"208.67.222.222",
	"208.67.220.220",
	"8.8.8.8",
	"8.8.4.4",
	//"178.79.131.110",
	//"106.187.39.23"
};
//DNS服务器选择
static unsigned int sel = 0;

//本地UDP转发服务socket
static SOCKET	serv;

//远程DNS地址信息
//struct sockaddr_in dns_addr;

//本地DNS包发送者地址信息
static struct sockaddr_in from_addr;
//本地UDP转发服务程序地址信息
static struct sockaddr_in serv_addr;

int	fromlen = sizeof(from_addr);

//线程参数结构体
typedef struct DATA{
	char data[BUFF_SIZE];
	struct sockaddr_in from_addr;
	int len;
}*pData;

//线程函数，处理DNS数据包
DWORD WINAPI ThreadFunc( LPVOID lp ); 

int main(int argc, char **argv)
{
	WSADATA	ws;

	int	cnt;
	char *buff = NULL;
	char *buf = NULL;

	printf("=========================================================\n");
	printf("Starting ...\n");
	if (0 != WSAStartup(MAKEWORD(2, 0), &ws))
	{
		printf("Init windows socket error\n");
		return -1;
	}
	
	serv = socket(AF_INET, SOCK_DGRAM, 0);
	if (INVALID_SOCKET == serv)
	{
		printf("socket() error\n");
		return -1;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(DNS_PORT);
	serv_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	
	if (-1 == bind(serv, (struct sockaddr *)&serv_addr, sizeof(serv_addr)))
	{
		printf("bind error\n");
		return -1;
	}

	printf("Server OK !\n");

	printf("DNS Servers IPs:\n");
	for (int i = 0; i < DNS_SERV_NUM; i++)
	{
		printf("\t%s\n", dns_ip[i]);
	}
	printf("=========================================================\n");

	while (1)
	{
		pData msg = NULL;
		msg = (pData)(new char[sizeof(struct DATA)]);
		if (NULL == msg)
		{
			printf("new char[] error !\n");
			continue;
		}

		buff = msg->data;
		buf = buff + 2;

		cnt = recvfrom(serv, buf, BUFF_SIZE - 2, 0, (struct sockaddr *)&from_addr, &fromlen);
		if (cnt == SOCKET_ERROR)
		{
			//printf("udp recv error [code = %d]\n", WSAGetLastError());
			free(msg);
			continue;
		}
		
		*(UINT16 *)buff = htons(cnt);
		memcpy(&(msg->from_addr), &(from_addr), sizeof(from_addr));
		msg->len = cnt;

		if (NULL == CreateThread(NULL,0,ThreadFunc,(LPVOID)msg, 0,NULL))
		{
			if (NULL == CreateThread(NULL,0,ThreadFunc,(LPVOID)msg, 0,NULL))
			{
				if (NULL == CreateThread(NULL,0,ThreadFunc,(LPVOID)msg, 0,NULL))
				{
					free(msg);
					printf("CreateThread error !\n");
				}
			}
		}
	}

	closesocket(serv);
	WSACleanup();

	return 0;
}

DWORD WINAPI ThreadFunc( LPVOID lp )
{
	pData msg = (pData)lp;
	char *buff = msg->data;
	char *buf = buff + 2;

	SOCKET client;
	struct sockaddr_in dns_addr;
	int cnt = 0, i = 0;

	dns_addr.sin_port = htons(53);
	dns_addr.sin_family = AF_INET;

	dns_addr.sin_addr.S_un.S_addr = inet_addr(dns_ip[sel % DNS_SERV_NUM]);
	//printf("%s\n", dns_ip[sel % DNS_SERV_NUM]);
	sel++;

	client = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == client)
	{
		printf("socket() error [dns = %s code = %d]\n", dns_ip[sel % DNS_SERV_NUM], WSAGetLastError());
		free(msg);
		return -1;
	}
	if (SOCKET_ERROR == connect(client, (struct sockaddr *)&dns_addr, sizeof(dns_addr)))
	{
		Sleep(10);
		if (SOCKET_ERROR == connect(client, (struct sockaddr *)&dns_addr, sizeof(dns_addr)))
		{
			Sleep(10);
			if (SOCKET_ERROR == connect(client, (struct sockaddr *)&dns_addr, sizeof(dns_addr)))
			{
				printf("connect error [dns = %s code = %d]\n", dns_ip[sel % DNS_SERV_NUM], WSAGetLastError());
				free(msg);
				closesocket(client);
				return -1;
			}
		}
	}

	cnt = send(client, buff, msg->len + 2, 0);
	if (cnt == SOCKET_ERROR)
	{
		Sleep(10);
		cnt = send(client, buff, msg->len + 2, 0);
		if (cnt == SOCKET_ERROR)
		{
			Sleep(10);
			cnt = send(client, buff, msg->len + 2, 0);
			if (cnt == SOCKET_ERROR)
			{
				printf("tcp send error [dns = %s code = %d]\n", dns_ip[sel % DNS_SERV_NUM], WSAGetLastError());
				free(msg);
				closesocket(client);
				return -1;
			}
		}
	}
		
	cnt = recv(client, buff, BUFF_SIZE, 0);
	if (cnt == SOCKET_ERROR)
	{
		Sleep(10);
		cnt = recv(client, buff, BUFF_SIZE, 0);
		if (cnt == SOCKET_ERROR)
		{
			Sleep(10);
			cnt = recv(client, buff, BUFF_SIZE, 0);
			if (cnt == SOCKET_ERROR)
			{
				printf("tcp recv error [dns = %s code = %d]\n", dns_ip[sel % DNS_SERV_NUM], WSAGetLastError());
				free(msg);
				closesocket(client);
				return -1;
			}
		}
	}
	
	if (sendto(serv, buf, cnt - 2, 0, (struct sockaddr *)&(msg->from_addr), sizeof(struct sockaddr)) == SOCKET_ERROR)
	{
		Sleep(10);
		if (sendto(serv, buf, cnt - 2, 0, (struct sockaddr *)&(msg->from_addr), sizeof(struct sockaddr)) == SOCKET_ERROR)
		{
			Sleep(10);
			if (sendto(serv, buf, cnt - 2, 0, (struct sockaddr *)&(msg->from_addr), sizeof(struct sockaddr)) == SOCKET_ERROR)
			{
				printf("udp send error [dns = %s code = %d]\n", dns_ip[sel % DNS_SERV_NUM], WSAGetLastError());
				free(msg);
				closesocket(client);
				return -1;
			}
		}
	}
	
	free(msg);
	closesocket(client);
	return 0;
}