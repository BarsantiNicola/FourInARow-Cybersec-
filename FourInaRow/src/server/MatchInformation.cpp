
#include "MatchInformation.h"

namespace server{

    MatchInformation::MatchInformation( int matchID , string challenger, string challenged ){

        //utility::Information( matchID );
        this->matchID = matchID;
        this->challenger = challenger;
        this->challenged = challenged;
        this->status = OPEN;

    }

    int MatchInformation::getMatchID() {
        return this->matchID;
    }

    string MatchInformation::getChallenger(){
        return this->challenger;
    }

    string MatchInformation::getChallenged(){
        return this->challenged;
    }

    MatchStatus MatchInformation::getStatus(){
        return this->status;
    }

    void MatchInformation::setStatus( MatchStatus status ){
        this->status = status;
    }

}