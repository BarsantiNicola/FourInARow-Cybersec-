#include"Game.h"
#include<string>
#include<thread>
#include<termios.h>
#include<stdlib.h>
#include<unistd.h>
#include"../cipher/CipherClient.h"
#include"../utility/Message.h"
#include"../utility/NetMessage.h"
#include"../utility/Converter.h"
//#include"ChallengeInformation.h"
#include"ChallengeRegister.h"
#include"TextualInterfaceManager.h"
#include"../utility/ConnectionManager.h"
#include<time.h>
#include<mutex>
#include<exception>
#include<vector>
#include<sstream>
#include<csignal>
#include<regex>
#define SLEEP_TIME 2
#define TOKEN_GAP 50
#define SAFE_ZONE 150
#define NUMBER_SEPARATOR 4
#define MAX_LENGTH_CHAT 200
using namespace utility;
/*
extern std::mutex mtx_time_expired;
extern std::mutex mtx_comand_to_timer;
extern ComandToTimer comandTimer = ComandToTimer::STOP;
extern bool time_expired=false;*/
namespace client
{
  enum MessageGameType{
        NO_GAME_TYPE_MESSAGE,
        MOVE_TYPE,
        CHAT_TYPE
    };


  enum ComandToTimer
  {
    STOP,
    START,
    RESUME,
    TERMINATE
  };
  enum ClientPhase
  {
    MAIN_INTERFACE_PHASE,
    LOGIN_PHASE,
    LOGOUT_PHASE,
    REJECT_PHASE,
    ACCEPT_PHASE,
    INGAME_PHASE,
    USER_LIST_PHASE,
    RANK_LIST_PHASE,
    MATCH_PHASE,
    NO_PHASE,
    START_GAME_PHASE
  };  

  class MainClient
  {
    private:

      bool keyExchangeMade=false;
      Message* messageToACK=nullptr;
      unsigned int nonceVerifyAdversary=0;
      unsigned int myNonceVerify=0;
      unsigned int serverNonceVerify=0;
      bool SendNonceOutOfBound=false;
      bool ReceiveNonceOutOfBound=false;
      std::thread timerThread;
      string reqStatus="none";
      bool partialKeyCreated=false;
      NetMessage* partialKey;
      int nonceAdv=0;
      vector<string> chatWait;
      Message* messageChatToACK=nullptr;
      bool startingMatch=false;
      bool firstMove=false;
      bool notConnected=true;
      bool startChallenge=false;
      bool implicitUserListReq=false;
      string textualMessageToUser="";
      //bool waitForChatACK=false;
      int currTokenChatAdv;
      int nUser=0;
      time_t startWaitChatAck;
      time_t startWaitAck;
      unsigned int sendNonce;
      unsigned int receiveNonce;
      bool logged=false;
      ClientPhase clientPhase= ClientPhase::NO_PHASE;
      bool currTokenIninzialized=false;
      unsigned int currentToken;//da inizializzare nel main
      unsigned int currTokenChat;//da inizializzare nel main
      int* advPort=nullptr;
      const char* serverIP="127.0.0.1";
      int serverPort=12345;
      int numberToTraslate=0;
      int myPort=1235;
      const char* myIP="127.0.0.1";
      bool sendImplicitUserListReq();
      double sendStart;
      string username = "";
      string adv_username_1 = "";
      char* advIP;
      string challenged_username = "";
      Game* game=nullptr;
      cipher::SessionKey* aesKeyServer=nullptr;
      cipher::SessionKey* aesKeyClient=nullptr;
      ChallengeRegister* challenge_register=new ChallengeRegister();
      ConnectionManager* connection_manager=nullptr;//da inizializzare nel main
      TextualInterfaceManager* textual_interface_manager=nullptr;
      cipher::CipherClient* cipher_client;
      bool receiveACKProtocol(Message* message);
      bool loginProtocol(std::string username,bool *socketIsClosed);//ok
      string printableString(unsigned char* toConvert,int len);//ok
      bool sendChallengeProtocol(const char* adversaryUsername,int size);//ok
      bool receiveChallengeProtocol(Message* message);//ok
      bool sendAcceptProtocol(const char* usernameAdv,int size);//ok
      bool receiveAcceptProtocol(Message* message);
      bool sendRejectProtocol(const char* usernameAdv,int size);//ok
      bool receiveRejectProtocol(Message* message);//ok
      bool errorHandler(Message* message);//ok
      bool sendRankProtocol();//ok
      bool receiveRankProtocol(Message* message);//ok
      bool certificateProtocol();//ok
      bool keyExchangeReciveProtocol(Message* message,bool exchangeWithServer);//ok
      bool keyExchangeClientSend();
      bool MakeAndSendGameMove(int column);
      void ReciveGameMove(Message* message);
      bool sendReqUserListProtocol();//ok
      bool sendChatProtocol(string chat);
      bool reciveChatProtocol(Message* message);
      bool receiveUserListProtocol(Message* message);//ok
      bool reciveDisconnectProtocol(Message* message);
      bool sendDisconnectProtocol();
      bool sendWithDrawProtocol();
      bool receiveWithDraw(Message* message);
      bool receiveWithDrawOkProtocol(Message* message);
      bool sendLogoutProtocol();//ok
      bool receiveLogoutProtocol(Message* message);//ok
      bool receiveGameParamProtocol(Message* message);
      int generateRandomNonce();
      void clearGameParam();
      bool timeIsExpired();
      void resetTimeExpired();
      void setcomandTimer(ComandToTimer comand );
      ComandToTimer getcomandTimer();
      bool comand(std::string comand_line);
      bool startConnectionServer(const char* myIP,int myPort);
      int countOccurences(string source,string searchFor);
      Message* createMessage(MessageType type, const char* param,unsigned char* g_param,int g_paramLen,cipher::SessionKey* aesKey,int token,bool keyExchWithClient);
      unsigned char* concTwoField(unsigned char* firstField,unsigned int firstFieldSize,unsigned char* secondField,unsigned int secondFieldSize,unsigned char separator,unsigned int numberSeparator);
      bool receiveACKChatProtocol(Message* message);
      bool deconcatenateTwoField(unsigned char* originalField,unsigned int originalFieldSize,unsigned char* firsField,unsigned int* firstFieldSize,unsigned char* secondField,unsigned int* secondFieldSize,unsigned char separator,unsigned int numberSeparator);//ok
      bool getDeconcatenateLength(unsigned char* originalField,unsigned int originalFieldSize,unsigned int* firstFieldSize,unsigned int* secondFieldSize,unsigned char separator,unsigned int numberSeparator);
       void printWhiteSpace();
    public:
      MainClient(const char* ipAddr , int port ); 
      ~MainClient();
      void client();//ok
  };
}
