/**
 * @file
 */

#pragma once

#include "network/IMsgProtocolHandler.h"
#include "backend/entity/User.h"
#include "ServerMessages_generated.h"
#include "ClientMessages_generated.h"

using namespace network;

namespace backend {

template<class MSGTYPE>
class IUserProtocolHandler: public network::IMsgProtocolHandler<MSGTYPE, User> {
public:
	IUserProtocolHandler() :
		network::IMsgProtocolHandler<MSGTYPE, User>(true) {
	}

	virtual ~IUserProtocolHandler() {
	}

	virtual void execute(User* attachment, const MSGTYPE* message) override = 0;
};

#define USERPROTOHANDLER(msgType) \
struct msgType##Handler: public IUserProtocolHandler<msgType> { \
	msgType##Handler() : IUserProtocolHandler<msgType>() {} \
	void execute(User* attachment, const msgType* message) override; \
}

#define USERPROTOHANDLERIMPL(msgType) \
USERPROTOHANDLER(msgType); \
inline void msgType##Handler::execute(User* user, const msgType* message)

}
