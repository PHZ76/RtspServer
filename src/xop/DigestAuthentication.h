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
	~DigestAuthentication();

	std::string getRealm() const
	{
		return m_realm;
	}

	std::string getUsername() const
	{
		return m_username;
	}

	std::string getPassword() const
	{
		return m_password;
	}

	std::string getNonce();
	std::string getResponse(std::string nonce, std::string cmd, std::string url);

private:
	std::string m_realm;
	std::string m_username;
	std::string m_password;

};

}

#endif
