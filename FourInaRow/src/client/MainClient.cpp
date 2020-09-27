#include"MainClient.h"
namespace client
{
  void MainClient::timerHandler(long secs)
  {
    while(true)
    {
      lck_time->lock();
      if(timer!=0)
      {
        lck_time->unlock();
        sleep(SLEEP_TIME);
        lck_time->lock();
        --timer;
        lck_time->unlock();
      }
      else
      {
        time_expired=true;
        lck_time->unlock();
      }
    }
   
  }
/*
--------------certificateProtocol function-----------------------------
*/
  bool MainClient::certificateProtocol()
  {
  
   const char*ip=nullptr;
   bool socketIsClosed=false;
   bool res;
   bool DecodRes;
   Message* messRet;
   NetMessage* netRet;
   Converter conv;
   Message* mess =createMessage(MessageType::CERTIFICATE_REQ, nullptr,nullptr,0,nullptr,this->nonce,false);

   if(mess==nullptr)
   {
     verbose<<"-->[MainClient][certificateProtocol] error to create a CERTIFICATION message "<<'\n';
     return false;
   }
   res=connection_manager->sendMessage(*mess,connection_manager->getserverSocket(),&socketIsClosed,ip,0);
 
   try
   {
     messRet=connection_manager->getMessage(connection_manager->getserverSocket());
   }
   catch(exception e)
   {
     notConnected=true;
     return false;
   }
   if(messRet==nullptr)
   {
     verbose<<"-->[MainClient][certificateProtocol] error to recive a CERTIFICATION message "<<'\n';
     return false;
   }
   if(messRet->getMessageType()!=CERTIFICATE)
   {
     verbose<<"-->[MainClient][certificateProtocol] error to recive a CERTIFICATION message "<<'\n';
     return false; 
   }
   vverbose<<"-->[MainClient][certificateProtocol] message recived!!"<<'\n';
   DecodRes=cipher_client->fromSecureForm( messRet , this->username ,nullptr ,true);//da controllare
   
   if(!res||!DecodRes)
   {
     verbose<<"-->[MainClient][certificateProtocol] error certificate protocol!!"<<'\n';
     if(socketIsClosed)
       notConnected=true;
     return false;
   }
    vverbose<<"-->[MainClient][certificateProtocol] message decifred!!"<<'\n';
   netRet=conv.encodeMessage(messRet-> getMessageType(), *messRet );
   if(netRet==nullptr)
   {
     return false;
   }
   
   this->nonce = *(messRet->getNonce())+1;
   vverbose<<"-->[MainClient][certificateProtocol] the vulue of nonce is "<<this->nonce<<'\n';
   //res = cipher_client-> getSessionKey( netRet->getMessage() ,netRet->length() );
   //vverbose<<"-->[MainClient][certificateProtocol] sessionKey obtained"<<'\n';
   return res;
  }
/*
--------------------------------loginProtocol function------------------------------
*/
  bool MainClient::loginProtocol(std::string username,bool *socketIsClosed)
  {
    bool res;
    int* nonce_s;
    bool connection_res;
    Message* retMess;
    Message* sendMess;
    if(socketIsClosed==nullptr)
      return false;
    if(username.empty())
    {
      return false;
    }
    char* cUsername=new char[username.size()+1];
    if(cUsername==nullptr)
      return false;
    std::strcpy(cUsername,username.c_str());
    Message* message=createMessage(MessageType::LOGIN_REQ, (const char*)cUsername,nullptr,0,nullptr,this->nonce,false);
    if(message==nullptr)
      return false;
    if(message->getMessageType()==LOGIN_REQ)
    {
      res=connection_manager->sendMessage(*message,connection_manager->getserverSocket(),socketIsClosed,nullptr,0); 
      if(!res)
      {
        if(socketIsClosed)
          notConnected=true;
        return false;
      }
      try
      {
        retMess=connection_manager->getMessage(connection_manager->getserverSocket());
        verbose<<"-->[MainClient][loginProtocol] a message recived"<<'\n';
      }
      catch(exception e)
      {
        notConnected=true;
        return false;
      }
      if(retMess==nullptr)
      {
        verbose<<"-->[MainClient][loginProtocol] message nullptr"<<'\n';
        return false;
      }
      nonce_s=message->getNonce();
      if(*nonce_s!=(this->nonce-1))
      {
        delete nonce_s;
        verbose<<"-->[MainClient][loginProtocol] nonce not equal"<<'\n';
        return false;
      }
      delete nonce_s;
      res=cipher_client->fromSecureForm( retMess , username ,nullptr,true);
      if(res==false)
      {
        verbose<<"-->[MainClient][loginProtocol] error to dec"<<'\n';
        return false;
      }
    
      if(retMess->getMessageType()==LOGIN_OK)
      {
        sendMess=createMessage(MessageType::KEY_EXCHANGE, nullptr,nullptr,0,nullptr,this->nonce,false);
        if(sendMess==nullptr)
          return false;
        verbose<<"-->[MainClient][loginProtocol] start key exchange protocol"<<'\n';
        res=connection_manager->sendMessage(*sendMess,connection_manager->getserverSocket(),socketIsClosed,nullptr,0); 
        if(!res)
        {
          if(*socketIsClosed)
            notConnected=true;
          return false;
        }
        try
        {
          retMess=connection_manager->getMessage(connection_manager->getserverSocket());
        }
        catch(exception e)
        {
          notConnected=true;
          return false;
        }
        if(retMess==nullptr)
          return false;
        res=keyExchangeReciveProtocol(retMess,true);
        verbose<<"-->[MainClient][loginProtocol] loginProtocol finished "<<'\n';
        return res;
      }
      else if(retMess->getMessageType()==LOGIN_FAIL)
      {  
        verbose<<"-->[MainClient][loginProtocol] loginFail"<<'\n';   
        return false;
      }
      
    }
      return false; 
  }
/*
--------------------sendReqUserListProtocol function----------------------------
*/

  bool MainClient::sendReqUserListProtocol()
  {
    bool res;
    bool socketIsClosed=false;
    Message* message;
    message=createMessage(MessageType::USER_LIST_REQ, nullptr,nullptr,0,aesKeyServer,this->nonce,false);
    if(message==nullptr)
      return false;
    res=connection_manager->sendMessage(*message,connection_manager->getserverSocket(),&socketIsClosed,nullptr,0);
    if(socketIsClosed)
    {
      vverbose<<"-->[MainClient][sendReqUserProtocol] error server is offline reconnecting"<<'\n';
      notConnected=true;
      return false;
    }
    if(res)
      clientPhase=ClientPhase::USER_LIST_PHASE;
    return res;
  
  }
/*
-------------------receiveUserListProtocol function--------------------------------
*/
    bool MainClient::receiveUserListProtocol(Message* message)
  {
      int* nonce_s;
      bool res;
      string search="username";
      if(message==nullptr)
      {
        verbose<<"--> [MainClient][reciveUserListProtocol] error the message is null"<<'\n';
        return false;
      }
      nonce_s=message->getNonce();
      if(*nonce_s!=(this->nonce-1))
      {
        verbose<<"--> [MainClient][reciveUserListProtocol] nonce not valid"<<'\n';
        delete nonce_s;
        //clientPhase=ClientPhase::NO_PHASE;
        return false;
      }
      if(message->getMessageType()!=USER_LIST || clientPhase!=ClientPhase::USER_LIST_PHASE)
      {
        if(clientPhase==ClientPhase::USER_LIST_PHASE)
          verbose<<"--> [MainClient][reciveUserListProtocol] message type: "<<message->getMessageType() << " not expected"<<'\n';
        else
          verbose<<"--> [MainClient][reciveUserListProtocol] wrong phase:"<<clientPhase<<'\n';
        return false;
      }
      else
      {
        string app;
        res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
        
        if(!res)
          return false; 
        clientPhase=ClientPhase::NO_PHASE;
        unsigned char* userList=message->getUserList();
        int userListLen=message->getUserListLen();
        app=printableString(userList,userListLen);
        int nUser= countOccurences(app,search);
        stringstream sstr;
        sstr<<nUser;
        textual_interface_manager->printMainInterface(this->username,sstr.str(),"online","none","0");
        if(!implicitUserListReq)
          std::cout<<app<<endl;
        implicitUserListReq=false;
        std::cout<<"\t# Insert a command:";
        cout.flush();
        return true;
      }
  }
  bool MainClient::sendImplicitUserListReq()
  {
    bool res;
    implicitUserListReq=true;
    res=sendReqUserListProtocol();
    return res;
  }

/*
-----------------reciveDiconnectProtocol-----------------
*/
  bool MainClient::reciveDisconnectProtocol(Message* message)
  {
      int* nonce_s;
      bool res;
      if(message==nullptr)
      {
        verbose<<"--> [MainClient][reciveLogoutProtocol] error the message is null"<<'\n';
        return false;
      }
      nonce_s=message->getNonce();
      if(*nonce_s!=(this->nonce-1))
      {
        verbose<<"--> [MainClient][reciveLogoutProtocol] error the nonce isn't valid"<<'\n';
        delete nonce_s;
        //clientPhase=ClientPhase::NO_PHASE;
        return false;
      }
      if(message->getMessageType()!=DISCONNECT)
      {
        verbose<<"--> [MainClient][reciveLogoutProtocol] message type not expected"<<'\n';
        return false;
      }

      //verbose<<"--> [MainClient][reciveLogoutProtocol] decript start"<<'\n';
      res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
      if(!res)
        return false; 
      clientPhase=ClientPhase::NO_PHASE;
      clearGameParam();
      sendImplicitUserListReq();
      return true;
  }
/*
--------------sendDisconnectProtocol---------------------
*/
  bool MainClient::sendDisconnectProtocol()
  {
    bool res;
    bool socketIsClosed=false;
    Message* message;
    message=createMessage(MessageType::DISCONNECT, nullptr,nullptr,0,aesKeyServer,this->nonce,false);
    if(message==nullptr)
      return false;
    res=connection_manager->sendMessage(*message,connection_manager->getserverSocket(),&socketIsClosed,nullptr,0);
    if(socketIsClosed)
    {
      vverbose<<"-->[MainClient][sendDisconnectProtocol] error server is offline reconnecting"<<'\n';
      clientPhase=ClientPhase::NO_PHASE;
      notConnected=true;
      return false;
    }
    if(res)
    {
      clientPhase=ClientPhase::NO_PHASE;
      sendImplicitUserListReq();
    }
     
    return res;
  }
/*
--------------------sendRankProtocol function----------------------------
*/

  bool MainClient::sendRankProtocol()
  {
    bool res;
    bool socketIsClosed=false;
    Message* message;
    message=createMessage(MessageType::RANK_LIST_REQ, nullptr,nullptr,0,aesKeyServer,this->nonce,false);
    if(message==nullptr)
      return false;
    res=connection_manager->sendMessage(*message,connection_manager->getserverSocket(),&socketIsClosed,nullptr,0);
    if(socketIsClosed)
    {
      vverbose<<"-->[MainClient][sendReqUserProtocol] error server is offline reconnecting"<<'\n';
      notConnected=true;
      return false;
    }
    if(res)
      clientPhase=ClientPhase::RANK_LIST_PHASE;
    return res;
  
  }
/*
-------------------receiveRankProtocol function--------------------------------
*/
    bool MainClient::receiveRankProtocol(Message* message)
  {
      int* nonce_s;
      bool res;
      if(message==nullptr)
      {
        verbose<<"--> [MainClient][reciveRankProtocol] error the message is null"<<'\n';
        return false;
      }
      nonce_s=message->getNonce();
      if(*nonce_s!=(this->nonce-1))
      {
        verbose<<"--> [MainClient][receiveRankProtocol] nonce not valid"<<'\n';
        delete nonce_s;
        //clientPhase=ClientPhase::NO_PHASE;
        return false;
      }
      if(message->getMessageType()!=RANK_LIST || clientPhase!=ClientPhase::RANK_LIST_PHASE)
      {
        verbose<<"--> [MainClient][receiveRankProtocol] message type not expected"<<'\n';
        return false;
      }
      else
      {
        string app;
        res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
        
        if(!res)
          return false;
        clientPhase=ClientPhase::NO_PHASE;
        unsigned char* userList=message->getRankList();
        int userListLen=message->getRankListLen();
        app=printableString(userList,userListLen);
        std::cout<<app<<endl;
        std::cout<<"\t# Insert a command:";
        cout.flush();
        return true;
      }
  }

/*
--------------------sendLogoutProtocol---------------------------------------------
*/
  bool MainClient::sendLogoutProtocol()
  {
    bool res;
    bool socketIsClosed=false;
    Message* message;
    message=createMessage(MessageType::LOGOUT_REQ, nullptr,nullptr,0,aesKeyServer,this->nonce,false);
    if(message==nullptr)
      return false;
    res=connection_manager->sendMessage(*message,connection_manager->getserverSocket(),&socketIsClosed,nullptr,0);
    if(socketIsClosed)
    {
      verbose<<"-->[MainClient][sendLogoutProtocol] error server is offline reconnecting"<<'\n';
      notConnected=true;
      return false;
    }
    if(res)
    {
      verbose<<"-->[MainClient][sendLogoutProtocol] LOGOUT_PHASE"<<'\n';
      clientPhase=ClientPhase::LOGOUT_PHASE;
    }
    return res;
  }
/*
---------------------receiveLogoutProtocol----------------------------------
*/
  bool MainClient::receiveLogoutProtocol(Message* message)
  {
      int* nonce_s;
      bool res;
      if(message==nullptr)
      {
        verbose<<"--> [MainClient][reciveLogoutProtocol] error the message is null"<<'\n';
        return false;
      }
      nonce_s=message->getNonce();
      if(*nonce_s!=(this->nonce-1))
      {
        verbose<<"--> [MainClient][reciveLogoutProtocol] error the nonce isn't valid"<<'\n';
        delete nonce_s;
        //clientPhase=ClientPhase::NO_PHASE;
        return false;
      }
      if(message->getMessageType()!=LOGOUT_OK || clientPhase!=ClientPhase::LOGOUT_PHASE)
      {
        verbose<<"--> [MainClient][reciveLogoutProtocol] message type not expected"<<'\n';
        return false;
      }

      //verbose<<"--> [MainClient][reciveLogoutProtocol] decript start"<<'\n';
      res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
      if(!res)
        return false; 
      clientPhase=ClientPhase::NO_PHASE;
      logged=false;
      username="";
      startChallenge=false;
      implicitUserListReq=false;
      return true;
      
  }

/*
---------------------keyExchangeReciveProtocol--------------------
*/
  bool MainClient::keyExchangeReciveProtocol(Message* message,bool exchangeWithServer)
  {
      unsigned char* app; 
      int len;
      int* nonce_s=message->getNonce();
      bool res;
      if(message==nullptr)
      {
        verbose<<"--> [MainClient][keyExchangeReciveProtocol(] error the message is null"<<'\n';
        return false;
      }
      nonce_s=message->getNonce();
      if(*nonce_s!=(this->nonce-1))
      {
        verbose<<"--> [MainClient][keyExchangeProtocol] nonce not valid"<<'\n';
        delete nonce_s;
        return false;
      }
      if(message->getMessageType()==KEY_EXCHANGE)
      {
       
        res=cipher_client->fromSecureForm( message , username ,nullptr,exchangeWithServer);
        if(!res)
          return false; 
        app=message->getDHkey();
        len=message->getDHkeyLength();
        if(app==nullptr)
          return false;
        if(exchangeWithServer)
        {
          this->aesKeyServer=cipher_client->getSessionKey( app , len );
          if(this->aesKeyServer==nullptr||this->aesKeyServer->iv==nullptr || this->aesKeyServer->sessionKey==nullptr)
            return false;
          vverbose<<"-->[MainClient][keyExchangeReciveProtoco] key iv "<<aesKeyServer->iv<<" session key: "<<aesKeyServer->sessionKey<<'\n';//da eliminare
          delete nonce_s;
          return true;

       }
       this->aesKeyClient=cipher_client->getSessionKey( app , len );
       if(this->aesKeyClient==nullptr||this->aesKeyClient->iv==nullptr || this->aesKeyClient->sessionKey==nullptr)
         return false;
       vverbose<<"-->[MainClient][keyExchangeReciveProtoco] key iv "<<aesKeyClient->iv<<" session key: "<<aesKeyClient->sessionKey<<'\n';//da eliminare
       delete nonce_s;
       return true;
        
      }
      return false;
  }
/*
--------------------------------keyExchangeClientSend-------------------------------
*/

  bool MainClient::keyExchangeClientSend()
  {
    bool res;
    int* nonce_s;
    bool connection_res;
    bool socketIsClosed;
    Message* retMess;
    Message* sendMess;
    sendMess=createMessage(MessageType::KEY_EXCHANGE, nullptr,nullptr,0,nullptr,this->currentToken,true);
    if(sendMess==nullptr)
      return false;
     verbose<<"-->[MainClient][loginProtocol] start key exchange protocol"<<'\n';
     res=connection_manager->sendMessage(*sendMess,connection_manager->getserverSocket(),&socketIsClosed,advIP,*advPort); 
     if(!res)
    {
      if(socketIsClosed)
      notConnected=true;
      return false;
    }
    return true;
  
  }

/*
----------------------------------sendChallengeProtocol function------------------------------------------------------
*/
  bool MainClient::sendChallengeProtocol(const char* adversaryUsername,int size)
  {
     bool res;
     bool socketIsClosed=false;
     Message* message=nullptr;
     if(adversaryUsername==nullptr)
     {
       return false;
     }
     message=createMessage(MessageType::MATCH,(const char*)username.c_str(),(unsigned char*)adversaryUsername,size,aesKeyServer,this->nonce,false);
     if(message==nullptr)
     {
       return false;
     }
    res=connection_manager->sendMessage(*message,connection_manager->getserverSocket(),&socketIsClosed,nullptr,0);
    if(socketIsClosed)
    {
      verbose<<"-->[MainClient][sendChallengeProtocol] error server is offline reconnecting"<<'\n';
      notConnected=true;
      return false;
    }
    if(res)
    {
      vverbose<<"-->[MainClient][sendChallengeProtocol]start a challenge";
      startChallenge=true;
      challenged_username=string(adversaryUsername);
    }
    return res;
  }
/*
--------------------------reciveChallengeProtocol function------------------------------------
*/
  bool MainClient::receiveChallengeProtocol(Message* message)//da continuare con il challenge_register
  {
    bool res;
    int* nonce_s;
    string advUsername="";
    ChallengeInformation *data=nullptr;
    if(message==nullptr)
    {
      return false;
    }
    nonce_s=message->getNonce();
    if(*nonce_s!=(this->nonce))
    {
      verbose<<"--> [MainClient][reciveChallengeProtocol] error the nonce isn't valid"<<'\n';
      delete nonce_s;
      return false;
      }
    if(message->getMessageType()!=MATCH)
    {
      verbose<<"--> [MainClient][reciveChallengeProtocol] message type not expected"<<'\n';
        return false;
    }
    res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
    if(!res)
      return false;
    advUsername=message->getUsername();
    this->nonce++;
    data=new ChallengeInformation(advUsername);
    res=challenge_register->addData(*data);
    return res;
  }
/*
-----------------------------sendRejectProtocol----------------------------------
*/
  bool MainClient::sendRejectProtocol(const char* usernameAdv,int size)
  {
     bool res;
     bool socketIsClosed=false;
     ChallengeInformation *data=nullptr;
     Message* message=nullptr;
     if(usernameAdv==nullptr)
     {
       return false;
     }
     if(!challenge_register->findData(*data))
       return false;

     message=createMessage(MessageType::REJECT,(const char*)username.c_str(),(unsigned char*)usernameAdv,size,aesKeyServer,this->nonce,false);
     if(message==nullptr)
     {
       return false;
     }
    res=connection_manager->sendMessage(*message,connection_manager->getserverSocket(),&socketIsClosed,nullptr,0);
    if(socketIsClosed)
    {
      verbose<<"-->[MainClient][sendChallengeProtocol] error server is offline reconnecting"<<'\n';
      notConnected=true;
      return false;
    }
    if(res)
    {
      vverbose<<"-->[MainClient][sendChallengeProtocol]start a challenge";
      data=new ChallengeInformation(string(usernameAdv));

      res=challenge_register->removeData(*data);
      
    }
    return res;
  }
/*
--------------------------receiveRejectProtocol--------------------------------------
*/
  bool MainClient::receiveRejectProtocol(Message* message)
  {
    bool res;
    int* nonce_s;
    string advUsername="";
    ChallengeInformation *data=nullptr;
    if(message==nullptr)
    {
      return false;
    }
    nonce_s=message->getNonce();
    if(*nonce_s!=(this->nonce))
    {
      verbose<<"--> [MainClient][reciveChallengeProtocol] error the nonce isn't valid"<<'\n';
      delete nonce_s;
      return false;
      }
    if(message->getMessageType()!=REJECT)
    {
      verbose<<"--> [MainClient][reciveChallengeProtocol] message type not expected"<<'\n';
        return false;
    }
    res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
    if(!res)
      return false;
    challenged_username = "";
    this->nonce++;
    startChallenge=false;
    return res;
  }
/*
-------------------------sendChatProtocol----------------------
*/
  bool MainClient::sendChatProtocol(string chat)
  {
     Message* message;
     bool res=false;
     bool socketIsClosed=false;
     if(chat.empty())
     {
       verbose<<"--> [MainClient][sendChatProtocol] string empty"<<'\n';
       return false;
     } 
     if( messageChatToACK!=nullptr)
     {
       chatWait.emplace_back(chat);
       return true;
     }
     message=createMessage(MessageType::CHAT, nullptr,(unsigned char*)chat.c_str(),chat.size(),aesKeyClient,currTokenChat,false);
     if(message==nullptr)
     {
       verbose<<"--> [MainClient][sendChatProtocol] error to create message"<<'\n';
       return false;
     }
     else
     {
       res=connection_manager->sendMessage(*message,connection_manager->getsocketUDP(),&socketIsClosed,(const char*)advIP,*advPort);
       if(!res)
       {
         verbose<<"--> [MainClient][sendChatProtocol] error to send message"<<'\n';
         return false;         
       }
       time(&startWaitChatAck);
       messageChatToACK=message;
       game->addMessageToChat(chat);
       textual_interface_manager->printGameInterface(true, string("15"),game->getChat(),game->printGameBoard());
       return true;
     }
  }

/*
---------------------receiveChatProtocol--------------------------
*/
  bool MainClient::reciveChatProtocol(Message* message)
  {
    bool res;
    int* nonce_s;
    bool socketIsClosed=false;
    Message* messageACK;
    string advUsername="";
    ChallengeInformation *data=nullptr;
    if(message==nullptr)
    {
      return false;
    }
    nonce_s=message->getCurrent_Token();
    if(*nonce_s!=(this->currTokenChatAdv))
    {
      //verbose<<"--> [MainClient][reciveChatProtocol] error the nonce isn't valid"<<'\n';
      res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
      if(!res)
      {
        verbose<<"--> [MainClient][reciveChatProtocol] error to decrypt"<<'\n';
        return false;
      }
      messageACK=createMessage(ACK,nullptr,nullptr,0,aesKeyClient,*nonce_s,false);
      connection_manager->sendMessage(*messageACK,connection_manager->getsocketUDP(),&socketIsClosed,(const char*)advIP,*advPort);
      delete nonce_s;
      return true;
      }
    if(message->getMessageType()!=CHAT)
    {
      verbose<<"--> [MainClient][reciveChatProtocol] message type not expected"<<'\n';
        return false;
    }
    res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
    if(!res)
    {
      verbose<<"--> [MainClient][reciveChatProtocol] error to decrypt"<<'\n';
      return false;
    }
    messageACK=createMessage(ACK,nullptr,nullptr,0,aesKeyClient,*nonce_s,false);
    connection_manager->sendMessage(*messageACK,connection_manager->getsocketUDP(),&socketIsClosed,(const char*)advIP,*advPort);
    game->addMessageToChat(printableString(message->getMessage(),message->getMessageLength()));
    textual_interface_manager->printGameInterface(true, string("15"),game->getChat(),game->printGameBoard());
    this->currTokenChatAdv+=2;
    return true;
  }
/*
---------------------receiveACKProtocol-------------------------
*/
  bool MainClient::receiveACKChatProtocol(Message* message)
  {	
    bool res;
    if(messageChatToACK)
      return false;
    if(message==nullptr)
      return false;
    if(message->getMessageType()!=ACK)
      return false;
    
    res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
    if(!res)
    {
      verbose<<"--> [MainClient][reciveACKProtocol] error to decrypt"<<'\n';
      return false;
    }   
    if(*message->getCurrent_Token() != *messageChatToACK->getCurrent_Token())
       return false;
    delete messageChatToACK;
    messageChatToACK=nullptr;//verificare funzionamento
    
    if(!chatWait.empty())
    {
      if(!sendChatProtocol(chatWait[0]))
      {
        verbose<<"--> [MainClient][reciveACKProtocol] some problem to send chat"<<'\n';
      }
      chatWait.erase(chatWait.begin());
    }
    return true;
  }
/*
---------------------------------------sendAccept------------------------------
*/
   bool MainClient::sendAcceptProtocol(const char* usernameAdv,int size)
   {
     bool res;
     bool socketIsClosed=false;
     ChallengeInformation *data=nullptr;
     Message* message=nullptr; 
     if(usernameAdv ==nullptr)
     {
       return false;
     }  
     if(!challenge_register->findData(*data))
       return false;

     message=createMessage(MessageType::ACCEPT,(const char*)username.c_str(),(unsigned char*)usernameAdv,size,aesKeyServer,this->nonce,false);
     if(message==nullptr)
     {
       return false;
     }
     res=connection_manager->sendMessage(*message,connection_manager->getserverSocket(),&socketIsClosed,nullptr,0);
     if(socketIsClosed)
     {
       verbose<<"-->[MainClient][sendAcceptProtocol] error server is offline reconnecting"<<'\n';
       notConnected=true;
       return false;
     }
     adv_username_1 =string(usernameAdv);
     clientPhase=START_GAME_PHASE;
     delete challenge_register;
     challenge_register= nullptr;//da valutare un possibile spostamento
     challenge_register = new ChallengeRegister();//da valutare un possibile spostamento
     return true;
   }
/*
--------------------------receiveAcceptProtocol--------------------------------------
*/
  bool MainClient::receiveAcceptProtocol(Message* message)
  {
    bool res;
    int* nonce_s;
    string advUsername="";
    ChallengeInformation *data=nullptr;
    if(message==nullptr)
    {
      return false;
    }
    nonce_s=message->getNonce();
    if(*nonce_s!=(this->nonce))
    {
      verbose<<"--> [MainClient][reciveAcceptProtocol] error the nonce isn't valid"<<'\n';
      delete nonce_s;
      return false;
      }
    if(message->getMessageType()!=ACCEPT)
    {
      verbose<<"--> [MainClient][reciveAcceptProtocol] message type not expected"<<'\n';
        return false;
    }
    res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
    if(!res)
      return false;
     clientPhase=START_GAME_PHASE;
     adv_username_1 = challenged_username;
     delete challenge_register;
     this->nonce++;
     challenge_register= nullptr;
     challenge_register = new ChallengeRegister();
     return true;
  }
/*
-------------------------receiveGameParamProtocol------------------------------------------
*/
  bool MainClient::receiveGameParamProtocol(Message* message)
  {
    bool res;
    int* nonce_s;
    int keyLen;
    string advUsername="";
    ChallengeInformation *data=nullptr;
    unsigned char* pubKeyAdv=nullptr;
    if(message==nullptr)
    {
      return false;
    }
    nonce_s=message->getNonce();
    if(*nonce_s!=(this->nonce))
    {
      verbose<<"--> [MainClient][receiveGameParamProtocol] error the nonce isn't valid"<<'\n';
      delete nonce_s;
      return false;
      }
    if(message->getMessageType()!=GAME_PARAM||clientPhase!=START_GAME_PHASE)
    {
      verbose<<"--> [MainClient][receiveGameParamProtocol] message type not expected"<<'\n';
        return false;
    }
    res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
    if(!res)
      return false;
    this->nonce++;
    advPort= message->getPort();
    pubKeyAdv=message->getPubKey();
    keyLen=message->getPubKeyLength();
    res=cipher_client->setAdversaryRSAKey( pubKeyAdv , keyLen );
    if(!res)
      return false;
    advIP=(char*)message->getNetInformations();
    if(advIP==nullptr)
      return false;
    return true;
    
  }
/*

-------------------------------sendWithDrawProtocol---------------------------
*/
  bool MainClient::sendWithDrawProtocol()
  {
     if(challenged_username.empty())
     {
       return false;
     }
     bool res;
     bool socketIsClosed=false;
     ChallengeInformation *data=nullptr;
     Message* message=nullptr; 
    /* if(adversaryUsername.empty())
     {
       return false;
     }*/ 
     if(!challenge_register->findData(*data))
       return false;

     message=createMessage(MessageType::WITHDRAW_REQ,(const char*)username.c_str(), (unsigned char*)challenged_username.c_str(), challenged_username.size(),aesKeyServer,this->nonce,false);
     if(message==nullptr)
     {
       return false;
     }
     res=connection_manager->sendMessage(*message,connection_manager->getserverSocket(),&socketIsClosed,nullptr,0);
     if(socketIsClosed)
     {
       verbose<<"-->[MainClient][sendWithDrawProtocol] error server is offline reconnecting"<<'\n';
       notConnected=true;
       return false;
     }
     return true;
  }
/*
----------------------------receiveWithDrawOkProtocol----------------------------
*/
  bool MainClient::receiveWithDrawOkProtocol(Message* message)
  {
    bool res;
    int* nonce_s;
    string advUsername="";
    ChallengeInformation *data=nullptr;
    if(message==nullptr)
    {
      return false;
    }
    nonce_s=message->getNonce();
    if(*nonce_s!=(this->nonce-1))
    {
      verbose<<"--> [MainClient][receiveWithDrawOkProtocol] error the nonce isn't valid"<<'\n';
      delete nonce_s;
      return false;
      }
    if(message->getMessageType()!=WITHDRAW_OK)
    {
      verbose<<"--> [MainClient][receiveWithDrawOkProtocol] message type not expected"<<'\n';
        return false;
    }
    res=cipher_client->fromSecureForm( message , username ,aesKeyServer,false);
    if(!res)
      return false;
     //clientPhase=START_GAME_PHASE;
     adv_username_1 = "";
     string challenged_username = "";
     return true;
  }
 /*
--------------------------------------createMessage function---------------------------------
*/
  Message* MainClient::createMessage(MessageType type, const char* param,unsigned char* g_param,int g_paramLen,cipher::SessionKey* aesKey,int token,bool keyExchWithClient)
  {
    NetMessage* partialKey;
    NetMessage* net;
    bool cipherRes=true;
    Message* message = new Message();
    switch(type)
    {
      case CERTIFICATE_REQ:
        message->setMessageType( CERTIFICATE_REQ );
        message->setNonce(0);
        break;
      case LOGIN_REQ:
        message->setMessageType( LOGIN_REQ );
        message->setNonce(this->nonce);
        message->setPort( this->myPort );
        message->setUsername(param );
        cipherRes=cipher_client->toSecureForm( message,aesKey);
        this->nonce++;
        break;
        
      case KEY_EXCHANGE:
        message->setMessageType( KEY_EXCHANGE );
        if(!keyExchWithClient)
        {
          message->setNonce(this->nonce);
          partialKey = this->cipher_client->getPartialKey();
          message->set_DH_key( partialKey->getMessage(), partialKey->length() );
          cipherRes=this->cipher_client->toSecureForm( message,aesKey);
          this->nonce++;
        }
        else
        {
          message->setCurrent_Token(this->currentToken);
          partialKey = this->cipher_client->getPartialKey();
          message->set_DH_key( partialKey->getMessage(), partialKey->length() );
          cipherRes=this->cipher_client->toSecureForm( message,aesKey);
          this->currentToken++;
       }
        break;

      case USER_LIST_REQ:
        message->setMessageType(USER_LIST_REQ);
        message->setNonce(this->nonce);
        cipherRes =this->cipher_client->toSecureForm( message,aesKey);
        this->nonce++;
        

        break;

      case RANK_LIST_REQ:
        message->setMessageType( RANK_LIST_REQ );
        message->setNonce(this->nonce);
        cipherRes =this->cipher_client->toSecureForm( message,aesKey );
        this->nonce++;
        break;

      case LOGOUT_REQ:
        message->setMessageType( LOGOUT_REQ );
        message->setNonce(this->nonce);
        
        cipherRes=this->cipher_client->toSecureForm( message,aesKey);
        this->nonce++;
        break;

      case ACCEPT:
        message->setMessageType( ACCEPT );
        message->setNonce(this->nonce);
        message->setAdversary_1(param);
        message->setAdversary_2(this->username.c_str());
        cipherRes = this->cipher_client->toSecureForm( message,aesKey);
        this->nonce++;
        break;

      case REJECT:
        message->setMessageType( REJECT );
        message->setNonce(this->nonce);
        message->setAdversary_1(param );
        message->setAdversary_2(this->username.c_str());
        cipherRes = this->cipher_client->toSecureForm( message,aesKey);
        this->nonce++;
        break;

      case WITHDRAW_REQ:
        message->setMessageType( WITHDRAW_REQ );
        message->setNonce(this->nonce);
        message->setUsername(this->username);
        cipherRes = this->cipher_client->toSecureForm( message,aesKey);
        this->nonce++;
        break;

      case MATCH:
        message->setMessageType( MATCH );
        message->setNonce(this->nonce);
        message->setUsername(string(param) );
        cipherRes = this->cipher_client->toSecureForm( message,aesKey);
       // net = Converter::encodeMessage(MATCH, *message );                //da vedere l'utilità in caso cancellare
       // message = this->cipher_client->toSecureForm( message,aesKey );
        this->nonce++;
        break;
      case MOVE:
        message->setMessageType(MOVE);
        message->setCurrent_Token(this->currentToken);
        message->setChosenColumn(  g_param,g_paramLen);
        cipherRes = this->cipher_client->toSecureForm( message,aesKey);
        //this->nonce++;
        break;
      case DISCONNECT:
        message->setMessageType( DISCONNECT );
        message->setNonce(this->nonce);
        message->setUsername(string(param) );
        cipherRes = this->cipher_client->toSecureForm( message,aesKey);
        //net = Converter::encodeMessage(MATCH, *message );               //da vedere l'utilità in caso cancellare
        //message = this->cipher_client->toSecureForm( message,aesKey );
        this->nonce++;
        break;

      case ACK:
        message->setMessageType(ACK);
        message->setCurrent_Token(token); 
        cipherRes = this->cipher_client->toSecureForm( message,aesKey);
        break;

      case CHAT:
        message->setMessageType(CHAT);     
        message->setCurrent_Token(this->currTokenChat);
        if( g_param==nullptr||aesKey==nullptr)
          return nullptr;
        message->setMessage( g_param,g_paramLen );
        cipherRes = this->cipher_client->toSecureForm( message,aesKey);
        this->currTokenChat+=2;
        break;

     case GAME:
        message->setMessageType(GAME); 
        message->setCurrent_Token(this->currentToken);
        message->setChosenColumn(  g_param,g_paramLen);
        cipherRes=cipher_client->toSecureForm( message,aesKey );
        break;

    }
    if(!cipherRes)
      return nullptr;
    return message;
  }
/*
---------------------------concatenateTwoField function--------------------------------------
*/
  unsigned char* MainClient::concTwoField(unsigned char* firstField,unsigned int firstFieldSize,unsigned char* secondField,unsigned int secondFieldSize,unsigned char separator,unsigned int numberSeparator)
  {
    if(firstField==nullptr||secondField==nullptr)
      return nullptr;
    int j=0;
    unsigned char* app=new unsigned char[firstFieldSize+secondFieldSize+numberSeparator];
    for(int i=0;i<firstFieldSize;++i)
    {
      app[i]=firstField[i];
    }
    for(int i=firstFieldSize;i<(firstFieldSize+numberSeparator);i++)
    {
      app[i]=separator;
    }
    for(int i=(firstFieldSize+numberSeparator);i<(firstFieldSize+numberSeparator+secondFieldSize);++i)
    {
     
      app[i]=secondField [j];
      ++j;
    }
    return app;
  }
/*
  ------------------------------deconcatenateTwoField function----------------------------------
*/
  bool MainClient::deconcatenateTwoField(unsigned char* originalField,unsigned int originalFieldSize,unsigned char* firstField,unsigned int* firstFieldSize,unsigned char* secondField,unsigned int* secondFieldSize, unsigned char separator,unsigned int numberSeparator)
  {
    int counter=0;
    int firstDimension=0;
    int secondDimension=0;
    if(originalField==nullptr||firstFieldSize==nullptr||secondField==nullptr)
      return false;
    for(int i=0;i<originalFieldSize;++i)
    {
       if(originalField[i]==separator)
       {
         ++counter;
         if(counter==numberSeparator)
         {
           firstDimension = (i-(numberSeparator-1));
           secondDimension= originalFieldSize-i;
         }
       }
       else
       {
         --counter;
       }
    }
    firstField=new unsigned char[firstDimension];
    secondField=new unsigned char[secondDimension];
    for(int i=0;i<firstDimension;++i)
    {
      firstField[i]=originalField[i];      
    }
    int j=0;
    for(int i=(firstDimension+numberSeparator);i<originalFieldSize;++i)
    {    
      secondField[j]=originalField[i];
      ++j;
    }
    *firstFieldSize=firstDimension;
    *secondFieldSize=secondDimension;
    return true;
  }
/*
----------------------function startConnectionServer-----------------------------
*/
bool MainClient::startConnectionServer(const char* myIP,int myPort)
{
  bool res;
  this->myIP=myIP;
  this->myPort=myPort;
  this->nonce=0;
  connection_manager=new ConnectionManager(false,this->myIP,this->myPort);
  try
  {
    res=connection_manager->createConnectionWithServerTCP(this->serverIP,this->serverPort);
  }
  catch(exception e)
  {
    res=false;
  }
  return res;
}

/*
------------------------------MakeAndSendGameMove-------------------------------------
*/
  bool MainClient::MakeAndSendGameMove(int column)
  {
    bool res=false;
    time_t start;
    bool waitForAck=false;
    vector<int> descrList;
    bool iWon=false;
    bool adversaryWon=false;
    bool tie=false;
    bool socketIsClosed=false;
    unsigned char* resu;
    Message* message;
    Message* appMess;
    Message* retMess;
    NetMessage* netMess;
    StatGame statGame;
    statGame=game->makeMove(column,&iWon,&adversaryWon,&tie,true);
    switch(statGame)
    {
      case BAD_MOVE:
        std::cout<<"The collumn selected is full."<<endl;
        std::cout<<"\t# Insert a command:";
        cout.flush();
        break;
      case NULL_POINTER:
        verbose<<"-->[MainClient][MakeAndSendGameMove] nullptr as parameter"<<'\n';
        break;
      case OUT_OF_BOUND:
        std::cout<<"The collumn selected doesn't exist."<<endl;
        std::cout<<"\t# Insert a command:";
        cout.flush();
        break;
      case BAD_TURN:
        verbose<<"-->[MainClient][MakeAndSendGameMove] error bad turn"<<'\n';
        break;
      case MOVE_OK:
       case GAME_FINISH://verify if ok
        std::string app=std::to_string(column);
        appMess = createMessage(MessageType::GAME,nullptr,(unsigned char*) app.c_str(),app.size(),nullptr,MessageGameType::MOVE_TYPE,false);
        if(appMess==nullptr)
          return false;
        netMess=Converter::encodeMessage(MessageType::GAME , *appMess );
        if(netMess==nullptr)
          return false;
        resu=concTwoField((unsigned char*) app.c_str(),app.size(),netMess->getMessage() ,netMess->length(),'&',NUMBER_SEPARATOR);
        message=createMessage(MessageType::MOVE,nullptr,resu,app.size() + netMess->length() + NUMBER_SEPARATOR,aesKeyClient,MessageGameType::MOVE_TYPE,false);
        connection_manager->sendMessage(*message,connection_manager->getsocketUDP(),&socketIsClosed,(const char*)advIP,*advPort);
        time(&start);
        waitForAck=true;
        while(waitForAck)
        {
          descrList=connection_manager->waitForMessage(nullptr,nullptr);
          if(!descrList.empty())
          {        
            for(int idSock: descrList)
            {
              if(idSock!=connection_manager->getstdinDescriptor())
              {
                retMess=connection_manager->getMessage(idSock);
                if(retMess==nullptr)
                  continue;
                switch(retMess->getMessageType())
                {
                  case ACK:
                   if(*retMess->getCurrent_Token()!=*message->getCurrent_Token())
                     continue;
                   res=cipher_client->fromSecureForm( retMess , username ,aesKeyClient,false); 
                   if(!res)
                     continue;
                  waitForAck=false;
                  textual_interface_manager->printGameInterface(true, string("15"),game->getChat(),game->printGameBoard());
                  this->currentToken++;
                  if(statGame==GAME_FINISH)
                  {
                    if(iWon)
                    {
                      cout<<"\t you won"<<endl;
                      cout.flush();
                    }
                    else if(adversaryWon)
                    {
                      cout<<"\t you lose"<<endl;
                      cout.flush();                
                    }
                    else if(tie)
                    {
                      cout<<"\t it's a tie"<<endl;
                      cout.flush(); 
                    }
                  
                    sleep(3000);
                    adv_username_1 = "";
                    clientPhase= ClientPhase::NO_PHASE;
                    sendImplicitUserListReq();
                  }
                }
              }
            }
            if(waitForAck && difftime(time(NULL),start)>SLEEP_TIME )
            {
              connection_manager->sendMessage(*message,connection_manager->getsocketUDP(),&socketIsClosed,(const char*)advIP,*advPort);
              time(&start);
              waitForAck=true;
            }
          }
        }
        delete netMess;
        delete appMess;
        break;
        //da continuare 
       
    }
    return true;
  }
/*
----------------------Generate nonceNumber----------------
*/
  int MainClient::generateRandomNonce()
  {
    unsigned int seed;
      FILE* randFile = fopen( "/dev/urandom","rb" );
      struct timespec ts;

      if( !randFile )
      {
        verbose<<" [MainClient][generateRandomNonce] Error, unable to locate urandom file"<<'\n';
        if( timespec_get( &ts, TIME_UTC )==0 )
        {
          verbose << "--> [MainClient][generateRandomNonce] Error, unable to use timespec" << '\n';
          srand( time( nullptr ));
        }
        else
          srand( ts.tv_nsec^ts.tv_sec );
        return rand();
      }

      if( fread( &seed, 1, sizeof( seed ),randFile ) != sizeof( seed ))
      {
        verbose<<" [MainClient][generateRandomNonce] Error, unable to load enough data to generate seed"<<'\n';
        if( timespec_get( &ts, TIME_UTC ) == 0 )
        {
          verbose << "--> [MainClient][generateRandomNonce] Error, unable to use timespec" << '\n';
          srand( time( NULL ));
        }
        else
          srand( ts.tv_nsec^ts.tv_sec );
      }
      else
        srand(seed);

      fclose( randFile );
      return rand();

    }
/*
------------------------------ReciveGameMove-------------------------------------
*/

  void MainClient::ReciveGameMove(Message* message)
  {
    bool iWon=false;
    bool adversaryWon=false;
    bool tie=false;
    bool res=false;
    bool socketIsClosed=false;
    string app;
    StatGame statGame;
    unsigned char* chosenColl;
    unsigned int chosenCollLen=0;
    int collMove;
    int collGame;
    unsigned char* gameMess;
    NetMessage* netGameMess;
    Message* messG;
    unsigned int gameMessLen=0;
    int* c_token;
    int appLen;
    unsigned char* c_app;
    Message* messageACK;
    if(message==nullptr)
      return;
    if(message->getMessageType()!=MessageType::MOVE)
      return;
    c_token=message->getCurrent_Token();
    res=cipher_client->fromSecureForm( message, username ,aesKeyClient,false); 
    if(!res)
    {
      return;
    }
    if(*c_token!=this->currentToken)
    {
      messageACK=createMessage(ACK,nullptr,nullptr,0,aesKeyClient,*c_token,false);
      connection_manager->sendMessage(*messageACK,connection_manager->getsocketUDP(),&socketIsClosed,(const char*)advIP,*advPort);
      return;
    }
    c_app=message->getChosenColumn();
    appLen=message->getChosenColumnLength();
    deconcatenateTwoField(c_app,appLen,chosenColl,&chosenCollLen,gameMess,&gameMessLen, '&',NUMBER_SEPARATOR);
    netGameMess=new NetMessage(gameMess , gameMessLen );
    if(netGameMess)
    {
      verbose<<"-->[MainClient][ReceiveGameMove] impossible to extract game message type netMessage"<<'\n';
      return;
    }
   messG= Converter::decodeMessage( *netGameMess );
   if(messG==nullptr)
   {
      verbose<<"-->[MainClient][ReceiveGameMove] impossible to extract game message type Message"<<'\n';
      return;
   }
   app=printableString(chosenColl,chosenCollLen);
   collMove=std::stoi(app,nullptr,10);
   app= printableString(message->getChosenColumn(),message->getChosenColumnLength());
   collGame=std::stoi(app,nullptr,10);
   Message appG=*messG;
   res=cipher_client->fromSecureForm( &appG, username ,aesKeyClient,false); 
   if(!res)
     return;
   if(*messG->getCurrent_Token()!=*message->getCurrent_Token() ||collMove!=collGame )
     return;
   messageACK=createMessage(ACK,nullptr,nullptr,0,aesKeyClient,this->currentToken,false);
   connection_manager->sendMessage(*messageACK,connection_manager->getsocketUDP(),&socketIsClosed,(const char*)advIP,*advPort);
   if(!cipher_client->toSecureForm( messG, aesKeyServer ))
     return;
   statGame=game->makeMove(collMove,&iWon,&adversaryWon,&tie,false);
   if(statGame!=StatGame::MOVE_OK || statGame!=StatGame::GAME_FINISH)
   {
     return;
   }
   
   connection_manager->sendMessage(*messG,connection_manager->getserverSocket(),&socketIsClosed,nullptr,0);
   this->currentToken++;
   textual_interface_manager->printGameInterface(true, string("15"),game->getChat(),game->printGameBoard());
   if(socketIsClosed)
   {
     vverbose<<"-->[MainClient][ReceiveGameMove] error to handle"<<'\n';
     return;
   }

   if(statGame==GAME_FINISH)
   {
     if(iWon)
     {
       cout<<"\t you won"<<endl;
       cout.flush();
     }
     else if(adversaryWon)
     {
       cout<<"\t you lose"<<endl;
       cout.flush();                
     }
     else if(tie)
     {
       cout<<"\t it's a tie"<<endl;
       cout.flush(); 
     }
       
      sleep(3000);
      clientPhase= ClientPhase::NO_PHASE;
      clearGameParam();
      sendImplicitUserListReq();
    }         
     
  }

/*
------------------------------function errorHandler----------------------------
*/
  bool MainClient::errorHandler(Message* message)
  {
    bool res;         
    res=cipher_client->fromSecureForm( message , username ,aesKeyServer,true);
    int *nonce_s=message->getNonce();
    if(res==false || *nonce_s!=(this->nonce-1))
    {
      return false;
    }
    //this->nonce++;
    std::cout<<"error to server request try again."<<endl;
    clientPhase=ClientPhase::NO_PHASE;
    std::cout<<"\t# Insert a command:";
    cout.flush();
    return true;
  }


/*
------------------------comand function------------------------------------

*/
  bool MainClient::comand(std::string comand_line)
  {
    if(comand_line.empty())
    {
      verbose<<""<<"--> [MainClient][comand] error comand_line is empty"<<'\n';
      return false;
    }
    try
    {
      if(comand_line.compare(0,5,"login")==0)
      {
        if(logged)
        {
          std::cout<<"already logged"<<endl;
          std::cout<<"\t# Insert a command:";
          std::cout.flush();
          return true;
        }
        string password;
        string username;
        cout<<"username:";
        cin>>username;
        cout<<'\n';
      
      
        cout<<"password:";

        termios oldt;
        tcgetattr(STDIN_FILENO, &oldt);
        termios newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);//hide input
        std::cin>>password;
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);//show input
        cout<<'\n';
        if(username.empty()||password.empty())
        {
          std::cout<<"username or password not valid"<<endl;
        }
        cipher_client->newRSAParameter(username,password);
        if(!cipher_client->getRSA_is_start())
        {
          std::cout<<"login failed retry"<<endl;
          std::cout<<"\t# Insert a command:";
          cout.flush();
        }
        else
        {
          bool socketIsClosed=false;
          if(loginProtocol(username,&socketIsClosed))
          {
            this->username=username;
            this->logged=true;
            if(!sendImplicitUserListReq())
            {
              implicitUserListReq=false;
            }
           // textual_interface_manager->printMainInterface(this->username," ","online","none","0");
          }
         }
         return true;
       }
       else if(comand_line.compare(0,4,"show")==0)
       {
         if(comand_line.compare(5,5,"users")==0)
         {
           if(!sendReqUserListProtocol())
           {
             std::cout<<"show user online failed retry \n \t# Insert a command:";
           }
         }
         else if(comand_line.compare(5,4,"rank")==0)
         {
           if(!sendRankProtocol())
           {
             std::cout<<"show user online failed retry"<<endl;
             std::cout<<"\t# Insert a command:";
             cout.flush();
           }
         }
         else if(comand_line.compare(5,7,"pending")==0)
         {
            string toPrint=challenge_register->printChallengeList();
            if(toPrint.empty())
            {
              std::cout<<"no active request game"<<endl;
            }
            else
            {
              std::cout<<"challenger list:"<<endl;
              std::cout<<toPrint<<endl;
            }
            std::cout<<"\t# Insert a command:";
            cout.flush();
         }
      }
      
      else if(comand_line.compare(0,9,"challenge")==0)
      {
      	 string app=comand_line.substr(10);
         if(app.empty())
         {
             std::cout<<"failed to send challenge "<<endl;
             std::cout<<"\t# Insert a command:";
             cout.flush();
         }
         bool res=sendChallengeProtocol(app.c_str(),comand_line.size());
         if(!res)
         {
             std::cout<<"failed to send challenge "<<endl;
             std::cout<<"\t# Insert a command:";
             cout.flush();
         }
      }
      else if(comand_line.compare(0,6,"accept")==0)
      {
      	 string app=comand_line.substr(7);
         if(app.empty())
         {
             std::cout<<"failed to send challenge "<<endl;
             std::cout<<"\t# Insert a command:";
             cout.flush();
         }
         bool res=sendAcceptProtocol(app.c_str(),comand_line.size());
         if(!res)
         {
             std::cout<<"failed to send challenge "<<endl;
             std::cout<<"\t# Insert a command:";
             cout.flush();
         }
      }

      else if(comand_line.compare(0,6,"reject")==0)
      {
      	 string app=comand_line.substr(7);
         if(app.empty())
         {
             std::cout<<"failed to send challenge "<<endl;
             std::cout<<"\t# Insert a command:";
             cout.flush();
         }
         bool res=sendRejectProtocol(adv_username_1.c_str(),comand_line.size());
         if(!res)
         {
             std::cout<<"failed to send challenge "<<endl;
             std::cout<<"\t# Insert a command:";
             cout.flush();
         }
      }
      else if(comand_line.compare(0,6,"logout")==0&&logged==true)
      {
        //ESEGUO LOGOUT
        bool ret=sendLogoutProtocol();
        if(!ret)
        {
          std::cout<<"logout failed retry"<<endl;
          std::cout<<"\t# Insert a command:";
          cout.flush();
        }
      }
      else if(comand_line.compare(0,8,"withdraw")==0&&logged==true && (clientPhase!=INGAME_PHASE||clientPhase!=START_GAME_PHASE))
      {
       
        bool ret=sendWithDrawProtocol();
        if(!ret)
        {
          std::cout<<"withdraw failed retry"<<endl;
          std::cout<<"\t# Insert a command:";
          cout.flush();
        }
      }
      else
      {
        std::cout<<"\t  comand: "<<comand_line<<" not valid"<<endl;
        std::cout<<"\t# Insert a command:";
        cout.flush();
      }
      
    }
      catch(exception e)
      {
          std::cout<<"comand not valid"<<endl;
          std::cout<<"\t# Insert a command:";
          cout.flush();
      }
    return true;
  }

  MainClient::MainClient(const char* ipAddr , int port )
  {
    this->myIP=ipAddr;
    this->myPort=port;
  }
/*
------------------------MainClient----------------------------------
*/
  void MainClient::client()
  {
     //char* buffer;
     Message* message;
     std::vector<int> sock_id_list;
     int newconnection_id=0;
     string newconnection_ip="";
     lck_time=new std::unique_lock<std::mutex>(mtx_time,std::defer_lock);
     cipher_client=new cipher::CipherClient();//create new CipherClient object

     textual_interface_manager=new TextualInterfaceManager();
     bool res;
    while(true)
    {
       if(notConnected==true)
       {
         res=startConnectionServer(this->myIP,this->myPort);
         if(!res)
           exit(1);

     
         if(!certificateProtocol())
           exit(1);
         textual_interface_manager->printLoginInterface();
         notConnected=false;
        }

      string comand_line;
      sock_id_list= connection_manager->waitForMessage(&newconnection_id,&newconnection_ip);
      if(!sock_id_list.empty())
      {
        for(int idSock: sock_id_list)
        {
          if(idSock==connection_manager->getstdinDescriptor())
          {
            //buffer=new char[500];
            //std::fgets(buffer,400,stdin);
            //cin>>comand_line=*buffer;
            std::getline(std::cin,comand_line);
            comand(comand_line);
            //delete[] buffer;
          }
          if(idSock==connection_manager->getserverSocket())
          {
            
            message=connection_manager->getMessage(connection_manager->getserverSocket());
            if(message==nullptr)
            {
              continue;
            }
            switch(message->getMessageType())
            {
              case LOGOUT_OK:
                if(clientPhase==ClientPhase::LOGOUT_PHASE)
                {
                    res=receiveLogoutProtocol(message);
                    if(!res)
                      continue;
                    textual_interface_manager->printLoginInterface();

                }
                break;
              case USER_LIST:
               if(clientPhase==ClientPhase::USER_LIST_PHASE)
               {
                 res=receiveUserListProtocol(message);
                 if(!res)
                 {
                   if(clientPhase==ClientPhase::NO_PHASE)
                   {
                     cout<<"error to show the online users list"<<endl;
                     std::cout<<"\t# Insert a command:";
                     cout.flush();
                   }
                 }
               }
               break;

              case RANK_LIST:
               if(clientPhase==ClientPhase::RANK_LIST_PHASE)
               {
                 res=receiveRankProtocol(message);
                 if(!res)
                 {
                   if(clientPhase==ClientPhase::NO_PHASE)
                   {
                     cout<<"error to show the rank users list"<<endl;
                     std::cout<<"\t# Insert a command:";
                     cout.flush();
                   }
                 }
               }
               break;
              case MATCH:
                res=receiveChallengeProtocol(message);
                if(!res)
                {
                     cout<<"error to recive challengeProtocol"<<endl;
                     std::cout<<"\t# Insert a command:";
                     cout.flush();
                }
                break;
              case MOVE:
                ReciveGameMove(message);
                break;
              case CHAT:
                reciveChatProtocol(message);
                break;
              case ACK:
                receiveACKChatProtocol(message);
                break;
      
              case DISCONNECT:
                reciveDisconnectProtocol(message);
                break;

              case REJECT:
                receiveRejectProtocol(message);
                break;

              case GAME_PARAM:
                res=receiveGameParamProtocol(message);
                if(res)
                {
                  if(startingMatch)
                  {
                    this->currentToken=generateRandomNonce();
                    keyExchangeClientSend();
                   
                    this->currTokenChat=this->currentToken+TOKEN_GAP;
                    this->currTokenChatAdv=this->currTokenChat+1;
           
                    
                  }

                }//come gestire un possibile fallimento??
                break;
              case KEY_EXCHANGE:
                    if(keyExchangeReciveProtocol(message,false))
                    {
                      clientPhase=INGAME_PHASE;
                      game=new Game(250,startingMatch);
                      textual_interface_manager->printGameInterface(startingMatch, std::to_string(timer)," ",game->printGameBoard());
                      if(!startingMatch)
                      {
                        this->currentToken=*message->getCurrent_Token()+1;
                        this->currTokenChatAdv=this->currentToken+TOKEN_GAP;
                        this->currTokenChat=this->currTokenChatAdv + 1;
                        
                      }
                      if(!chatWait.empty())
                      {
                        chatWait.clear();
                      }
                      startingMatch=false;
                    }
                    break;
              case ACCEPT:
                 res=receiveAcceptProtocol(message);
                 if(res)
                   startingMatch=true;
                 break;
              case WITHDRAW_OK:
                receiveWithDrawOkProtocol(message);
                break;
              case ERROR:
              {
                res=errorHandler(message);
                if(res)
                {
                  cout<<"an error occured"<<endl;
                  std::cout<<"\t# Insert a command:";
                  cout.flush();
                  
                }
                break;
              }
           
              default:
                 vverbose<<"--> [MainClient][client] message_type: "<<message->getMessageType()<<" unexpected"<<'\n';
                 std::cout<<"\t# Insert a command:";
                 cout.flush();
            }
            delete message;
          }
        }
        
      }
      if(messageChatToACK!=nullptr)
      {
        if(difftime(time(NULL),startWaitChatAck)>SLEEP_TIME)
        {
           bool socketIsClosed;
           connection_manager->sendMessage(*messageChatToACK,connection_manager->getsocketUDP(),&socketIsClosed,(const char*)advIP,*advPort);
          
        }
      }
    }
  }
/*


--------------------------utility function---------------------------------------
*/
  string MainClient::printableString(unsigned char* toConvert,int len)
  {
    char* app=new char[len+1];
    string res="";
    if(len==0)
    {
      res="NO RANK_LIST";
    }
    for(int i =0;i<len;i++)
    {
      app[i]=toConvert[i];
    }
    app[len]='\0';
    res.append(app);
    delete[] app;
    return res;
  }
/*
-----------------------------countOccurences ---------------------------------
*/
  int MainClient::countOccurences(string source,string searchFor)
  {
    unsigned int count=0;
    unsigned int startPos=0;
    const string app=searchFor;
    if(source.empty()||searchFor.empty())
      return 0;
    if(searchFor.size()>source.size())
      return 0;
    unsigned int searchSize=searchFor.size();
    while(startPos+(searchSize-1)<source.size())
    {
      try
      {
        if(source.compare(startPos,searchSize,app)==0)
        {
          ++count;
          startPos+=searchSize;
        }
        else
          ++startPos;
      }
      catch(exception e)
      {
        break;
      }
    }
    return count; 

  }


  void MainClient::clearGameParam()
  {
    adv_username_1 = "";
    delete game;
    game=nullptr;
    chatWait.clear();
    if(messageChatToACK!=nullptr)
    {
      delete messageChatToACK;
      messageChatToACK=nullptr;
    }
    
  }
/*-----------------destructor-----------------------------------
*/
  MainClient::~MainClient()
  {
    if(serverIP!=nullptr)
      delete[] serverIP;
    if(myIP!=nullptr)
      delete[]myIP;
    if(game!=nullptr)
      delete game;
    if(aesKeyServer!=nullptr)
      delete aesKeyServer;
    if(aesKeyClient!=nullptr)
      delete aesKeyClient;
    if(textual_interface_manager!=nullptr)
      delete textual_interface_manager;
    if(connection_manager!=nullptr)
    {
      connection_manager->closeConnection(connection_manager->getserverSocket());
      delete connection_manager;
    }
  }
}
  
/*
--------------------main function-----------------
*/  
  int main(int argc, char** argv)
  {
    Logger::setThreshold(  VERY_VERBOSE );
    //Logger::setThreshold(  VERBOSE );
    client::MainClient* main_client;
    if(argc==1)
    {
      main_client=new client::MainClient("127.0.0.1",12000);
      main_client->client();
    }
    else
    {
      main_client=new client::MainClient("127.0.0.1",atoi(argv[1]));
      main_client->client();
    }
    return 0;
  }
