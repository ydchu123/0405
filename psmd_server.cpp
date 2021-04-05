//---------------------------------------------------------------------------

#pragma hdrstop

#include "psmd_server.h"

#include <WS2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <ImageHlp.h>
#include "psmd_main_form_24c.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

// #ifdef _DEBUG
LONGLONG last_set_time = 0;
#define SOCKET_ERROR    -1
//#define CONNECT_ERROR   -2
//int FileNumbersflag=0;
//int FileNumbersflag2=0;
// #endif
//add  for extern //////////////
 float SDVol;
 unsigned char SamRateRecBuf[65];
 unsigned char StopSampRcvBuf[2];
 unsigned char QueryFileRcvBuf[24];
///////////////////////////


unsigned short calc_crc(void* beg, int len) {
	unsigned short crc = 0xFFFF;
	unsigned char* cursor = (unsigned char*)beg;

	for(int i = 0; i < len; i++){
		unsigned short j = (unsigned short)*cursor;
		crc ^= j;
		for(int k = 0; k < 8; k++){
			if(crc & 1){
				crc >>= 1;
                crc ^= 0xA001;
			}else{
                crc >>= 1;
            }
        }
	}
    return crc;
}
unsigned __int32  HextoInt32(char *str)
{
	__int32 length=strlen(str);
	if(length==0) return 0;
	__int32 i,Num=0,stmp=0;
	for(i=0;i<length;i++)
	{
		if(str[i]<='9'&&str[i]>='0')
			stmp=str[i]-'0';
		else if(str[i]>='a' && str[i]<='f')
			stmp=str[i]-87;
		else if(str[i]>='A' && str[i]<='F')
			stmp=str[i]-55;
		else  //其它字符返回-1
			return -1;

		if(stmp>=0) Num=Num*16+stmp;
	}
	return Num;
}
unsigned __int64  HextoInt64(char *str)
{
	__int64 length=strlen(str);
	if(length==0) return 0;
	__int64 i,Num=0,stmp=0;
	for(i=0;i<length;i++)
	{
		if(str[i]<='9'&&str[i]>='0')
			stmp=str[i]-'0';
		else if(str[i]>='a' && str[i]<='f')
			stmp=str[i]-87;
		else if(str[i]>='A' && str[i]<='F')
			stmp=str[i]-55;
		else  //其它字符返回-1
			return -1;

		if(stmp>=0) Num=Num*16+stmp;
	}
	return Num;
}
unsigned __int64 ntohll(unsigned __int64 num){
	unsigned __int64 res;
	*((unsigned int*)&(res)) = ntohl(*((unsigned int*)&(num)+1));
	*((unsigned int*)&(res)+1) = ntohl(*((unsigned int*)&(num)));
	return res;
}

LONGLONG read_time_at_idx(HANDLE file, int sizeof_frame, int idx){
	LONGLONG res;
	if(idx < 0) {
        return -1;
    }
	SetFilePointer(file, sizeof_frame * idx, NULL, SEEK_SET);
    DWORD bytes_read;
	ReadFile(file, &res, sizeof(res), &bytes_read, 0);
	if(bytes_read != 8){
        res = -1;
	}
    return res;
}

int get_first_frame_be_time(HANDLE file, int sizeof_frame, LONGLONG time){
	int beg_idx = 0;
	int end_idx = SetFilePointer(file, 0, NULL, SEEK_END) / sizeof_frame - 1;

    while(true) {
		LONGLONG beg_time = read_time_at_idx(file, sizeof_frame, beg_idx);
		LONGLONG end_time = read_time_at_idx(file, sizeof_frame, end_idx);

		if(time < beg_time || end_time == beg_time){
            return beg_idx;
		}

		int prefered_idx = beg_idx + (time - beg_time) * (end_idx - beg_idx) / (end_time - beg_time);
		if((beg_idx == end_idx || beg_idx == end_idx - 1) && (prefered_idx == beg_idx || prefered_idx == end_idx)){
            return prefered_idx;
		}
		LONGLONG real_time = read_time_at_idx(file, sizeof_frame, prefered_idx);
        LONGLONG real_time_pre = read_time_at_idx(file, sizeof_frame, prefered_idx - 1);
		if(real_time >= time && real_time_pre < time){
            return prefered_idx;
		}else{
			if(real_time > time){
                end_idx = prefered_idx - 1;
			}else {
                beg_idx = prefered_idx + 1;
            }
        }
	}
}

int get_last_frame_le_time(HANDLE file, int sizeof_frame, LONGLONG time){
    int beg_idx = 0;
	int end_idx = SetFilePointer(file, 0, NULL, SEEK_END) / sizeof_frame - 1;

    while(true) {
		LONGLONG beg_time = read_time_at_idx(file, sizeof_frame, beg_idx);
		LONGLONG end_time = read_time_at_idx(file, sizeof_frame, end_idx);
		if(time > end_time || end_time == beg_time) {
            return end_idx;
		}
        int prefered_idx = beg_idx + (time - beg_time) * (end_idx - beg_idx) / (end_time - beg_time);
		if((beg_idx == end_idx || beg_idx == end_idx - 1) && (prefered_idx == beg_idx || prefered_idx == end_idx)){
            return prefered_idx;
		}
		LONGLONG real_time = read_time_at_idx(file, sizeof_frame, prefered_idx);
        LONGLONG real_time_lat = read_time_at_idx(file, sizeof_frame, prefered_idx + 1);
		if(real_time <= time && (real_time_lat > time || real_time_lat == -1)){
            return prefered_idx;
		}else{
			if(real_time > time){
                end_idx = prefered_idx - 1;
			}else{
                beg_idx = prefered_idx + 1;
            }
        }
	}
}

int get_frame_eq_time(HANDLE file, int sizeof_frame, LONGLONG time){
    int beg_idx = 0;
	int end_idx = SetFilePointer(file, 0, NULL, SEEK_END) / sizeof_frame - 1;

    while(true) {
		LONGLONG beg_time = read_time_at_idx(file, sizeof_frame, beg_idx);
		LONGLONG end_time = read_time_at_idx(file, sizeof_frame, end_idx);

		int prefered_idx = beg_idx + (time - beg_time) * (end_idx - beg_idx) / (end_time - beg_time);

		LONGLONG real_time = read_time_at_idx(file, sizeof_frame, prefered_idx);
        LONGLONG real_time_lat = read_time_at_idx(file, sizeof_frame, prefered_idx + 1);
		if(real_time <= time && real_time_lat > time){
            return prefered_idx;
		}else{
			if(real_time > time){
                end_idx = prefered_idx - 1;
			}else{
                beg_idx = prefered_idx;
            }
        }
	}
}

EPSMDServerBase::EPSMDServerBase(char* _client_ip, int _client_port, int _local_port){
	clr_hist();
    init_mem();
	init_database();

	strcpy(client_ip, _client_ip);
	client_port = _client_port;
    local_port = _local_port;
}

int EPSMDServerBase::get_version() {
	return send_scmd(PSMD_UPP_V1_GVESN, NULL, 0);
}

int EPSMDServerBase::set_time(unsigned __int64 time){
	return send_scmd(PSMD_UPP_V1_STIME, &time, sizeof(time));
}

void EPSMDServerBase::get_hist_list(){
	sqlite3_exec(m_dtbase, "select * from records", history_item, this, &m_errmsg);
}

void EPSMDServerBase::set_data_handler(DATA_CALLBACK _user_clbk, void* _user_data){
	data_user_clbk = _user_clbk;
	data_user_data = _user_data;
}

void EPSMDServerBase::set_hist_handler(HIST_CALLBACK _hist_clbk, void* _hist_data){
	hist_user_clbk = _hist_clbk;
	hist_user_data = _hist_data;
}

void WINAPI not_worker(ULONG_PTR param)
{
}


EPSMDServerBase::~EPSMDServerBase(){
	working = false;
	//while(worker_identi){
		QueueUserAPC(not_worker, worker_handle, (ULONG_PTR)this);
		WaitForSingleObject(worker_handle, 200);
   //}
	shutdown(server_socket, SD_BOTH);
	//closesocket(server_socket);
	server_socket = 0;

	clr_hist();
	sqlite3_close(m_dtbase);
}

int EPSMDServerBase::history_item(void* data, int argc, char** argv, char** col_name) {
	EPSMDServerBase* server = (EPSMDServerBase*)data;
	server->hist_user_clbk(-1, argv[1], server->hist_user_data);
	server->hist_user_clbk(0, argv[0], server->hist_user_data);
	return 0;
}

int EPSMDServerBase::history_config(void* data, int argc, char** argv, char** col_name) {
    EPSMDServerBase* server = (EPSMDServerBase*)data;
	memcpy(&server->config_posi, argv[0], server->config_size);
    return 0;
}

int EPSMDServerBase::start() {

	if(SOCKET_ERROR==init_net(client_ip, client_port, local_port))
	{
	  worker_identi=0;
	  return SOCKET_ERROR;
	}
	//add end
	init_net_data_handler();
	for(int i = 0; i < 255; i++){
		evt[i] = new TEvent(NULL, true, false, "", false);
	}
	working = true;
	worker_handle = CreateThread(NULL, 0, net_data_worker, this, 0, &worker_identi);
	return 0;
}

void EPSMDServerBase::set_hist_time(char* time, void* config, LONGLONG* min, LONGLONG* max){
	tm _tm;

	sscanf(time, "%4d-%2d-%2d %2d:%2d:%2d",
			&_tm.tm_year, &_tm.tm_mon, &_tm.tm_mday,
			&_tm.tm_hour, &_tm.tm_min, &_tm.tm_sec);

	clr_hist();

    if(config) memcpy(config_posi, config, config_size);

	sprintf(beg_time, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d",
			_tm.tm_year, _tm.tm_mon, _tm.tm_mday,
			_tm.tm_hour, _tm.tm_min, _tm.tm_sec);

	sprintf(time_prefix, ".\\rec\\%.4d%.2d%.2d%.2d%.2d%.2d",
			_tm.tm_year, _tm.tm_mon, _tm.tm_mday,
			_tm.tm_hour, _tm.tm_min, _tm.tm_sec);

	MakeSureDirectoryPathExists(time_prefix);

	AnsiString str = time_prefix;

	*min = -1;
	*max = -1;

    open_hist_file(min, max);
}

int EPSMDServerBase::send_scmd(int type, void* buf, int len) {
	EPPSMDUpperSFrame frame = (EPPSMDUpperSFrame)&s_req[type];
    memset(frame, 0, sizeof(*frame));
	frame->head = 0x5555555555555555ULL;
	frame->type = type;
	if(len && buf) {
        memcpy(frame->parm, buf, len);
		}
	//sdd for SDcard
	//frame->crc = calc_crc(&frame->type, EOFFSET_OF(EPSMDUpperSFrame, crc) - EOFFSET_OF(EPSMDUpperSFrame, type));

	frame->crc = 0xFFFF;
	evt[type]->ResetEvent();
	int send_ret = send(server_socket, (char*)frame, sizeof(*frame), 0);
	if(send_ret<0)
	{
       return 2;
    }
  // int signal =  evt[type]->WaitFor(3000);
  if(frame->type==0x04) //格式化SD卡，响应时间过长
  {
		if(wrSignaled != evt[type]->WaitFor(8000)){
				return 1;}

  }
  if(frame->type==0x05) //查询文件列表，响应时间过长
  {
		if(wrSignaled != evt[type]->WaitFor(10000)){
				return 1;}

  }
  else
  {
		if(wrSignaled != evt[type]->WaitFor(4000)){
				return 1;}
  }

//	if(memcmp(frame, &res[PSMD_UPP_STIME], sizeof(*frame))){
//		return 2;
//	}

//	return *(int*)&s_res[type].parm;
		return 0;
}

void EPSMDServerBase::clr_hist(){
	for(int i = 0; i < 2; i++){
		if(file_data[i] && file_data[i] != INVALID_HANDLE_VALUE){
			CloseHandle(file_data[i]);
			file_data[i] = NULL;
		}
		if(file_anal[i] && file_anal[i] != INVALID_HANDLE_VALUE){
			CloseHandle(file_anal[i]);
			file_anal[i] = NULL;
		}
#ifdef _DEBG
		if(file_debg[i] && file_debg[i] != INVALID_HANDLE_VALUE){
			CloseHandle(file_debg[i]);
			file_debg[i] = NULL;
		}
#endif
	}
    file_ready = false;
}

void EPSMDServerBase::init_mem() {
	memset(file_data, 0, sizeof(file_data));
	memset(file_anal, 0, sizeof(file_anal));
#ifdef _DEBUG
	memset(file_debg, 0, sizeof(file_debg));
#endif
}

void EPSMDServerBase::init_database(){
	sqlite3_open("records.db", &m_dtbase);
	sqlite3_exec(m_dtbase, "create table if not exists records(config blob, tdate_beg DATETIME default (datetime(CURRENT_TIMESTAMP, 'localtime')))", NULL, NULL, &m_errmsg);
}

void EPSMDServerBase::add_hist(void* config, int size, char* file_time, int file_count) {
	clr_hist();

	std::string sql = "insert into records (config, tdate_beg) values(?, ?)";

	time_t	_time_t;
	time(&_time_t);

	tm _tm;
	localtime_s(&_time_t, &_tm);

	sprintf(beg_time, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d",
			_tm.tm_year + 1900, _tm.tm_mon + 1, _tm.tm_mday,
			_tm.tm_hour, _tm.tm_min, _tm.tm_sec);

    strcpy(file_time, beg_time);

	sprintf(time_prefix, ".\\rec\\%.4d%.2d%.2d%.2d%.2d%.2d",
			_tm.tm_year + 1900, _tm.tm_mon + 1, _tm.tm_mday,
			_tm.tm_hour, _tm.tm_min, _tm.tm_sec);

	MakeSureDirectoryPathExists(time_prefix);

	sqlite3_stmt* stmt;

	sqlite3_prepare(m_dtbase, sql.c_str(), sql.length(), &stmt, NULL);
	sqlite3_bind_blob(stmt, 1, config, size, NULL);
	sqlite3_bind_text(stmt, 2, beg_time, strlen(beg_time), NULL);
	sqlite3_step(stmt);

	sqlite3_finalize(stmt);

	AnsiString str = time_prefix;

	for(int i = 0; i < file_count; i++){
		char file_path[255];
		sprintf(file_path, "%s_%d.data", str.c_str(), i);
		file_data[i] = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
		sprintf(file_path, "%s_%d.anal", str.c_str(), i);
		file_anal[i] = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
#ifdef _DEBUG
		sprintf(file_path, "%s_%d.debg", str.c_str(), i);
		file_debg[i] = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
#endif
		hist_beg[i] = -1;
		hist_end[i] = -1;
	}

	file_ready = true;
}

EPSMDUDPServer::EPSMDUDPServer(char* client_ip, int client_port, int local_port) :
	EPSMDServerBase(client_ip, client_port, local_port) {
    net_data_worker = server_worker;
}

int EPSMDUDPServer::init_net(char* client_ip, int client_port, int local_port) {
	server_socket = socket(AF_INET,SOCK_DGRAM,0);
	SOCKADDR_IN addr_client;
	inet_pton(AF_INET, client_ip, &addr_client.sin_addr);
	addr_client.sin_family=AF_INET;
	addr_client.sin_port=ntohs(local_port);


	bool setopt = true;
	/*int retval = */setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, (char*)&setopt, sizeof(bool));

	bind(server_socket,(SOCKADDR*)&addr_client,sizeof(addr_client));

	addr_client.sin_port = ntohs(client_port);
	if(SOCKET_ERROR==connect(server_socket, (SOCKADDR*)&addr_client,sizeof(addr_client)))
	{
	  return -1;
	}
    int size = 1024 * 1024 * 256;
	setsockopt(server_socket, SOL_SOCKET, SO_RCVBUF, (char*)&size, 4);
    return 0;
}


unsigned long __stdcall EPSMDUDPServer::server_worker(void* param){
	EPSMDUDPServer* server = (EPSMDUDPServer*)param;
	EPSMDIOContext context;
	context.server = server;
	context.wsabuf.len = sizeof(context.buf);
	context.wsabuf.buf = context.buf;
	DWORD dwFlags = 0;
	DWORD dwBytesTransfered = 0;
	try
	{
		while(server->working) {
			if (SOCKET_ERROR == WSARecv(server->server_socket, &context.wsabuf, 1, &dwBytesTransfered, &dwFlags, &context.overlapped, server->net_data_handler))
			{
				int errorCode = GetLastError();
				if (WSA_IO_PENDING != errorCode) return -1;
				else SleepEx(INFINITE, true);
			}
			else
			{
				SleepEx(INFINITE, true);
			}
		}
	}catch(...){
		OutputDebugString(TEXT("Hello\n"));
	}
	server->worker_identi = 0;
    return 0;
}

EPSMDTCPServer::EPSMDTCPServer(char* client_ip, int client_port, int local_port) :
	EPSMDServerBase(client_ip, client_port, local_port) {
    net_data_worker = server_worker;
}
 //add and fix 0604
int EPSMDTCPServer::init_net(char* client_ip, int client_port, int local_port)
{
	WORD wVersionRequested = MAKEWORD(1,1);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);

	if (LOBYTE(wsaData.wVersion)!=1 ||  //取得16进制数最低(最右边)那个字节的内容
		HIBYTE(wsaData.wVersion)!=1)    //取最高字节内容
	{
		WSACleanup();
		return -1;
	}
	server_socket = socket(AF_INET,SOCK_STREAM,0);
	SOCKADDR_IN addr_client;
	inet_pton(AF_INET, client_ip, &addr_client.sin_addr);     //IP地址转换
	addr_client.sin_family=AF_INET;
//	addr_client.sin_port=ntohs(local_port);
//	bool setopt = true;
//	/*int retval = */setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, (char*)&setopt, sizeof(bool));

//	bind(server_socket,(SOCKADDR*)&addr_client,sizeof(addr_client));
	addr_client.sin_port = htons(client_port);
	//socket 设置为非阻塞   add new start
	unsigned long on = 1;
//
	if(ioctlsocket(server_socket, FIONBIO, &on) < 0)
	{
		OutputDebugString(TEXT("ioctlsocket failed"));
		int nLastError = WSAGetLastError();
		//printf("ioctlsocket failed\n");
		closesocket(server_socket);
		WSACleanup();
		return -1;
	}

	int connect_ret=connect(server_socket, (SOCKADDR*)&addr_client,sizeof(addr_client));
    //因为是非阻塞的，这个时候错误码应该是WSAEWOULDBLOCK，用select判断是否和服务器连接成功
	if(connect_ret<0&&(WSAGetLastError() == WSAEWOULDBLOCK))
	{
		fd_set writefds,expectfds;
		struct timeval tv;

		tv.tv_sec = 5;//设置select()超时时间为3s
		tv.tv_usec = 0;
		FD_ZERO(&writefds);
		FD_ZERO(&expectfds);
		FD_SET(server_socket,&writefds);
		FD_SET(server_socket,&expectfds);
		int result = select(server_socket + 1, NULL, &writefds, &expectfds, &tv);
		if (result > 0)
		{
			if(FD_ISSET(server_socket,&writefds))
			{
			   OutputDebugString(TEXT("connect success"));
            }
			if(FD_ISSET(server_socket,&expectfds))
			{
				OutputDebugString(TEXT("connect failed!"));
				int error, error_len;
				error_len = sizeof(error);
				getsockopt(server_socket, SOL_SOCKET, SO_ERROR, (char *)&error, &error_len);//获得错误号
				WSACleanup();
				closesocket(server_socket);
				return -1;
			}
		}
		else
		{
		//在一个多线程的环境下，WSACleanup（）中止了Windows Sockets在所有线程上的操作.
			closesocket(server_socket);
		    WSACleanup();  //
            return -1;
		}
         //add end


	}


	int size = 1024 * 1024 * 256;
	setsockopt(server_socket, SOL_SOCKET, SO_RCVBUF, (char*)&size, 4);
    return 0;
}

unsigned long __stdcall EPSMDTCPServer::server_worker(void* param){
	EPSMDTCPServer* server = (EPSMDTCPServer*)param;
	EPSMDIOContext context;
	context.server = server;
	context.wsabuf.len = sizeof(context.buf);
	context.wsabuf.buf = context.buf;
	DWORD dwFlags = 0;
	DWORD dwBytesTransfered = 0;
	try
	{
		while(server->working) {
			if (SOCKET_ERROR == WSARecv(server->server_socket, &context.wsabuf, 1, &dwBytesTransfered, &dwFlags, &context.overlapped, server->net_data_handler))
			{
				int errorCode = GetLastError();
				if (WSA_IO_PENDING != errorCode) return -1;
				else SleepEx(INFINITE, true);
			}
			else
			{
				SleepEx(INFINITE, true);
			}
		}
	}catch(...){
		OutputDebugString(TEXT("Hello\n"));
	}
	server->worker_identi = 0;
    return 0;
}


EPSMDServerV1::EPSMDServerV1(char* client_ip, int client_port, int local_port) :
	EPSMDUDPServer(client_ip, client_port, local_port) {
   	config_posi = &m_config;
    config_size = sizeof(m_config);
}

void EPSMDServerV1::set_config(void* config) {
    memcpy(&m_config, config, sizeof(m_config));
}

int EPSMDServerV1::stt_samp(char* file_time){
	add_hist(&m_config, sizeof(m_config), file_time, this->m_config.sparam.mode == 1 ? 2 : 1);
   	last_pkg_time[0] = last_set_time;
	last_pkg_time[1] = last_set_time;
	if(apply_sparam() || apply_tparam()) {
        return 3;
    }
    return send_scmd(PSMD_UPP_V1_STTSP, NULL, 0);

}

int EPSMDServerV1::stp_samp(){
		return send_scmd(PSMD_UPP_V1_STPSP, NULL, 0);
}

void EPSMDServerV1::init_net_data_handler() {
    net_data_handler = data_handler;
}

int EPSMDServerV1::apply_tparam() {
    return send_scmd(PSMD_UPP_V1_SVALV, &m_config.tparam, sizeof(m_config.tparam));
}

int EPSMDServerV1::apply_sparam() {
    return send_scmd(PSMD_UPP_V1_SSMPL, &m_config.sparam, sizeof(m_config.sparam));
}

void EPSMDServerV1::open_hist_file(LONGLONG* min, LONGLONG* max) {
	char file_path[255];
	for(int i = 0; i < PSMD_MAX_CHNCT; i++){

		sprintf(file_path, "%s_%d.data", time_prefix, i);
		file_data[i] = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
		sprintf(file_path, "%s_%d.anal", time_prefix, i);
		file_anal[i] = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);

		hist_beg[i] = -1;
		hist_end[i] = -1;

		if(file_data[i] != INVALID_HANDLE_VALUE && file_data[i]){
			SetFilePointer(file_data[i], 0, NULL, SEEK_SET);
			DWORD bytes_read = 0;
			ReadFile(file_data[i], &hist_beg[i], sizeof(hist_beg[i]), &bytes_read, NULL);
			DWORD end = SetFilePointer(file_data[i], 0, NULL, SEEK_END);

			int count = end / sizeof(EPSMDFileDataFrameV1);
			#ifdef _DEBUG
			LONGLONG t_referr = read_time_at_idx(file_data[i], sizeof(EPSMDFileDataFrameV1), 0);
			int j = 1;
			for(j = 1; j < count; j++){
				LONGLONG t_cursor = read_time_at_idx(file_data[i], sizeof(EPSMDFileDataFrameV1), j);
				if(j == 3320){
                    continue;
				}
				if(t_cursor < t_referr){
					OutputDebugString(TEXT("Hello"));
				}
                t_referr = t_cursor;
			}
            #endif
			SetFilePointer(file_data[i], (count - 1) * sizeof(EPSMDFileDataFrameV1), NULL, SEEK_SET);
			ReadFile(file_data[i], &hist_end[i], sizeof(hist_end[i]), &bytes_read, NULL);
		}
		if(hist_beg[i] != -1){
			if(*min == -1 || *min > hist_beg[i]){
				*min = hist_beg[i];
			}
		}
		if(hist_end[i] != -1 && hist_end[i] > *max){
            *max = hist_end[i];
        }
	}
}

void __stdcall EPSMDServerV1::data_handler(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags){
	try{
		EPPSMDIOContext 	ctx = (EPPSMDIOContext)lpOverlapped;
		EPPSMDUpperSFrame 	frm = (EPPSMDUpperSFrame)ctx->buf;
        EPSMDServerV1* server = (EPSMDServerV1*)ctx->server;

		DWORD bytes_written = 0;

		if(frm->type & 0xF0){
			EPPSMDLowerFrameV1 frm = (EPPSMDLowerFrameV1)ctx->buf;

			int idx = frm->type & 0x0F;

			if(server->last_pkg_time[idx] == 0){ server->last_pkg_time[idx] = frm->time; }
			else if(server->last_pkg_time[idx] >= frm->time || frm->time - server->last_pkg_time[idx] > 3600000) {
			#ifdef _DEBUG
				OutputDebugString(TEXT("BACD"));
				WriteFile(server->file_debg[idx], &frm->time, 12, &bytes_written, NULL);
			#endif
				return;
			}

			server->last_pkg_time[idx] = frm->time;

			if(frm->numb > 1256){
				frm->numb = 1256;
			}

			if(	frm->head == 0xAAAAAAAAAAAAAAAAULL && (true ||
				frm->crc == calc_crc(&frm->type, EOFFSET_OF(EPSMDLowerFrameV1, crc) - EOFFSET_OF(EPSMDLowerFrameV1, type))
				)
			){
				EPSMDFileAnalFrameV1 anal_frame;

				int peak = -1;
				int pawk = 256;

				for(int i = 0; i < frm->numb + 256 && i < 1256; i++){
					if(peak < (int)frm->data[i])
						peak = frm->data[i];
					if(pawk > (int)frm->data[i])
						pawk = frm->data[i];
				}

//				while(!ctx->server->fiready){
//					Sleep(20);
//				}

				if(server->hist_beg[idx] == -1){
					server->hist_beg[idx] = frm->time;
				}
				server->hist_end[idx] = frm->time;

				anal_frame.peak = peak;
				anal_frame.pawk = pawk;
				anal_frame.time = frm->time;

				WriteFile(server->file_anal[idx], &anal_frame, sizeof(anal_frame), &bytes_written, NULL);
				WriteFile(server->file_data[idx], &frm->time, sizeof(EPSMDFileDataFrameV1), &bytes_written, NULL);

				if(server->data_user_clbk) server->data_user_clbk(frm, sizeof(*frm), &anal_frame, sizeof(anal_frame), server->data_user_data);
			}
		}else{

			memcpy(&server->s_res[frm->type], frm, sizeof(*frm));
			server->evt[frm->type]->SetEvent();
		}
	}catch(...){
		OutputDebugString(TEXT("Hello"));
	}
}

int EPSMDServerV1::get_samp_bt(int chn, LONGLONG beg, LONGLONG end, int* count, void** anal, void** data){
	if(chn >= PSMD_MAX_CHNCT || chn < 0 || file_data[chn] == 0 || file_data[chn] == INVALID_HANDLE_VALUE){
        return -1;
	}
	if(beg > hist_end[chn] || end < hist_beg[chn]){
        return -2;
	}

	if(end > hist_end[chn]){
		end = hist_end[chn];
	}
	if(beg < hist_beg[chn]){
		beg = hist_beg[chn];
	}
	int beg_idx = get_first_frame_be_time(file_anal[chn], 10, beg);
	int end_idx = get_last_frame_le_time(file_anal[chn], 10, end);

	*count = end_idx - beg_idx + 1;


	SetFilePointer(file_data[chn], beg_idx * sizeof(EPSMDFileDataFrameV1), NULL, SEEK_SET);
	SetFilePointer(file_anal[chn], beg_idx * sizeof(EPSMDFileAnalFrameV1), NULL, SEEK_SET);

    DWORD bytes_read;
	if(anal) {
		*anal = new EPSMDFileAnalFrameV1[*count];
		ReadFile(file_anal[chn], *anal, *count * sizeof(EPSMDFileAnalFrameV1), &bytes_read, NULL);
	}
	if(data) {
		*data = new EPSMDFileDataFrameV1[*count];
		ReadFile(file_data[chn], *data, *count * sizeof(EPSMDFileDataFrameV1), &bytes_read, NULL);
	}



    return 0;
}

EPSMDServerV2::EPSMDServerV2(char* client_ip, int client_port, int local_port) :
	EPSMDUDPServer(client_ip, client_port, local_port) {
   	config_posi = &m_config;
    config_size = sizeof(m_config);
}

void EPSMDServerV2::set_config(void* config) {
    memcpy(&m_config, config, sizeof(m_config));
}

int EPSMDServerV2::stt_samp(char* file_time){
	add_hist(&m_config, sizeof(m_config), file_time, 2);
	m_config.rmde_param.rmode = 1;

	if(apply_sparam() || apply_tparam()) {
        return 3;
	}
	return apply_rmode();
}

int EPSMDServerV2::stp_samp() {
    m_config.rmde_param.rmode = 0;
    return apply_rmode();
}

int EPSMDServerV2::get_samp_bt(int chn, LONGLONG beg, LONGLONG end, int* count, void** anal, void** data){
	if(chn >= PSMD_MAX_CHNCT || chn < 0 || file_data[chn] == 0 || file_data[chn] == INVALID_HANDLE_VALUE){
        return -1;
	}
	if(beg > hist_end[chn] || end < hist_beg[chn]){
		return -2;
	}

	if(end > hist_end[chn]){
		end = hist_end[chn];
	}
	if(beg < hist_beg[chn]){
		beg = hist_beg[chn];
	}

    *count = end - beg;
	DWORD bytes_read;

	if(m_config.rmde_param.dmode == 0) {
		SetFilePointer(file_data[chn], beg * 2, NULL, SEEK_SET);
		*data = new WORD[*count];

        ReadFile(file_data[chn], *data, *count * 2, &bytes_read, NULL);
	} else {
		SetFilePointer(file_data[chn], beg * sizeof(EPSMDFileDataFrameV2), NULL, SEEK_SET);
		SetFilePointer(file_anal[chn], beg * sizeof(EPSMDFileAnalFrameV2), NULL, SEEK_SET);
		*data = new EPSMDFileDataFrameV2[*count];
		*anal = new EPSMDFileAnalFrameV2[*count];

		ReadFile(file_data[chn], *data, *count * sizeof(EPSMDFileDataFrameV2), &bytes_read, NULL);
		ReadFile(file_anal[chn], *anal, *count * sizeof(EPSMDFileAnalFrameV2), &bytes_read, NULL);
	}
	return 0;
}

void EPSMDServerV2::init_net_data_handler() {
	net_data_handler = data_handler;
}

int EPSMDServerV2::apply_tparam()
{
	return send_scmd(PSMD_UPP_V2_STPRM, &m_config.trig_param, sizeof(m_config.trig_param));

}

int EPSMDServerV2::apply_sparam() {
    return send_scmd(PSMD_UPP_V2_SSPRM, &m_config.smpl_param, sizeof(m_config.smpl_param));

}

void EPSMDServerV2::open_hist_file(LONGLONG* min, LONGLONG* max)
{
	*min = -1;
	*max = -1;

	for(int i = 0; i < PSMD_MAX_CHNCT; i++){
		char file_path[255];
		sprintf(file_path, "%s_%d.data", time_prefix, i);
		file_data[i] = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
		sprintf(file_path, "%s_%d.anal", time_prefix, i);
		file_anal[i] = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
//		hist_beg_time[i] = -1;
//		hist_end_time[i] = -1;

		LARGE_INTEGER li;
		li.QuadPart = 0;
		if(file_data[i] != INVALID_HANDLE_VALUE && file_data[i]){
			li.LowPart = SetFilePointer(file_data[i], 0, &li.HighPart, SEEK_END);
			*min = 0;
			if(m_config.rmde_param.dmode == 0) {
				*max = li.QuadPart / 2;
			} else {
                *max = li.QuadPart / 128;
			}

			hist_beg[i] = *min;
			hist_end[i] = *max;
		}
	}
}

int EPSMDServerV2::apply_rmode() {
	return send_scmd(PSMD_UPP_V2_SMODE, &m_config.rmde_param, sizeof(m_config.rmde_param));

}




void __stdcall EPSMDServerV2::data_handler(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags){
	try{
		EPPSMDIOContext 	ctx = (EPPSMDIOContext)lpOverlapped;
		EPPSMDUpperSFrame 	frm = (EPPSMDUpperSFrame)ctx->buf;
				EPSMDServerV2		*server = (EPSMDServerV2*)ctx->server;

		DWORD bytes_written = 0;

		if(frm->head == 0xAAAAAAAAAAAAAAAAULL) {
			if(frm->type >= 0xA0) {
				EPPSMDLowerNormalFrameV2 frm = (EPPSMDLowerNormalFrameV2)ctx->buf;
				int idx = frm->type & 0x0F;
				if(	true || frm->crc == calc_crc(&frm->smrt, EOFFSET_OF(EPSMDLowerNormalFrameV2, crc) - EOFFSET_OF(EPSMDLowerNormalFrameV2, smrt))){
					EPSMDFileAnalFrameV2 anal_frame;
//					while(!ctx->server->fiready) {
//						Sleep(20);
//					}

					int peak = 0x80000000;
					int pawk = 0x7FFFFFFF;

					for(int i = 0; i < (int)frm->smct; i++){
						if(peak < (int)frm->data[i])
							peak = frm->data[i];
						if(pawk > (int)frm->data[i])
							pawk = frm->data[i];
					}
					anal_frame.peak = peak;
					anal_frame.pawk = pawk;

					if(server->hist_end[idx] == -1) {
						server->hist_end[idx] = 1;
						server->hist_beg[idx] = 0;
					} else {
						server->hist_end[idx] += 1;
					}

					WriteFile(server->file_data[idx], &frm->data, sizeof(frm->data), &bytes_written, NULL);
                    WriteFile(server->file_anal[idx], &anal_frame, sizeof(anal_frame), &bytes_written, NULL);
					if(server->data_user_clbk) server->data_user_clbk(frm, sizeof(*frm), &anal_frame, sizeof(anal_frame), server->data_user_data);
				}
			} else if(frm->type >= 0x90){
				EPPSMDLowerDisplyFrameV2 frm = (EPPSMDLowerDisplyFrameV2)ctx->buf;

				int idx = frm->type & 0x0F;
				if(	true || frm->crc == calc_crc(&frm->type, EOFFSET_OF(EPSMDLowerDisplyFrameV2, crc) - EOFFSET_OF(EPSMDLowerDisplyFrameV2, type))){
//					while(!ctx->server->fiready) {
//						Sleep(20);
//					}
					unsigned short* data_cursor = (unsigned short*)frm->data;
					int len = sizeof(frm->data);
					for(int i = 627; i >= 0; i--) {
						if(data_cursor[i]) {
							break;
						} else {
							len -= 2;
						}
					}
					if(server->hist_end[idx] == -1) {
						server->hist_end[idx] = len / 2;
						server->hist_beg[idx] = 0;
					} else {
						server->hist_end[idx] += len / 2;
					}

					WriteFile(server->file_data[idx], &frm->data, sizeof(frm->data), &bytes_written, NULL);
					if(server->data_user_clbk) server->data_user_clbk(frm, sizeof(*frm), NULL, 0, server->data_user_data);
				}
			}
		}
		else
		{
			int index = frm->type;
			switch(index)
			{
			case 0x80:
				index = PSMD_UPP_V2_RPARA;
				break;
			}
			memcpy(&server->s_res[index], frm, cbTransferred);
			server->evt[index]->SetEvent();

	}
	}catch(...){
		OutputDebugString(TEXT("Hello"));
	}
}

EPSMDServer24C::EPSMDServer24C(char* client_ip, int client_port, int local_port) :
	EPSMDTCPServer(client_ip, client_port, local_port) {
//	config_posi = &m_config;
//	config_size = sizeof(m_config);
	//add for create 24file
   FileNumbersflag=0;
   MyReadFromSDCardCount=0;

  /*
   BuffShift[0]=19;
   BuffShift[1]=18;
   BuffShift[2]=17;
   BuffShift[3]=16;
   BuffShift[4]=13;
   BuffShift[5]=12;
   BuffShift[6]=15;
   BuffShift[7]=14;
   BuffShift[8]=23;
   BuffShift[9]=22;
   BuffShift[10]=21;
   BuffShift[11]=20;
   BuffShift[12]=9;
   BuffShift[13]=8;
   BuffShift[14]=11;
   BuffShift[15]=10;
   BuffShift[16]=7;
   BuffShift[17]=6;
   BuffShift[18]=5;
   BuffShift[19]=4;
   BuffShift[20]=1;
   BuffShift[21]=0;
   BuffShift[22]=3;
   BuffShift[23]=2;
   */
   BuffShift[0]=2;
   BuffShift[1]=3;
   BuffShift[2]=0;
   BuffShift[3]=1;
   BuffShift[4]=6;
   BuffShift[5]=7;
   BuffShift[6]=4;
   BuffShift[7]=5;
   BuffShift[8]=20;
   BuffShift[9]=21;
   BuffShift[10]=22;
   BuffShift[11]=23;
   BuffShift[12]=10;
   BuffShift[13]=11;
   BuffShift[14]=8;
   BuffShift[15]=9;
   BuffShift[16]=16;
   BuffShift[17]=17;
   BuffShift[18]=18;
   BuffShift[19]=19;
   BuffShift[20]=14;
   BuffShift[21]=15;
   BuffShift[22]=12;
   BuffShift[23]=13;
}

void EPSMDServer24C::get_hist_list() {
	send_scmd(0x05, NULL, 0);
}
/////////////////////////add////////////设置采样率  add start

//get localtime
int EPSMDServer24C::GetLocalTimeFun(char *TimeParaBuf)
{
    return send_scmd(0x00,TimeParaBuf,53);
}
int EPSMDServer24C::SetSampleRateForSDCard(char *RateParaBuf)
{
	 return send_scmd(0x01,RateParaBuf,53);
}
///////开始采样
int EPSMDServer24C::SendStSampleCommand(char *SampleCommandBUf)
{
	return send_scmd(0x02,SampleCommandBUf,53);
}
//////查询余量
int EPSMDServer24C::Query_remain(char *SDPareBuf)
{
	return  send_scmd(0x03, SDPareBuf, 53);
}
////stop sampimng
int EPSMDServer24C::Stop_SampleCommand(char *STParaBuf)
{
	 return send_scmd(0x02,STParaBuf,53);
}
//Query filenames
int	EPSMDServer24C::QueryFileNameFun(char *QueParaBuf)
{
	 return  send_scmd(0x05,QueParaBuf,53);
}
//formatsdcard  格式化sd卡
int	EPSMDServer24C::FormatSDcardFun(char *FormatParaBuf)
{
		return  send_scmd(0x04,FormatParaBuf,53);
}
//read FIles Header from SDcard
int	EPSMDServer24C::ReadFileHeaderFromSDcardFun(char *FileNameBuf)
{
		return  send_scmd(0x07,FileNameBuf,53);
}
//read Files data from SDcard
int	EPSMDServer24C::ReadFilesDataFromSDcardFun(char *FileNameParaBuf)
{
	 return  send_scmd(0x08,FileNameParaBuf,53);
}
//Delete files from SDcard
int EPSMDServer24C::DelFileNamesFromSDcardFun(char *FileNameBuf)
{
		return send_scmd(0x06, FileNameBuf,53);
}
////////////////////////////////////////////////////add  end
//int EPSMDServer24C::delete_hist(char *file_name) {
//		return send_scmd(0x06, file_name, strlen(file_name) + 1);
//}

int EPSMDServer24C::download_file(char *file_name, char *path) {
	downloading_file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
	int res = send_scmd(0x07, file_name, strlen(file_name) + 1);
	if(res) {
        return 1;
	}
	unsigned long bytes_written;
	WriteFile(downloading_file, reading_buff + 9, 53, &bytes_written, NULL);
}

void EPSMDServer24C::init_net_data_handler() {
	net_data_handler = data_handler;
	reading_type = 0;
	reading_cusr = 0;
}

void __stdcall EPSMDServer24C::data_handler(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags){
	try
	{
		EPPSMDIOContext 	ctx = (EPPSMDIOContext)lpOverlapped;
		EPSMDServer24C*     server = (EPSMDServer24C*)ctx->server;
		/////add
		EPPSMDUpperSFrame 	frm = (EPPSMDUpperSFrame)ctx->buf;
    //////test end
		char *cursor = ctx->buf;
		while(cbTransferred) //receive datas length
		{
			if(server->reading_cusr < 8)
			{
				if(server->reading_cusr == 0 && cbTransferred >= 8)
				{  //启动采集
					if(*(unsigned __int64*)cursor == 0xAAAAAAAAAAAAAAAAull)
					{
						cbTransferred-= 8;
						cursor += 8;
						server->reading_type = 0xAA;
						server->reading_cusr += 8;
					}
                    // read SDcard files data
					else if(*(unsigned __int64*)cursor == 0xBBBBBBBBBBBBBBBBull)
					{
                        cbTransferred-= 8;
						cursor += 8;
						server->reading_type = 0xBB;
						server->reading_cusr += 8;
                    }
				    /////////add for SDcard   start
					else if(*(unsigned __int64*)cursor == 0x5555555555555555ull)
					{
						cbTransferred -= 8;
						cursor += 8;
						server->reading_type = 0x55;
						server->reading_cusr += 8;
					}
					//////////////////add end
					else
					{
						server->reading_type = 0;
						server->reading_cusr = -1;
					}
				}
				else
				{
					char data = *cursor;
					if(server->reading_cusr <= 0)
					{
						if(data == (char)0xAA || data == (char)0x55 || data == (char)0xBB)
						{
							server->reading_type = data;
							server->reading_cusr = 1;
						}
						else
						{
							server->reading_type = 0;
							server->reading_cusr = -1;
						}
						cbTransferred--;
						cursor++;
					}
					else
					{
						if(data == server->reading_type) {
							server->reading_cusr ++;
							cbTransferred--;
							cursor++;
						} else {
							server->reading_type = 0;
							server->reading_cusr = -1;
						}
					}
				}
			}
			else
			{
				int length = server->reading_type == (char)0x55 ? 64 : 14419;
				int curlen = length - server->reading_cusr;
				if(curlen > (int)cbTransferred) {
					curlen = cbTransferred;
				}
				memcpy(&server->reading_buff[server->reading_cusr], cursor, curlen);
				cursor += curlen;
				server->reading_cusr += curlen;
				cbTransferred -= curlen;

				if(server->reading_cusr == length)
				{
				//////////////////////////////////////////////////////////
					if(server->reading_type == (char)0x55)
					{
						EPPSMDUpperSFrame frame = (EPPSMDUpperSFrame)server->reading_buff;
					 // server->evt[frame->type]->SetEvent(); 信号量状态置1
					 //get localtime
						if(frame->type == 0x00)
						{
						  server->evt[frame->type]->SetEvent();

						}
                     //add for setSamplerate
						if(frame->type == 0x01)
						{
            	            server->evt[frame->type]->SetEvent();
							//char buf[65]={0};
							int Samparalen=(sizeof(frame->parm)>53?64:sizeof(frame->parm));
							for(int i=0;i<Samparalen;i++)
							{
								SamRateRecBuf[i]=frame->parm[i];
							}

						}
						//stop samping
						if(frame->type==0x02&&frame->parm[0]==0x00)
						{
							server->evt[frame->type]->SetEvent();
							char buf[2];
							sprintf(StopSampRcvBuf,"%02x",frame->parm[0]);
						}
						//start samping
						if(frame->type==0x02&&frame->parm[0]!=0x00)
						{
                            server->evt[frame->type]->SetEvent();
            }
						//add for sdcard
						if(frame->type == 0x03)
						{
							server->evt[frame->type]->SetEvent();
							char Volbuf[9]={0};
							//低字节在前
							sprintf(Volbuf,"%02x%02x%02x%02x",frame->parm[3],frame->parm[2],frame->parm[1],frame->parm[0]);
							SDVol=(float)HextoInt32(Volbuf)/1024.0;
						}
						//SDcard 格式化
						if(frame->type == 0x04)
						{
							server->evt[frame->type]->SetEvent();
						}
						// 查询文件名
						if(frame->type==0x05)
						{
							char bufCount1[5]={0};
							char bufCount2[5]={0};
							//int FilesNum=0;
							//char bufCount2[5]={0};
							sprintf(bufCount1,"%02x%02x",frame->parm[1],frame->parm[0]); //文件数量
							sprintf(bufCount2,"%02x%02x",frame->parm[3],frame->parm[2]); //防止重复赋值
							if(HextoInt32(bufCount2) == 1)
							{
								server->FileNumbers=HextoInt32(bufCount1);
							}
							for(int i=4;i<24;i++)
							{
								server->FileNamelist[server->FileNumbersflag].parm[i-4]=frame->parm[i];
							}
							server->FileNamelist[server->FileNumbersflag].Fileindex=server->FileNumbersflag;
							server->FileNumbersflag++;

							if(server->FileNumbers==server->FileNumbersflag)
							{
              					server->evt[frame->type]->SetEvent();
							}
						}
						//删除文件
						if(frame->type==0x06)
						{
							server->evt[frame->type]->SetEvent();
						}

						//文件头信息
						if(frame->type==0x07)  //文件头保存在对应的文件里
						{
							char FilesLengBuf[16]={0};
							sprintf(FilesLengBuf,"%02x%02x%02x%02x%02x%02x%02x%02x",frame->parm[20],frame->parm[19],frame->parm[18],frame->parm[17],
											frame->parm[16],frame->parm[15],frame->parm[14],frame->parm[13]);
							server->ReadSDCardFilesLeng=HextoInt64(FilesLengBuf);
							server->evt[frame->type]->SetEvent();
							char buf[11]={0};
							fwrite(frame->parm,53,1,server->fp2);
							//文件填充够一帧长度64Byte
							fwrite(buf,11,1,server->fp2);
						}
					}
					else if(server->reading_type == (char)0xAA)
					{
						// 文件内容的保存，在这里，按对应格式写入downloading_file里
					 //	EPPSMDUpperSFrame24 frame = (EPPSMDUpperSFrame24)server->reading_buff;
					 //	server->evt[frame->type]->SetEvent();
//						DWORD dwWritenSize = 0;
//						HANDLE downloading_file = CreateFileA("14419file.data", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, 0);
//						WriteFile(downloading_file, server->reading_buff,server->reading_cusr,&dwWritenSize,NULL);
//							if(server->fp)
//							{
//								fwrite(server->reading_buff,14419,1,server->fp);
//							}

					 server->evt[2]->SetEvent();

					}
					else if(server->reading_type == (char)0xBB)  //读文件到本地
					{

						if(server->fp2)
						{
							memcpy(server->reading_buf2, server->reading_buff, 14419);
							for(int ii=0;ii<200;ii++)
							{
								for(int jj=0;jj<24;jj++)
								{
									memcpy(&server->reading_buff[17+ii*72+jj*3], &server->reading_buf2[17+ii*72+server->BuffShift[jj]*3],3);
								}
							}
//							if(server->fpFloat)
//							{
//                                server->pFile=server->reading_buff+17;
//								for(int i=0;i<4800;i++)
//								{
//									memcpy((char*)(&server->IntForFloat)+1,server->pFile,3);
//									server->pFile+=3;
//									server->reading_buffloat[i]=server->IntForFloat;
//									server->IntForFloat=0;
//								}
//								//fwrite(server->reading_buffloat,4800*4,1,server->fpFloat);
//							}


							fwrite(server->reading_buff+17,14400,1,server->fp2);
							server->MyReadFromSDCardCount+=14400;
                            //文件格式：文件头（64）+数据
							if(server->MyReadFromSDCardCount==(server->ReadSDCardFilesLeng-64))//文件头64字节先写文件
							{
								fclose(server->fp2);
								//m_status_bar->SimpleText = "文件读取成功.";
								//fclose(server->fpFloat);
							}
						}
					 server->evt[8]->SetEvent();
					}
					server->reading_cusr = 0;
					server->reading_type = 0;
					//fclose(fp);
				}
			}
		}
	}catch(...){
		OutputDebugString(TEXT("Hello"));
	}
}

