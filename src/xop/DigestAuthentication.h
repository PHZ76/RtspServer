//PHZ
//2019-10-6

#ifndef RTSP_DIGEST_AUTHENTICATION_H
#define RTSP_DIGEST_AUTHENTICATION_H

#include <cstdint>
#include <string>

namespace xop
{

class DigestAuthentication
{
public:
	DigestAuthentication(std::string realm, std::string username, std::string password);
	virtual ~DigestAuthentication();

	std::string GetRealm() const
	{ return m_realm; }

	std::string GetUsername() const
	{ return m_username; }

	std::string GetPassword() const
	{ return m_password; }

	std::string GetNonce();
	std::string GetResponse(std::string nonce, std::string cmd, std::string url);

private:
	std::string m_realm;
	std::string m_username;
	std::string m_password;

};

}

#endif
