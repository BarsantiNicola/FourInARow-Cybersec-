
#ifndef FOURINAROW_MAINSERVER_H
#define FOURINAROW_MAINSERVER_H

#include "ClientRegister.h"
#include "UserRegister.h"
#include "MatchRegister.h"
#include "SQLConnector.h"
#include "../Logger.h"
#include "../utility/Message.h"
#include "../cipher/CipherServer.h"
#include "../utility/ConnectionManager.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

using namespace server;

namespace server {

    ///////////////////////////////////////////////////////////////////////////////////////
    //                                                                                   //
    //                                   MAIN SERVER                                     //
    //    The class implements a server based on a given ipv4 address and a port.        //
    //    The server wait to receive a message after sanitization and identification of  //
    //    its content is assign it to a protocol handler which generate the correct      //
    //    behavior. The server is based on three register which collect information      //
    //    about the connected clients and a CipherServer class which contains the        //
    //    security routines to be applied during its work.                               //
    //                                                                                   //
    ///////////////////////////////////////////////////////////////////////////////////////

    class MainServer {

        private:
            ClientRegister clientRegister;        //  COLLECTS INFORMATION ABOUT CONNECTED CLIENTS(IPADDR, SOCKET)
            UserRegister userRegister;            //  COLLECTS INFORMATION ABOUT THE LOGGED USERS(NAME,KEYS,STATUS)
            MatchRegister matchRegister;          //  COLLECTS INFORMATION ABOUT THE OPENED MATCHES(PARTECIPANTS, STATUS)
            utility::ConnectionManager* manager;  //  GIVES ROUTINES TO RECEIVE AND SEND MESSAGES INTO NETWORK
            cipher::CipherServer cipherServer;    //  GIVES ROUTINES TO APPLY SECURITY PROTOCOLS

            //  PROTOCOL HANDLERS
            Message* certificateHandler( Message* message , int socket );       //  MANAGES MESSAGE CERTIFICATE_REQ                        //////////////// DONE
            Message* loginHandler( Message* message,  int socket );             //  MANAGES MESSAGE LOGIN_REQ                              //////////////// DONE
            Message* keyExchangeHandler( Message* message , string username );  //  MANAGES MESSAGE KEY_EXCHANGE                           //////////////// DONE
            Message* gameParamHandler( string source , int match , bool step ); //  MANAGES MESSAGE GAME_PARAM                             //////////////// TODO
            Message* userListHandler( Message* message, string username );      //  MANAGES MESSAGE USER_LIST_REQ                          //////////////// DONE
            Message* rankListHandler( Message* message, string username );      //  MANAGES MESSAGE RANK_LIST_REQ                          //////////////// DONE
            Message* matchListHandler( Message* message );                      //  MANAGES MESSAGE MATCH                                  //////////////// TODO
            Message* acceptHandler( Message* message);                          //  MANAGES MESSAGE ACCEPT                                 //////////////// TODO
            Message* rejectHandler( Message* message);                          //  MANAGES MESSAGE REJECT                                 //////////////// TODO
            Message* disconnectHandler( Message* message, int matchID);         //  MANAGES MESSAGE DISCONNECT                             //////////////// TODO
            Message* logoutHandler( Message* message , string username );       //  MANAGES MESSAGE LOGOUT_REQ                             //////////////// DONE

            //  MESSAGE HANDLERS
            Message* manageMessage( Message* message, int socket );                 //  HANDLES MESSAGES AND SENT THEM TO THE CORRECT PROTOCOL
            Message* userManager(Message* message, string username , int socket );  //  HANDLES MESSAGES WHICH INVOLVE ONLY THE USER AND THE SERVER
            Message* matchManager(Message* message, string username );              //  HANDLES MESSAGES WHICH INVOLVE MANY USERS

            //  EVENT HANDLERS
            void logoutClient(int socket);                           //  SECURE DISCONNECTION OF A CLIENT FROM THE SERVER
            Message* sendError( string errorMessage , int* nonce );  //  GENERATES AN ERROR MESSAGE
            bool closeMatch( int matchID );                            //  CLOSES A MATCH AND ADVERTICE PARTICIPANTS
            int generateRandomNonce();                        //  GENERATES A RANDOM NONCE TO BE USED BY CLIENTS

        public:
            MainServer( string ipAddr , int port );                  //  GENERATES THE SERVER
            void server();                                           //  STARTS THE SERVER

    };
}


#endif //FOURINAROW_MAINSERVER_H
