//////////////////////////////////////////////////
// Blabber [UserID.h]
//     Encapsulates user data for any account,
//     Jabber or otherwise.
//////////////////////////////////////////////////

#ifndef USER_ID_H
#define USER_ID_H

#include <gloox/gloox.h>
#include <gloox/jid.h>

#include <string>

class UserID {
public:
	enum online_status  {UNKNOWN, UNACCEPTED, OFFLINE, ONLINE, TRANSPORT_ONLINE};
	enum user_type      {INVALID, JABBER, AIM, ICQ, YAHOO, MSN, TRANSPORT};

public:
          	             UserID(gloox::JID username);
          	             UserID(const UserID &copied_userid);
	virtual             ~UserID();

	UserID              &operator=(const UserID &rhs);

	      user_type      UserType() const;
	const std::string    Handle() const;
	const std::string    FriendlyName() const;

	      online_status  OnlineStatus() const;
	const std::string    ExactOnlineStatus() const;
	const std::string    MoreExactOnlineStatus() const;
	gloox::SubscriptionType SubscriptionStatus() const;

	bool                 HaveSubscriptionTo() const;
	bool                 IsUser() const;

	// Jabber
	const std::string    JabberHandle() const;         // xxx@yyy
	const std::string    JabberCompleteHandle() const; // xxx@yyy/zzz
	const std::string    JabberUsername() const;       // xxx
	const std::string    JabberServer() const;         // yyy
	const std::string    JabberResource() const;       // zzz

	const std::string    TransportID() const;
	const std::string    TransportUsername() const;
	const std::string    TransportPassword() const;

	void                 StripJabberResource();        // xxx@yyy/zzz -> xxx@yyy
	std::string          WhyNotValidJabberHandle();

	// MANIPULATORS
	void                 SetHandle(gloox::JID handle);
	void                 SetFriendlyName(std::string friendly_name);

	void                 SetOnlineStatus(online_status status);
	void                 SetExactOnlineStatus(std::string exact_status);
	void                 SetMoreExactOnlineStatus(std::string exact_status);
	void                 SetSubscriptionStatus(gloox::SubscriptionType status);

private:
	void                 _ProcessHandle();

	// identification
	gloox::JID           _handle;
	std::string          _friendly_name;
	std::string          _service;

	online_status        _status;
	std::string          _exact_status;
	std::string          _more_exact_status;
	gloox::SubscriptionType _subscription_status;

	// split into pieces
	user_type            _user_type;

	std::string          _jabber_username;
	std::string          _jabber_server;
	std::string          _jabber_resource;

	std::string          _transport_id;
	std::string          _transport_username;
	std::string          _transport_password;
};

#endif
