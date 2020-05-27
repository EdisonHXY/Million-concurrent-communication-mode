#ifndef _MessageHeader_hpp_
#define _MessageHeader_hpp_

//��Ϣ������
enum CMD
{
	CMD_LOGIN,         //��¼
	CMD_LOGIN_RESULT,  //��¼���
	CMD_LOGOUT,        //�˳�
	CMD_LOGOUT_RESULT, //�˳����
	CMD_NEW_USER_JOIN, //�µĿͻ��˼���
	CMD_ERROR          //����
};

//���ݱ��ĵ�ͷ��
struct DataHeader
{
	DataHeader() :cmd(CMD_ERROR), dataLength(sizeof(DataHeader)) {}
	short cmd;        //���������
	short dataLength; //���ݵĳ���
};

//��¼��Ϣ��
struct Login :public DataHeader
{
	Login() {
		cmd = CMD_LOGIN;
		dataLength = sizeof(Login); //��Ϣ����=��Ϣͷ(����)+��Ϣ��(����)
	}
	char userName[32]; //�˺�
	char PassWord[32]; //����
	char data[32];
};

//��¼���
struct LoginResult :public DataHeader
{
	LoginResult() :result(0) {
		cmd = CMD_LOGIN_RESULT;
		dataLength = sizeof(LoginResult);
	}
	int result; //��¼�Ľ����0��������
	char data[92];
};

//�˳���Ϣ��
struct Logout :public DataHeader
{
	Logout() {
		cmd = CMD_LOGOUT;
		dataLength = sizeof(Logout);
	}
	char userName[32]; //�˺�
};

//�˳����
struct LogoutResult :public DataHeader
{
	LogoutResult() :result(0) {
		cmd = CMD_LOGOUT_RESULT;
		dataLength = sizeof(LogoutResult);
	}
	int result; //�˳��Ľ����0��������
};

//�µĿͻ��˼��룬����˸��������пͻ��˷��ʹ˱���
struct NewUserJoin :public DataHeader
{
	NewUserJoin(int _cSocket = 0) :sock(_cSocket) {
		cmd = CMD_NEW_USER_JOIN;
		dataLength = sizeof(LogoutResult);
	}
	int sock; //�¿ͻ��˵�socket
};

#endif