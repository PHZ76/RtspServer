#include "DigestAuthentication.h"
#include "md5/md5.hpp" 

using namespace xop;

DigestAuthentication::DigestAuthentication(std::string realm, std::string username, std::string password)
	: realm_(realm)
	, username_(username)
	, password_(password)
{

}

DigestAuthentication::~DigestAuthentication()
{

}

std::string DigestAuthentication::GetNonce()
{
	return md5::generate_nonce();
}

std::string DigestAuthentication::GetResponse(std::string nonce, std::string cmd, std::string url)
{
	//md5(md5(<username>:<realm> : <password>) :<nonce> : md5(<cmd>:<url>))

	auto hex1 = md5::md5_hash_hex(username_ + ":" + realm_ + ":" + password_);
	auto hex2 = md5::md5_hash_hex(cmd + ":" + url);
	auto response = md5::md5_hash_hex(hex1 + ":" + nonce + ":" + hex2);
	return response;
}

