//---------------------------------------------------------------------------

#ifndef psmd_serverH
#define psmd_serverH

#include<vector>
#include<iostream>
using namespace std;

#include 	<Vcl.h>
#include 	<windows.h>
#include 	"sqlite3.h"
#include	"System.Classes.hpp"
#include	<math.h>
//---------------------------------------------------------------------------

#define PSMD_PROTOCOL_UDP 0x01
#define PSMD_PROTOCOL_TCP 0x02

#define PSMD_UPP_V1_STIME 0x00
#define PSMD_UPP_V1_SSMPL 0x01
#define PSMD_UPP_V1_SVALV 0x02
#define PSMD_UPP_V1_STTSP 0x03
#define PSMD_UPP_V1_STPSP 0x04
#define PSMD_UPP_V1_GVESN 0x05

#define PSMD_UPP_V2_RPARA 0x01
#define PSMD_UPP_V2_STPRM 0x02
#define PSMD_UPP_V2_SSPRM 0x03
#define PSMD_UPP_V2_SMODE 0x04
#define PSMD_UPP_V2_GADDR 0x05
//////////////////////////////////////
#define PSMD_UPP_V24_RPARA 0x01
#define PSMD_UPP_V24_STPRM 0x02
#define PSMD_UPP_V24_SSPRM 0x03
#define PSMD_UPP_V24_SMODE 0x04
#define PSMD_UPP_V24_GADDR 0x05
////////////////////////////////////////
#define PSMD_DWN_S500M 0x10
#define PSMD_DWN_D500F 0x20
#define PSMD_DWN_D500S 0x21
#define PSMD_DWN_T1GSM 0x30

#define PSMD_MAX_CHNCT 2

#define EOFFSET_OF(type,member) ((size_t) &((type*)0)->member)

#pragma pack(push, 1)


//////add   query filenames
typedef struct{

	unsigned char	parm[24];
	int Fileindex;
}EPSMDTcpQueryFile24,*EpPSMDTcpQueryFile24;


///int hex2int(char c);



///////  add for 14419
typedef struct{
	unsigned __int64 	head;
	unsigned char       type;
	unsigned char		parm[14408];
	unsigned short		crc;
}	EPSMDUpperSFrame24, *EPPSMDUpperSFrame24;

//短帧
typedef struct{
	unsigned __int64 	head;
	unsigned char       type;
	unsigned char		parm[53];
	unsigned short		crc;
}	EPSMDUpperSFrame, *EPPSMDUpperSFrame;
//长帧
typedef struct {
	unsigned __int64	head;
	unsigned char		type;
	unsigned char		parm[245];
	unsigned short		crc;
}	EPSMDUpperLFrame, *EPPSMDUpperLFrame;

typedef struct{
	unsigned __int64   	head;
	unsigned char		type;
	unsigned __int64	time;
	unsigned short  	numb;
	unsigned short      resv;
	unsigned char       data[1256];
	unsigned short		crc;
}	EPSMDLowerFrameV1, *EPPSMDLowerFrameV1;

typedef struct {
	unsigned __int64	head;
	unsigned char		type;
	unsigned char		smrt;
	unsigned int		smct;
	int					data[32];
	unsigned char		reserved[112];
    unsigned short		crc;
}	EPSMDLowerNormalFrameV2, *EPPSMDLowerNormalFrameV2;

typedef struct{
	unsigned __int64   	head;
	unsigned char		type;
	unsigned char       data[1256];
	unsigned short		crc;
}	EPSMDLowerDisplyFrameV2, *EPPSMDLowerDisplyFrameV2;

typedef struct {
	unsigned __int64 	time;
	unsigned char    	peak;
	unsigned char      	pawk;
} 	EPSMDFileAnalFrameV1, *EPPSMDFileAnalFrameV1;

typedef struct {
	int	peak;
	int	pawk;
}	EPSMDFileAnalFrameV2, *EPPSMDFileAnalFrameV2;

//typedef struct {
//	unsigned char  freq;
//}	ELowerSampParam, *EPLowerSampParam;

typedef struct {
	unsigned char	mode;
	unsigned short	L;
	unsigned short	D;
	unsigned short	W;
	unsigned short	A1;
	unsigned short	A2;
	unsigned char	N;
	unsigned short	M[20];
}	EPSMDTrigParamV2, *EPPSMDTrigParamV2;

typedef struct {
	unsigned char valv_u;
	unsigned char valv_d;
}	EPSMDTrigParamV1, *EPPSMDTrigParamV1;

typedef struct {
	unsigned char 	rmode;
	unsigned char 	dmode;
	unsigned int    stime;
}	EPSMDRmdeParamV2, *EPPSMDRmdeParamV2;

typedef struct {
	unsigned char	freq;
	unsigned short	valv;
}	EPSMDSampParamV2, *EPPSMDSampParamV2;

typedef struct {
	unsigned char mode;
}	EPSMDSampParamV1, *EPPSMDSampParamV1;

//typedef struct {
//	unsigned __int64	head;
//	unsigned char   	param;
//	ELowerSampParam 	samp;
//	ELowerTrigParam 	trig;
//	unsigned char		resv[43];
//	unsigned short		crc;
//}	ELowerParamFrame, *EPLowerParamFrame;

typedef struct {
	unsigned __int64	time;
	unsigned short  	numb;
	unsigned short      resv;
	unsigned char       data[1256];
} 	EPSMDFileDataFrameV1, *EPPSMDFileDataFrameV1;

typedef struct {
	int	data[32];
} 	EPSMDFileDataFrameV2, *EPPSMDFileDataFrameV2;

//typedef struct{
//	LONGLONG 	time;
//	int 		mode;
//	int 		peak;
//	int 		pawk;
//}   EPSMDConfigV1, *EPPSMDConfigV1;

typedef struct {
	EPSMDTrigParamV1 	tparam;
	EPSMDSampParamV1 	sparam;
} EPSMDConfigV1, *EPPSMDConfigV1;

typedef struct {
	EPSMDRmdeParamV2 rmde_param;
	EPSMDSampParamV2 smpl_param;
	EPSMDTrigParamV2 trig_param;
} 	EPSMDConfigV2, *EPPSMDConfigV2;

#pragma pack(pop)
unsigned __int32  HextoInt(char *str);//add for query Sdcard  capacity
unsigned short calc_crc(void* beg, int len);
typedef int(*DATA_CALLBACK)(void* data, int data_len, void* anal, int anal_len, void* user_data);
typedef int(*HIST_CALLBACK)(int type, char* time, void* user_data);
typedef void __stdcall (*NET_DATA_HANDLER)(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);
typedef unsigned long __stdcall (*SERVER_WORKER)(void* param);

class EPSMDServerBase {
public:
	EPSMDServerBase(char* client_ip, int client_port, int local_port/*, int protocol*/);
	int get_version();
	virtual void get_hist_list();
	int set_time(unsigned __int64 time);
	void set_data_handler(DATA_CALLBACK cbk, void* user_data);
	void set_hist_handler(HIST_CALLBACK cbk, void* user_data);
	void set_hist_time(char* time, void* config, LONGLONG* min, LONGLONG* max);

	int start();
	virtual void set_config(void* config) {}
	virtual int stt_samp(char* file_time) { return 0; }
	virtual int stp_samp() { return 0; }
	////////////////////////add//////////////////////////////
	//get loacla time
	virtual int GetLocalTimeFun(char *TimeParaBuf)	{return 0;}
	// set sample rate
	virtual int SetSampleRateForSDCard(char *RateParaBuf)	{return 0;}
	//sart sample
	virtual int	SendStSampleCommand(char *SampleCommandBUf) {return 0;}
	//stop samping
	virtual int Stop_SampleCommand(char *STParaBuf)	{return 0;}
	//查询sd卡余量   add
	virtual int Query_remain(EPSMDUpperSFrame *ShortFramPare) { return 0; }
	//Query 文件名
	virtual int	QueryFileNameFun(char *QueParaBuf)	{ return 0;}
	//format 格式化Sdcard
	virtual int	FormatSDcardFun(char *FormatParaBuf)	{return 0;}
	//读取文件头信息
	virtual int	ReadFileHeaderFromSDcardFun(char *FileNameBuf)	{return 0;}
	//从SDcard读文件内容
	virtual int ReadFilesDataFromSDcardFun(char *FileNameParaBuf) {return 0;}
	//删除SDcard文件内容
	virtual int	DelFileNamesFromSDcardFun(char *FileNameBuf){return 0;}

	SOCKET 		server_socket;//add for close socket 20200608
/////////////////////////////////end////////////////////////////////
	virtual int get_samp_bt(int chn, LONGLONG beg, LONGLONG end, int* count, void** anal, void** data) { return 0; }
	virtual ~EPSMDServerBase();
private:
	static int history_item(void* data, int argc, char** argv, char** col_name);
	static int history_config(void* data, int argc, char** argv, char** col_name);
	static unsigned long __stdcall server_worker(void* param);
protected:
	virtual int init_net(char* client_ip, int client_port, int local_port) {}
	void clr_hist();
	void init_mem();
	void init_database();
	void add_hist(void* config, int size, char* file_time, int file_count);
	int send_scmd(int type, void* buf, int len);
	int send_lcmd(int type, void* buf, int len);
	virtual void init_net_data_handler() {}
	virtual int apply_tparam() { return 0; }
	virtual int apply_sparam() { return 0; }
	virtual void open_hist_file(LONGLONG* min, LONGLONG* max) {}
protected:
	bool        working;
	//SOCKET 		server_socket;    add by 20200608

	char*       m_errmsg;
	sqlite3*	m_dtbase;

	HANDLE 		worker_handle;
	DWORD       worker_identi;

	char		client_ip[64];
	int			client_port;
	int			local_port;
protected:
	SERVER_WORKER	 net_data_worker;
	NET_DATA_HANDLER net_data_handler;
	bool		file_ready;

	void*		data_user_data;
	DATA_CALLBACK data_user_clbk;
	void*       hist_user_data;
	HIST_CALLBACK hist_user_clbk;

  	char        beg_time[32];
	char        time_prefix[255];

	HANDLE      file_data[PSMD_MAX_CHNCT];
	HANDLE		file_anal[PSMD_MAX_CHNCT];

	LONGLONG    hist_beg[PSMD_MAX_CHNCT];
	LONGLONG	hist_end[PSMD_MAX_CHNCT];
#ifdef _DEBUG
	HANDLE		file_debg[PSMD_MAX_CHNCT];
#endif
	TEvent*     evt[255];
	EPSMDUpperLFrame l_req[256];
	EPSMDUpperLFrame l_res[256];

	EPSMDUpperSFrame s_req[256];
	EPSMDUpperSFrame s_res[256];

	int 		config_size;
	void*       config_posi;

//	int read_param(unsigned char *mode, unsigned short *W1, unsigned short *W2, unsigned short *A1, unsigned short *A2);
};

class EPSMDUDPServer : public EPSMDServerBase {
public:
	EPSMDUDPServer(char* client_ip, int client_port, int local_port);
protected:
	virtual int init_net(char* client_ip, int client_port, int local_port);
	static unsigned long __stdcall server_worker(void* param);
};

class EPSMDTCPServer : public EPSMDServerBase {
public:
	EPSMDTCPServer(char* client_ip, int client_port, int local_port);
protected:
	virtual int init_net(char* client_ip, int client_port, int local_port);
	static unsigned long __stdcall server_worker(void* param);
};

typedef struct
{
	OVERLAPPED			overlapped;

	WSABUF				wsabuf;
	EPSMDServerBase*	server;
	unsigned char		buf[2048];
} EPSMDIOContext, *EPPSMDIOContext;

class EPSMDServerV1 : public EPSMDUDPServer{
public:
	EPSMDServerV1(char* client_ip, int client_port, int local_port);
	virtual void set_config(void* config);
	virtual int stt_samp(char* file_time);
	virtual int stp_samp();
//	virtual void set_hist_time(char* time, LONGLONG* min, LONGLONG* max);
	virtual int get_samp_bt(int chn, LONGLONG beg, LONGLONG end, int* count, void** anal, void** data);
protected:
	virtual void init_net_data_handler();
	virtual int apply_tparam();
	virtual int apply_sparam();
	virtual void open_hist_file(LONGLONG* min, LONGLONG* max);
private:
	static void __stdcall data_handler(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);
private:
	EPSMDConfigV1 m_config;

	LONGLONG last_pkg_time[PSMD_MAX_CHNCT];
//	int read_param(unsigned char *mode, unsigned short *W1, unsigned short *W2, unsigned short *A1, unsigned short *A2);
};

class EPSMDServerV2 : public EPSMDUDPServer{
public:
	EPSMDServerV2(char* client_ip, int client_port, int local_port);
	virtual void set_config(void* config);
	virtual int stt_samp(char* file_time);
	virtual int stp_samp();
//	virtual void set_hist_time(char* time, LONGLONG* min, LONGLONG* max);
	virtual int get_samp_bt(int chn, LONGLONG beg, LONGLONG end, int* count, void** anal, void** data);
protected:
	virtual void init_net_data_handler();
	virtual int apply_tparam();
	virtual int apply_sparam();
	virtual void open_hist_file(LONGLONG* min, LONGLONG* max);
private:
	int apply_rmode();
private:
	static void __stdcall data_handler(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);
private:
	EPSMDConfigV2 m_config;
};

class EPSMDServer24C : public EPSMDTCPServer {
public:
		//vector<AnsiString>   FileNameList;
	EPSMDTcpQueryFile24 FileNamelist[1000];
	FILE *fp;
	FILE *fp2;
	FILE *fpFloat;
	int FileNumbersflag;
	int FileNumbers;
	__int64 ReadSDCardFilesLeng;
	__int64 MyReadFromSDCardCount;
  //vector<AnsiString> FileNameList;
	EPSMDServer24C(char* client_ip, int client_port, int local_port);
	virtual void get_hist_list();
	virtual void init_net_data_handler();
	//////////////add start///////////////////////////////
	virtual int GetLocalTimeFun(char *TimeParaBuf);	//get loacaltime
	virtual int SetSampleRateForSDCard(char *RateParaBuf); //set sample rate
	virtual int	SendStSampleCommand(char *SampleCommandBUf);//start sample
	virtual int Query_remain(char *SDParaBuf);//查询SD卡余量
	virtual int Stop_SampleCommand(char *STParaBuf);//stop sampling;
	virtual int	QueryFileNameFun(char *QueParaBuf);//Query all of filenames
	virtual int	FormatSDcardFun(char *FormatParaBuf);//格式化Sd卡
	virtual int ReadFilesDataFromSDcardFun(char *FileNameParaBuf);  //ReadFilesData from SDcard
	virtual int	ReadFileHeaderFromSDcardFun(char *FileNameBuf);//ReadFileHeader  from SDcard
	virtual int	DelFileNamesFromSDcardFun(char *FileNameBuf);   //DeleteFileName from SDcard
	////////////////////////end//////////////////////////////////////////
	//int delete_hist(char* name);
	int download_file(char* name, char* path);
private:
	static void __stdcall data_handler(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);
private:
	char reading_type;
	int reading_cusr;
	HANDLE downloading_file;

	//TStrings  FileNameList;
	__int32 IntForFloat;
	char reading_buff[20480];
	char reading_buf2[20480];
	int  BuffShift[24];
	__int32 reading_buffloat[4800];
	char *pFile;
};
#endif
