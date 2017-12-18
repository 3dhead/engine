/**
 * @file
 */

#include "Connection.h"
#include "ConnectionPool.h"
#include "core/Log.h"
#include "core/Singleton.h"
#include "engine-config.h"
#ifdef HAVE_POSTGRES
#include <libpq-fe.h>
#endif

namespace persistence {

Connection::Connection() :
		_connection(nullptr), _port(0) {
}

Connection::~Connection() {
}

void Connection::setLoginData(const std::string& username, const std::string& password) {
	_password = password;
	_user = username;
}

bool Connection::status() const {
	if (_connection == nullptr) {
		return false;
	}
#ifdef HAVE_POSTGRES
	return PQstatus(_connection) == CONNECTION_OK;
#else
	return false;
#endif
}

void Connection::changeDb(const std::string& dbname) {
	_dbname = dbname;
}

void Connection::changeHost(const std::string& host) {
	_host = host;
}

void Connection::changePort(uint16_t port) {
	_port = port;
}

#ifdef HAVE_POSTGRES
static void defaultNoticeProcessor(void *arg, const char *message) {
	Log::debug("Notice processor: '%s'", message);
}
#endif

bool Connection::connect() {
	if (status()) {
		return true;
	}
	std::string conninfo;

	const char *host = nullptr;
	if (!_host.empty()) {
		host = _host.c_str();
	}

	const char *dbname = nullptr;
	if (!_dbname.empty()) {
		dbname = _dbname.c_str();
	}

	const char *user = nullptr;
	if (!_user.empty()) {
		user = _user.c_str();
	}

	const char *password = nullptr;
	if (!_password.empty()) {
		password = _password.c_str();
	}

	std::string port;
	if (_port > 0) {
		port = std::to_string(_port);
	}

#ifdef HAVE_POSTGRES
	_connection = PQsetdbLogin(host, port.empty() ? nullptr : port.c_str(), conninfo.c_str(), nullptr, dbname, user, password);
#else
	_connection = nullptr;
#endif
	Log::debug("Connect %p", _connection);
	if (!status()) {
#ifdef HAVE_POSTGRES
		Log::error("Connection to database failed: '%s'", PQerrorMessage(_connection));
#endif
		disconnect();
		return false;
	}

	_preparedStatements.clear();

#ifdef HAVE_POSTGRES
	PQsetNoticeProcessor(_connection, defaultNoticeProcessor, nullptr);
	PQexec(_connection, "SET TIME ZONE 'UTC';CREATE EXTENSION pgcrypto;");
#endif
	return true;
}

void Connection::disconnect() {
	if (_connection != nullptr) {
		Log::debug("Disconnect %p", _connection);
#ifdef HAVE_POSTGRES
		PQfinish(_connection);
#endif
		_connection = nullptr;
	}
	_preparedStatements.clear();
}

void Connection::close() {
	core::Singleton<ConnectionPool>::getInstance().giveBack(this);
}

}
