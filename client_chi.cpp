//#include <mutex>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
//#include "camfun.h"
#include <conio.h>
#include <stdio.h>
#include "Header.h"
#include <time.h>
#include <modbus.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <Windows.h>
#include <thread>
#include <winsock.H> 
#include <WS2tcpip.h>
//#define WINSOCK_DEPRECATED_NO_WARNINGS 0
#pragma comment(lib, "ws2_32.lib") 
#pragma warning(disable:4996)

using namespace std;

vector<int> spilt_string(string str)
{
	vector<int> locs;
	for (int i = 0; i < str.length(); i++)
	{
		if (str[i] == ',')
		{
			locs.push_back(i);
		}
	}
	vector<int> spilt_int;
	for (int i = 0; i < locs.size(); i++)
	{
		string temp;
		if (i == 0)
		{
			temp.append(str, 0, locs[i]);
			spilt_int.push_back(stoi(temp));
		}
		else
		{
			temp.append(str, locs[i - 1] + 1, locs[i]);
			spilt_int.push_back(stoi(temp));
		}
	}
	string temp;
	temp.append(str, locs[locs.size() - 1] + 1, str.length() - 1);
	spilt_int.push_back(stoi(temp));
	return spilt_int;
}
void thread_modbus(modbus_t * mb)
{
	Sleep(1000);
	modbus_write_register(mb, 0, 2400);
}
//void thread_point(point_t)
//{
//point = { xx,0 + y_go,155,angle_a,0,-180 };
//}

//void thread_sleep(sleep * sp)
//{
//	sleep(sp);
//}

enum {
	TCP,
	TCP_PI,
	RTU
};

int main()
{
	//socket client
	//string ipAddress = "127.0.0.1";			// IP Address of the server
	string ipAddress = "192.168.225.56";			// IP Address of the server   // wifi與筆電連線
	int port = 7000; //4位數要相同
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		cerr << "Can't start Winsock, Err #" << wsResult << endl;
	}
	// Create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		cerr << "Can't create socket, Err #" << WSAGetLastError() << endl;
		WSACleanup();
	}
	// Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);
	// Connect to server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
		cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
		closesocket(sock);
		WSACleanup();
	}
	else
	{
		modbus_t *ctx;
		modbus_mapping_t *mb_mapping;

		//open robot 開手臂
		RTN_ERR ret = 0;
		I32_T   devIndex = 0;
		I32_T   retDevID = 0;
		I32_T   retGroupCount = 0;
		I32_T   retGroupAxisCount = 0;
		I32_T   GroupIndex = 0;
		F64_T   paravalue = 0;
		//int tg = 0;
		//int movev = 20;
		//fstream fp;

#ifdef UNDER_WIN32_SIMULATION
		I32_T devType = NMC_DEVICE_TYPE_SIMULATOR;
		U32_T sleepTime = 1000;
#else
		I32_T devType = NMC_DEVICE_TYPE_ETHERCAT; //指定驅動裝置型態
		U32_T sleepTime = 2000;
#endif

		OpenDevice(devType, devIndex, retDevID, sleepTime, retGroupCount, retGroupAxisCount);
		SafyOn(retDevID, sleepTime, retGroupCount);
		ResetAlarm(retDevID, sleepTime, retGroupCount);
		SetBreak(Break_Off, retDevID, sleepTime, retGroupCount);
		GetPoseValue(retDevID, GroupIndex, retGroupCount, sleepTime);
		Pos_T HomePos = { 0,90,0,0,0,0 }; //手臂最一開始的位置(角度)
		SetHome(HomePos, retDevID, sleepTime, retGroupCount, GroupIndex, retGroupAxisCount);

		ret = NMC_GroupSetParamI32(retDevID, GroupIndex, 0x30, 0, 0);
		if (ret != 0) {
			cout << "ERROR! NMC_GroupSetParamI32:" << NMC_GetErrorDescription(ret, NULL, 0) << endl; //NMC_GetErrorDescription 回傳錯誤碼描述C字串
			CloseDevice(retDevID, sleepTime, retGroupCount); //關閉設備
		}
		else {
			cout << "set abs success" << endl;
		}

		ret = NMC_GroupSetVelRatio(retDevID, GroupIndex, 100); //群組轉速度百分比，範圍0~100%
		if (ret != 0) {
			cout << "ERROR! NMC_GroupSetVelRatio:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		else {
			cout << "feed ratio : 100%" << endl;
		}

		for (int j = 0; j < 6; j++) {
			ret = NMC_GroupAxSetParamF64(retDevID, GroupIndex, j, 0x32, 0, 30);
		}


		//*********************************************************************************************
		Pos_T standbypos = { -6,45,-16,0,-28,-6 };// angles 手臂運作前等待位置 原本{0,55,-31,0,-25,0}
		ret = NMC_GroupSetParamI32(retDevID, GroupIndex, 0x30, 0, 0);
		if (ret != 0) {
			cout << "ERROR! NMC_GroupSetParamI32:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		ret = NMC_GroupPtpAcsAll(retDevID, GroupIndex, 63, &standbypos);
		if (ret != 0) {
			cout << "ERROR! NMC_GroupPtpAcsAll:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
		if (ret != 0)
		{
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}

		//open modbus 開夾抓
		ctx = modbus_new_rtu("COM7", 9600, 'N', 8, 1);
		if (ctx == NULL)
		{
			fprintf(stderr, "modbus doesn't control");
			return -1;
		}
		else {
			cout << "modbus connected!" << endl;
		}
		modbus_set_debug(ctx, 1);
		modbus_set_slave(ctx, 1);
		modbus_connect(ctx);
		uint16_t reg[64];
		int num = modbus_read_registers(ctx, 0, 5, reg);
		if (num != 5)
		{
			fprintf(stderr, "failed to read:", modbus_strerror(errno));
		}
		modbus_write_register(ctx, 0, 400);

		char buf[256];
		string Output;
		bool isstoped = true;
		int time = 3100;   //下去的速度，3100斜的極致 for 0 
		while (isstoped)
		{
			Output = "start";
			int sendResult = send(sock, Output.c_str(), Output.size() + 1, 0);
			if (sendResult != SOCKET_ERROR)
			{
				std::cout << "Send> " << Output << endl;
				// Wait for response
				ZeroMemory(buf, 256);
				int bytesReceived = recv(sock, buf, 256, 0);
				std::cout << "recv: " << bytesReceived << endl;
				if (bytesReceived > 0)
				{
					//******************************************************************************************************************************
					string recive = string(buf, 0, bytesReceived); // x,y,z,agle(0,1,2,3)
					vector<int> information = spilt_string(recive);
					if (information.size() != 0)
					{
						float pos_x, pos_y, pos_y1, pos_z, angle_a, timego, L, cy, trans, shot, V, lomg;
						float x_drift;
						pos_x = information[0];
						pos_y = -50;
						pos_z = 220;  //information[2] //190
						pos_y1 = 2; //下來抓的y位置
						angle_a = -information[3];
						cy = information[1];
						L = abs(cy - 280);
						trans = (338 - 204 / 120);
						shot = 7.21;
						V = 300 * trans / shot;
						lomg = 13.56;//13.50 for 0
						//timego = 11400;//((L / V) + lomg) * 1000 - time;//information[4];

																//int sleep_time = information[0];
						if (angle_a >= 75 && angle_a <= 90) {
							timego = 11600;
							x_drift = 351;
							cout << "loop 1" << endl;
						}
						else if (angle_a >= 60 && angle_a < 75) {
							timego = 11450;
							x_drift = 352;
							cout << "loop 2" << endl;
						}
						else if (angle_a >= 45 && angle_a < 60) {
							timego = 11350;
							x_drift = 350;
							cout << "loop 3" << endl;
						}
						else if (angle_a >= 30 && angle_a < 45) {
							timego = 11200;
							x_drift = 349;
							cout << "loop 4" << endl;
						}
						else if (angle_a >= 15 && angle_a < 30) {
							timego = 11100;
							x_drift = 351;
							cout << "loop 5" << endl;
						}
						else if (angle_a >= 0 && angle_a < 15) {
							timego = 11000-50;
							x_drift = 350;
							cout << "loop 6" << endl;
						}
						else if (angle_a <= -75 && angle_a >= -90) {
							timego = 11550;
							x_drift = 350;
							cout << "loop 7" << endl;
						}
						else if (angle_a <= -60 && angle_a > -75) {
							timego = 11450;
							x_drift = 350;
							cout << "loop 8" << endl;
						}
						else if (angle_a <= -45 && angle_a > -60) {
							timego = 11350;
							x_drift = 348;
							cout << "loop 9" << endl;
						}
						else if (angle_a <= -30 && angle_a > -45) {
							timego = 11200;
							x_drift = 351;
							cout << "loop 10" << endl;
						}
						else if (angle_a <= -15 && angle_a > -30) {
							timego = 11100;
							x_drift = 350;
							cout << "loop 11" << endl;
						}
						else if (angle_a < 0 && angle_a > -15) {
							timego = 11000;
							x_drift = 350;
							cout << "loop 12" << endl;
						}


						float y_go = 250; //手臂跟的距離
						float z_wait = 25; //手臂等待的高度

						//ret = NMC_GroupSetParamI32(retDevID, GroupIndex, 0x30, 0, 0);
						if (ret != 0) {
							cout << "ERROR! NMC_GroupSetParamI32:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
							CloseDevice(retDevID, sleepTime, retGroupCount);
						}
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x28, 0, 70);
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x32, 0, 100);
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x33, 0, 100);

						float xx; 
						xx = ((435 - pos_x) * 0.782) + x_drift;//座標轉換學一下怎麼算? 相機拍攝到的x轉換成手臂的y,相機拍攝到的y轉換成手臂的x
					

						Pos_T point = { xx,pos_y,pos_z + z_wait,angle_a,0,-180 };
						cout << "move to {  " << xx << ',' << pos_y << ',' << pos_z + z_wait << ',' << angle_a << ',' << timego << '}' << endl;
	
						//cout << "move to  " << point.pos << endl;
						//NMC_GroupLine(retDevID, GroupIndex, 63, &point, NULL);
						NMC_GroupPtpCartAll(retDevID, GroupIndex, 63, &point); //卡式座標(裝置識別號,組群識別號,卡式座標軸組合碼,目標座標位置)
							
						Sleep(timego);//看相機拍攝點到手臂抓取點要多少時間
						if (ret != 0) {
							cout << "ERROR! NMC_Groupline:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;

							break;
						}
						//ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
						if (ret != 0)
						{
							CloseDevice(retDevID, sleepTime, retGroupCount);
							break;
						}

						ret = NMC_GroupSetParamI32(retDevID, GroupIndex, 0x36, 0, 2);
						ret = NMC_GroupSetParamI32(retDevID, GroupIndex, 0x36, 1, 3);
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x36, 2, 100);

						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x28, 0, 70); //基本速度
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x32, 0, 100); //最大速度
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x33, 0, 100); //加速度
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x34, 0, 110);   //減速度
						point = { xx,pos_y1,155,angle_a,0,-180 };
						NMC_GroupLine(retDevID, GroupIndex, 63, &point, NULL);
						//NMC_GroupPtpCartAll(retDevID, GroupIndex, 63, &point);
						//ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);

						
						

						// z的高度待確認 

						thread th_modbus(thread_modbus, ctx);
						th_modbus.detach();
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x28, 0, 35); //基本速度
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x32, 0, 40); //最大速度
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x33, 0, 45); //加速度
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x34, 0, 75);   //減速度
						point = { xx,pos_y1+250,155,angle_a,0,-180 };
						NMC_GroupLine(retDevID, GroupIndex, 63, &point, NULL);
						//NMC_GroupPtpCartAll(retDevID, GroupIndex, 63, &point);
						//ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);

						ret = NMC_GroupSetParamI32(retDevID, GroupIndex, 0x36, 0, 2);
						ret = NMC_GroupSetParamI32(retDevID, GroupIndex, 0x36, 1, 3);
						ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x36, 2, 100);

						//  目標位置待確認 (xyz座標)
						//ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x32, 0, 100);
						//ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x28, 0, 80);
						//ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x33, 0, 50);
						point = { 5,463,350,-0,0,-180 }; //手臂轉向至目標位置 
						NMC_GroupPtpCartAll(retDevID, GroupIndex, 63, &point);
						ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
						//Sleep(1000);
						point = { 5,463,200,0,0,-180 }; //手臂下降 
						NMC_GroupPtpCartAll(retDevID, GroupIndex, 63, &point); //將卡式空間座標位置直線運動至目標位置
						ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
						Sleep(1000);
						modbus_write_register(ctx, 0, 400); //手臂放鬆排氣(400固定值)
						Sleep(3000);
						point = { 5,463,300,0,0,-180 }; //放取物件後抬升高度
						NMC_GroupPtpCartAll(retDevID, GroupIndex, 63, &point);
						ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);

						ret = NMC_GroupPtpAcsAll(retDevID, GroupIndex, 63, &standbypos);
						ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
					}
					else
					{
						closesocket(sock);
						WSACleanup();
						isstoped = false;
					}
				}
				else
				{
					std::cout << "Not get the receive ! " << endl;
				}
			}
			else
			{
				cerr << "Lost connect to server, Err #" << WSAGetLastError() << endl; // 程式是否繼續(yes就關機/其他鍵就回到原始位置)
			}
			std::cout << "stop? y/n" << endl;
			string ans;
			std::cin >> ans;
			if (ans == "y")
			{
				Output = "end";
				int sendResult = send(sock, Output.c_str(), Output.size() + 1, 0);
				if (sendResult != SOCKET_ERROR)
				{
					std::cout << "Send> " << Output << endl;
					// Wait for response
					ZeroMemory(buf, 256);
					int bytesReceived = recv(sock, buf, 256, 0);
					std::cout << "recv: " << bytesReceived << endl;
				}
				isstoped = false;
			}
			//time = stoi(ans);
			//if (GetAsyncKeyState(VK_UP)) {
			//	closesocket(sock);
			//	WSACleanup();
			//	cout << "End Keyin Event..." << endl;
			//	break;
			//}
		}
		ret = NMC_GroupSetParamI32(retDevID, GroupIndex, 0x30, 0, 0);
		if (ret != 0) {
			cout << "ERROR! NMC_GroupSetParamI32:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		ret = NMC_GroupPtpAcsAll(retDevID, GroupIndex, 63, &HomePos);
		if (ret != 0) {
			cout << "ERROR! NMC_GroupPtpAcsAll:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
		if (ret != 0)
		{
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		SetBreak(Break_On, retDevID, sleepTime, retGroupCount);
		Sleep(10);
		CloseDevice(retDevID, sleepTime, retGroupCount);
		modbus_close(ctx);
		//testclass.join();
	}
	return 0;
}



void CloseDevice(I32_T retDevID, U32_T sleepTime, I32_T retGroupCount) {

	//=================================================	
	//              Shutdown device
	//=================================================

	RTN_ERR ret = 0;
	I32_T retState = 0;


	ret = NMC_DeviceDisableAll(retDevID);
	if (ret != 0) {
		printf("ERROR! NMC_DeviceDisableAll: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
	}
	else {
		printf("\nDevice disable all succeed.\n");
	}


	//sleep
	Sleep(sleepTime);

	//check group state
	for (I32_T i = 0; i < retGroupCount; i++)
	{
		ret = NMC_GroupGetState(retDevID, 0, &retState);
		if (ret != 0)
		{
			printf("ERROR! NMC_GroupGetState(group index %d): (%d)%s.\n", i, ret, NMC_GetErrorDescription(ret, NULL, 0));
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}

		if (retState != NMC_GROUP_STATE_DISABLE)
		{
			printf("ERROR! Group disable failed.(group index %d)\n", i);
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		else
			printf("Group disable succeed.(group index %d)\n", i);
	}

	ret = NMC_DeviceShutdown(retDevID);
	if (ret != 0) {
		printf("ERROR! NMC_DeviceShutdown: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
	}
	else {
		printf("\nDevice shutdown succeed.\n");
	}
	system("pause");
	exit(0);
}

void OpenDevice(I32_T &devType, I32_T &devIndex, I32_T &retDevID, I32_T sleepTime, I32_T &retGroupCount, I32_T &retGroupAxisCount) {

	//=================================================
	//              Device open up
	//=================================================

	RTN_ERR ret = 0;
	I32_T retDevState = 0;
	I32_T retSingleAxisCount = 0;

	cout << "Start to openup device..." << endl;

	ret = NMC_DeviceOpenUp(devType, devIndex, &retDevID);
	if (ret != 0)
	{
		printf("ERROR! NMC_DeviceOpenUp: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}
	else {
		printf("\nDevice open up succeed, device ID: %d.\n", retDevID);
	}

	ret = NMC_DeviceResetStateAll(retDevID);
	if (ret != NMC_AXIS_STATE_DISABLE)
	{
		printf("ERROR! NMC_DeviceResetStateAll, device ID: %d. (err code: %d)\n", retDevID, ret);
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}
	else {
		printf("Device ID %d: Device Reset State All success.\n", retDevID);
	}

	//get device state
	ret = NMC_DeviceGetState(retDevID, &retDevState);
	if (retDevState != NMC_DEVICE_STATE_OPERATION)
	{
		printf("ERROR! Device open up failed, device ID: %d.\n", retDevID);
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}
	else {
		printf("Device ID %d: state is OPERATION.\n", retDevID);
	}



	//=================================================
	//              Step 2 : Get device infomation
	//=================================================
	//Get amount of single axis
	ret = NMC_DeviceGetAxisCount(retDevID, &retSingleAxisCount);
	if (ret != 0)
	{
		printf("ERROR! NMC_DeviceGetAxisCount: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}
	else
		printf("\nGet single axis count succeed, device has %d single axis.\n", retSingleAxisCount);

	//Get amount of GROUP
	ret = NMC_DeviceGetGroupCount(retDevID, &retGroupCount);
	if (ret != 0)
	{
		printf("ERROR! NMC_DeviceGetGroupCount: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}
	else
		printf("Get group count succeed, device has %d group.\n", retGroupCount);

	if (retGroupCount == 0)
	{
		printf("ERROR! The NCF has no group!");
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}

	//Get amount of AXIS of each GROUP
	for (I32_T i = 0; i < retGroupCount; i++)
	{
		ret = NMC_DeviceGetGroupAxisCount(retDevID, i, &retGroupAxisCount);
		if (ret != 0)
		{
			printf("ERROR! NMC_DeviceGetGroupAxisCount: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		else
			printf("Get group axis count succeed, group index %d has %d axis.\n", i, retGroupAxisCount);
	}

	printf("\nReady to reset all drives in device...\n");


}

int WaitGroupStandStill(I32_T DeviceID, I32_T GroupIndex) {
	RTN_ERR     ret = 0;
	I32_T       groupStatus = 0;
	U32_T       count = 0;
	U32_T       timeOut = (20);


	do
	{
		printf("** Waiting group enabled.\n");
		ret = NMC_GroupGetStatus(DeviceID, GroupIndex, &groupStatus);
		if (ret != 0)
		{
			printf("ERROR! NMC_GroupGetStatus: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
			break;
		}

		// break condition
		if (count > timeOut)
		{
			printf("** Timeout, stop waiting and continue...\n");
			return -1;
		}
		else if ((groupStatus & GROUP_STANDSTILL) != false)
		{
			printf("** The Group has been enabled and continues...\n");
			return 0;
		}


		Sleep(SLEEP_TIME);
		count++;
	} while (true);

	return 0;
}

int WaitCmdReached(I32_T DeviceID, I32_T GroupIndex, I32_T AxisCount) {
	RTN_ERR     ret = 0;
	I32_T       groupStatus = 0;
	U32_T       count = 0;
	Pos_T       groupAxisActPostion = { 0 };
	U32_T       timeOut = 10000;
	do
	{
		ret = NMC_GroupGetStatus(DeviceID, GroupIndex, &groupStatus);
		if (ret != 0)
		{
			printf("ERROR! NMC_GroupGetStatus: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
			break;
		}

		//0:MCS, 2:ACS
		NMC_GroupGetActualPos(DeviceID, GroupIndex, 0, &groupAxisActPostion);
		if (ret != 0)
			printf("ERROR! NMC_GroupGetActualPosAcs: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));

		// print actual position
		for (int idxAxisPos = 0; idxAxisPos < AxisCount; idxAxisPos++)
		{
			printf("A%d: %.3f, ", idxAxisPos + 1, groupAxisActPostion.pos[idxAxisPos]);
		}
		printf("\n");

		// break condition
		if (count > timeOut)
		{
			printf("** Timeout, stop waiting and continue...\n");
			return -1;
		}
		else if ((groupStatus & GROUP_TARGET_REACHED) != false)
		{
			printf("** The command has been reached and continues...\n");
			return 0;
		}

		Sleep(SLEEP_TIME * 4);
		count++;
	} while (true);

	return ret;
}

void SafyOn(I32_T retDevID, U32_T sleepTime, I32_T retGroupCount) {
	//=================================================
	//    Step 3 : (for Hiwin RA605) Trigger safety rely
	//=================================================

	RTN_ERR ret = 0;
	U32_T   firstIoSet = 1;
	I32_T   dioValue = 0;
	U32_T   sizeByteDIO = 1;


	//get current DO value
	ret = NMC_ReadOutputMemory(retDevID, firstIoSet, sizeByteDIO, &dioValue);
	if (ret != 0)
	{
		printf("ERROR! NMC_ReadOutputMemory: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}


	//output DO value include bit of trigger safety rely
	dioValue = dioValue | DIO_START_RESET;
	ret = NMC_WriteOutputMemory(retDevID, firstIoSet, sizeByteDIO, &dioValue);
	if (ret != 0)
	{
		printf("ERROR! NMC_WriteOutputMemory: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}

}

void ResetAlarm(I32_T retDevID, U32_T sleepTime, I32_T retGroupCount) {
	//=================================================
	//  Step 4 : Clean alarm of drives of each group
	//=================================================

	RTN_ERR ret = 0;
	I32_T retState = 0;


	for (I32_T i = 0; i < retGroupCount; i++)
	{
		ret = NMC_GroupResetDriveAlmAll(retDevID, i);
		if (ret != 0)
		{
			printf("ERROR! NMC_GroupResetDriveAlmAll: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
	}

	//sleep
	Sleep(sleepTime);

	//check state
	for (I32_T i = 0; i < retGroupCount; i++)
	{
		ret = NMC_GroupGetState(retDevID, i, &retState);
		if (ret != 0)
		{
			printf("ERROR! NMC_GroupGetState(group index %d): (%d)%s.\n", i, ret, NMC_GetErrorDescription(ret, NULL, 0));
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}

		if (retState != NMC_GROUP_STATE_DISABLE)
		{
			printf("ERROR! Group reset failed.(group index %d)\n", i);
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		else {
			printf("Group reset succeed.(group index %d)\n", i);
		}
	}


	//=================================================
	//       Step 5 : Enable all groups
	//=================================================
	for (I32_T i = 0; i < retGroupCount; i++)
	{
		ret = NMC_DeviceEnableAll(retDevID);
		if (ret != 0)
		{
			printf("ERROR! NMC_DeviceEnableAll: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		else {
			printf("\nReady to enable all single axes and groups...\n");
		}
		ret = WaitGroupStandStill(retDevID, i);
		if (ret != 0)
		{
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
	}

	//check group state
	for (I32_T i = 0; i < retGroupCount; i++)
	{
		ret = NMC_GroupGetState(retDevID, i, &retState);
		if (ret != 0)
		{
			printf("ERROR! NMC_GroupGetState(group index %d): (%d)%s.\n", i, ret, NMC_GetErrorDescription(ret, NULL, 0));
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}

		if (retState != NMC_GROUP_STATE_STAND_STILL)
		{
			printf("ERROR! Group enable failed.(group index %d) (err code : %d) \n", i, retState);
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		else {
			printf("Group enable succeed.(group index %d)\n", i);
		}
	}

	//sleep
	Sleep(sleepTime);
}

void GoHome(I32_T retDevID, U32_T sleepTime, I32_T retGroupCount, I32_T GroupIndex, I32_T retGroupAxisCount) {

	RTN_ERR ret = 0;

	I32_T groupAxesIdxMask = 0;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_X;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_Y;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_Z;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_A;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_B;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_C;

	Sleep(sleepTime);


	ret = NMC_GroupAxesHomeDrive(retDevID, GroupIndex, groupAxesIdxMask);
	if (ret != 0) {
		cout << "ERROR! NMC_GroupAxesHomeDrive:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}
	else {
		cout << endl;
		cout << "Go Home Success!!!" << endl;
	}

	ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
	if (ret != 0)
	{
		printf("ERROR! Wait Command Reached: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}
}

void GetPoseValue(I32_T retDevID, I32_T GroupIndex, I32_T retGroupCount, U32_T sleepTime) {
	//=================================================
	//       Step 6 : Get position information
	//=================================================

	RTN_ERR ret = 0;
	Pos_T cmdPosPcs = { 0 };

	//Group Get Command PosPcs
	ret = NMC_GroupGetActualPosAcs(retDevID, GroupIndex, &cmdPosPcs);
	if (ret != 0)
	{
		printf("ERROR! NMC_GroupGetActualPosAcs: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}
	else
	{
		printf("\nGroup NMC_GroupGetActualPosAcs succeed.(group index %d)\n", GroupIndex);
		printf("ActualPosAcs[0] = %f \n", cmdPosPcs.pos[0]);
		printf("ActualPosAcs[1] = %f \n", cmdPosPcs.pos[1]);
		printf("ActualPosAcs[2] = %f \n", cmdPosPcs.pos[2]);
		printf("ActualPosAcs[3] = %f \n", cmdPosPcs.pos[3]);
		printf("ActualPosAcs[4] = %f \n", cmdPosPcs.pos[4]);
		printf("ActualPosAcs[5] = %f \n", cmdPosPcs.pos[5]);
	}

	//Group Get Command PosPcs
	ret = NMC_GroupGetCommandPosAcs(retDevID, GroupIndex, &cmdPosPcs);
	if (ret != 0)
	{
		printf("ERROR! NMC_GroupGetCommandPosAcs: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}
	else
	{
		printf("\nGroup NMC_GroupGetCommandPosAcs succeed.(group index %d)\n", GroupIndex);
		printf("CommandPosAcs[0] = %f \n", cmdPosPcs.pos[0]);
		printf("CommandPosAcs[1] = %f \n", cmdPosPcs.pos[1]);
		printf("CommandPosAcs[2] = %f \n", cmdPosPcs.pos[2]);
		printf("CommandPosAcs[3] = %f \n", cmdPosPcs.pos[3]);
		printf("CommandPosAcs[4] = %f \n", cmdPosPcs.pos[4]);
		printf("CommandPosAcs[5] = %f \n", cmdPosPcs.pos[5]);
	}

	//sleep
	Sleep(sleepTime);

}

void SetBreak(BreakFlag inStaute, I32_T retDevID, U32_T sleepTime, I32_T retGroupCount) {
	//=================================================
	//    Set Brake
	//=================================================

	RTN_ERR ret = 0;
	U32_T   firstIoSet = 0;
	I32_T   dioValue = 0;
	U32_T   sizeByteDIO = 1;

	//get current DO value
	ret = NMC_ReadOutputMemory(retDevID, firstIoSet, sizeByteDIO, &dioValue);
	if (ret != 0)
	{
		printf("ERROR! NMC_ReadOutputMemory: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}

	//output DO value include bit of unbrake
	if (inStaute == Break_On) {
		dioValue = dioValue & ~DIO_UNBRAKE;
	}
	else if (inStaute == Break_Off) {
		dioValue = dioValue | DIO_UNBRAKE;
	}
	ret = NMC_WriteOutputMemory(retDevID, firstIoSet, sizeByteDIO, &dioValue);
	if (ret != 0)
	{
		printf("ERROR! NMC_WriteOutputMemory: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}

	Sleep(sleepTime);
}

void SetHome(Pos_T HomePos, I32_T retDevID, U32_T sleepTime, I32_T retGroupCount, I32_T GroupIndex, I32_T retGroupAxisCount) {

	RTN_ERR ret = 0;

	I32_T groupAxesIdxMask = 0;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_X;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_Y;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_Z;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_A;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_B;
	groupAxesIdxMask += NMC_GROUP_AXIS_MASK_C;

	Sleep(sleepTime);

	ret = NMC_GroupSetHomePos(retDevID, GroupIndex, groupAxesIdxMask, &HomePos);
	if (ret != 0) {
		cout << "ERROR! NMC_GroupSetHomePos:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
		CloseDevice(retDevID, sleepTime, retGroupCount);
	}
	else {
		cout << endl;
		cout << "Set Home!!!" << endl;
		cout << "A1:" << HomePos.pos[0] << endl;
		cout << "A2:" << HomePos.pos[1] << endl;
		cout << "A3:" << HomePos.pos[2] << endl;
		cout << "A4:" << HomePos.pos[3] << endl;
		cout << "A5:" << HomePos.pos[4] << endl;
		cout << "A6:" << HomePos.pos[5] << endl;
		cout << "set Home complete..." << endl;
	}
}

//void Set15DO(IO15 status, I32_T retDevID, U32_T sleepTime, I32_T retGroupCount, I32_T *do15value) {
//
//	RTN_ERR ret = 0;
//	U32_T   firstIoSet = 0;
//	I32_T   valueByte0 = 0;
//	U32_T   sizeByteDIO = 1;
//
//
//	ret = NMC_ReadOutputMemory(retDevID, 1, 1, &valueByte0);
//	if (ret != 0)
//	{
//		printf("ERROR! NMC_ReadOutputMemory: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
//		CloseDevice(retDevID, sleepTime, retGroupCount);
//	}
//
//	//DO_On or DO_Off
//	if (status == IO_Off) {
//		valueByte0 = valueByte0 & ~(1 << 7);
//		Mutex.lock();
//		*do15value = (valueByte0 >> 7) & 1;//do15value=0
//		Mutex.unlock();
//	}
//	if (status == IO_On) {
//		valueByte0 = valueByte0 | (1 << 7);
//		Mutex.lock();
//		*do15value = (valueByte0 >> 7) | 1;////do15value=1
//		Mutex.unlock();
//	}
//	ret = NMC_WriteOutputMemory(retDevID, firstIoSet, sizeByteDIO, &valueByte0);
//	if (ret != 0)
//	{
//		printf("ERROR! NMC_WriteOutputMemory: (%d)%s.\n", ret, NMC_GetErrorDescription(ret, NULL, 0));
//		CloseDevice(retDevID, sleepTime, retGroupCount);
//	}
//
//	cout << "do15value = " << *do15value << endl;
//
//	Sleep(sleepTime);
//}

void detectface(string path, vector<vector<float> >& LIST) {
	float  val;
	string line;

	ifstream myfile(path);

	//vector<vector<float> > LIST;

	if (myfile.is_open())
	{
		while (getline(myfile, line))
		{
			vector<float> v;

			istringstream IS(line);

			while (IS >> val)
			{
				v.push_back(val);
			}

			LIST.push_back(v);
		}

		myfile.close();
	}

	for (int i = 0; i < LIST.size(); i++)
	{
		for (int j = 0; j < LIST[i].size(); j++)
		{
			cout << LIST[i][j] << " ";
		}
		cout << endl;
	}
}

void TrackMove(I32_T retDevID, I32_T GroupIndex, U32_T sleepTime, I32_T retGroupCount, I32_T retGroupAxisCount, float *c_speed, float *dis, I32_T *do15value) {
	//軌跡移動
	RTN_ERR ret = 0;
	vector<vector<float> > LIST;
	detectface("C:/Users/Robot/Desktop/point_test.txt", LIST);

	int d_facenum = LIST[0][0];



	if (d_facenum == 2) {
		int point_num = d_facenum * 2;
		int path_num = point_num - 1;
		float path_dis = 800 / path_num;
		float path_time = path_dis / *c_speed;


		//relative move 
		float x = LIST[2][0] - LIST[1][0];
		float y = LIST[2][1] - LIST[1][1];
		float z = LIST[2][2] - LIST[1][2];

		Pos_T point_1 = { LIST[1][0],400,LIST[1][2],LIST[1][3],LIST[1][4],LIST[1][5] };
		Pos_T dis1 = { 0,-path_dis,0,0,0,0 };
		Pos_T dis2 = { x,-(path_dis - y),z,0,0,0 };
		float dis3 = sqrt(pow(dis2.pos[0], 2) + pow(dis2.pos[1], 2) + pow(dis2.pos[2], 2));
		float dis4 = sqrt(pow(point_1.pos[0], 2) + pow(point_1.pos[1], 2) + pow(point_1.pos[2], 2));
		float speed1 = dis4 / (200 / *c_speed);
		float speed2 = dis3 / path_time;

		ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x32, 0, speed1);
		for (int j = 0; j < 6; j++) {
			ret = NMC_GroupAxSetParamF64(retDevID, GroupIndex, j, 0x32, 0, speed1);
		}

		ret = NMC_GroupPtpCartAll(retDevID, GroupIndex, 63, &point_1);//p1
		if (ret != 0) {
			cout << "ERROR! NMC_GroupPtpCartAll:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
		if (ret != 0)
		{
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}

		//move by abs(0) or relative(1)
		ret = NMC_GroupSetParamI32(retDevID, GroupIndex, 0x30, 0, 1);

		ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x32, 0, *c_speed);
		for (int j = 0; j < 6; j++) {
			ret = NMC_GroupAxSetParamF64(retDevID, GroupIndex, j, 0x32, 0, *c_speed);
		}

		ret = NMC_GroupLine(retDevID, GroupIndex, 63, &dis1, NULL);//p2
		if (ret != 0) {
			cout << "ERROR! NMC_GroupLine:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
		if (ret != 0)
		{
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}

		ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x32, 0, speed2);
		for (int j = 0; j < 6; j++) {
			ret = NMC_GroupAxSetParamF64(retDevID, GroupIndex, j, 0x32, 0, speed2);
		}

		ret = NMC_GroupLine(retDevID, GroupIndex, 63, &dis2, NULL);//p3
		if (ret != 0) {
			cout << "ERROR! NMC_GroupLine:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
		if (ret != 0)
		{
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}

		ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x32, 0, *c_speed);
		for (int j = 0; j < 6; j++) {
			ret = NMC_GroupAxSetParamF64(retDevID, GroupIndex, j, 0x32, 0, *c_speed);
		}

		ret = NMC_GroupLine(retDevID, GroupIndex, 63, &dis1, NULL);//p4
		if (ret != 0) {
			cout << "ERROR! NMC_GroupLine:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
		if (ret != 0)
		{
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}

		//move by abs(0) or relative(1)
		ret = NMC_GroupSetParamI32(retDevID, GroupIndex, 0x30, 0, 0);

		//ret = NMC_GroupSetParamI32(retDevID, GroupIndex, 0x36, 0, 0);

		Pos_T HomePos = { 0,90,0,0,0,0 };
		ret = NMC_GroupPtpAcsAll(retDevID, GroupIndex, 63, &HomePos);
		if (ret != 0) {
			cout << "ERROR! NMC_GroupPtpAcsAll:" << NMC_GetErrorDescription(ret, NULL, 0) << endl;
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		ret = WaitCmdReached(retDevID, GroupIndex, retGroupAxisCount);
		if (ret != 0)
		{
			CloseDevice(retDevID, sleepTime, retGroupCount);
		}
		//Set15DO(IO_Off, retDevID, sleepTime, retGroupCount, do15value);
	}



}

void adaptspeed(float *c_speed, I32_T retDevID, U32_T sleepTime, I32_T retGroupCount) {
	//適應速度
	RTN_ERR ret;
	I32_T   GroupIndex = 0;

	ret = NMC_GroupSetParamF64(retDevID, GroupIndex, 0x32, 0, *c_speed);//this parameter is for line move
																		//for (int j = 0; j < 6; j++) { //this parameter is for P2P
																		//	ret = NMC_GroupAxSetParamF64(retDevID, GroupIndex, j, 0x32, 0, robotspeed);
																		//}
	cout << "set cart_speed to " << *c_speed << endl;
}

