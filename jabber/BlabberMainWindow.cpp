//////////////////////////////////////////////////
// Blabber [BlabberMainWindow.cpp]
//////////////////////////////////////////////////

#include "BlabberMainWindow.h"

#include <InterfaceKit.h>
#include <be_apps/NetPositive/NetPositive.h>
#include "AppLocation.h"
#include "BlabberSettings.h"
#include "BuddyWindow.h"
#include "BuddyInfoWindow.h"
#include "ChangeNameWindow.h"
#include "CustomStatusWindow.h"
#include "GenericFunctions.h"
#include "JabberSpeak.h"
#include "Messages.h"
#include "MessageRepeater.h"
#include "ModalAlertFactory.h"
#include "PreferencesWindow.h"
#include "RosterItem.h"
#include "RotateChatFilter.h"
#include "SendTalkWindow.h"
#include "TalkManager.h"

#include <Application.h>
#include <CardLayout.h>
#include <Deskbar.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>
#include <ScrollView.h>
#include <String.h>

#include <algorithm>
#include <cstdio>
#include <stdlib.h>


BlabberMainWindow *BlabberMainWindow::_instance = NULL;

BlabberMainWindow *BlabberMainWindow::Instance() {
	BlabberSettings *settings = BlabberSettings::Instance();
	
	if (_instance == NULL && !settings->Data("no-window-on-startup")) {
		float main_window_width, main_window_height;

		// determine what the width and height of the window should be
		if (settings->Data("main-window-width") && settings->Data("main-window-height")) {
			main_window_width  = atof(settings->Data("main-window-width"));
			main_window_height = atof(settings->Data("main-window-height"));
		} else {
			// default
			main_window_width  = 210;
			main_window_height = 332; 
		}
		
		// create window frame position
		BRect frame(GenericFunctions::CenteredFrame(main_window_width, main_window_height));

		// poition window to last known position
		if (settings->Data("main-window-left") && settings->Data("main-window-top")) {
			frame.OffsetTo(BPoint(atof(settings->Data("main-window-left")), atof(settings->Data("main-window-top"))));
		}

		// create window singleton
		_instance = new BlabberMainWindow(frame);
	}
	
	return _instance;
}

BlabberMainWindow::~BlabberMainWindow() {
	// remove self from message family
	MessageRepeater::Instance()->RemoveTarget(this);

	// remove deskbar icon
//	BDeskbar db;
//	db.RemoveItem(_deskbar_id);	

	_instance = NULL;
}

void BlabberMainWindow::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		// channels
		case JAB_A_CHANNEL: {
			if (!BlabberSettings::Instance()->Data("channel-name")) {
				ModalAlertFactory::Alert("Before you may use channels, you must select a channel name under the Messages/Chat section of Preferences.", "OK");
				break;
			}
			
			BString fChannelName;
			if (msg->FindString("channel",&fChannelName) == B_OK)
			{
				//TODO ValidateRoom (see SendTalkWindow.cpp)
				TalkManager::Instance()->CreateTalkSession(gloox::Message::Groupchat, NULL, fChannelName.String(), BlabberSettings::Instance()->Data("channel-name"));
			}
			
			break;
		}

		// transplanted from LoginWindow
		case JAB_LOGIN: {
			// must pass validation
			if (!ValidateLogin()) {
				break;
			}

			// switch out views
			_login_login->MakeDefault(false);
			BCardLayout* cl = (BCardLayout*)GetLayout();
			cl->SetVisibleItem((int32)0);

			// connect with current username or register new account
			JabberSpeak::Instance()->SendConnect(_login_username->Text(), _login_password->Text(), _login_realname->Text(), _login_new_account->Value());

			break;
		}
		
		case B_ABOUT_REQUESTED: {
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;
		}

		case JAB_QUIT: {
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}

		case JAB_PREFERENCES: {
			PreferencesWindow::Instance()->Show();
			PreferencesWindow::Instance()->Activate();
			break;
		}

		case JAB_CONNECT: {
			_status_view->SetMessage("connecting");
			JabberSpeak::Instance()->SendConnect("", "", "", false, true);

			break;
		}

		case JAB_RECONNECTING: {
			// we are connecting
			_status_view->SetMessage("reconnecting");

			break;
		}

		case JAB_LOGGED_IN: {
			// we just logged in
			_status_view->SetMessage("connected as " + JabberSpeak::Instance()->CurrentLogin());

			// save these settings
			BlabberSettings::Instance()->SetData("last-realname", _login_realname->Text());
			BlabberSettings::Instance()->SetData("last-login", _login_username->Text());
			BlabberSettings::Instance()->SetData("last-password", _login_password->Text());
			BlabberSettings::Instance()->SetTag("auto-login", _login_auto_login->Value());
			BlabberSettings::Instance()->WriteToFile();

			break;
		}
		
		case JAB_DISCONNECT: {
			JabberSpeak::Instance()->SendDisconnect();
			_status_view->SetMessage("disconnecting");
			JabberSpeak::Instance()->Reset();

			break;
		}

		case JAB_DISCONNECTED: {
			JRoster::Instance()->RefreshRoster();

			break;
		}

		case JAB_RIV: {
			string jabber_org = "http://www.users.uswest.net/~jblanco"; 
			
			char *argv[] = {const_cast<char *>(jabber_org.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", jabber_org.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case JAB_BE_USERS: {
			string jabber_org = "http://home.t-online.de/home/sascha.offe/jabber.html"; 
			
			char *argv[] = {const_cast<char *>(jabber_org.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", jabber_org.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}
				
		case JAB_JABBER_ORG: {
			string jabber_org = "http://www.jabber.org"; 
			
			char *argv[] = {const_cast<char *>(jabber_org.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", jabber_org.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case JAB_JABBER_CENTRAL_ORG: {
			string jabber_org = "http://www.jabbercentral.org"; 
			
			char *argv[] = {const_cast<char *>(jabber_org.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", jabber_org.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case JAB_JABBER_VIEW_COM: {
			string jabber_org = "http://www.jabberview.com"; 
			
			char *argv[] = {const_cast<char *>(jabber_org.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", jabber_org.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case JAB_FAQ: {
			string user_guide = "http://www.users.uswest.net/~jblanco/jabber-faq.html";
			
			char *argv[] = {const_cast<char *>(user_guide.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", user_guide.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case JAB_USER_GUIDE: {
			string user_guide = AppLocation::Instance()->AbsolutePath("resources/user-guide/user_guide.html");
			
			char *argv[] = {const_cast<char *>(user_guide.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", user_guide.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case BLAB_UPDATE_ROSTER: {
			// a message that the roster singleton was updated
			_roster->UpdateRoster();
	
			break;
		}

		case JAB_SUBSCRIBE_PRESENCE: {
			RosterItem *item = _roster->CurrentItemSelection();
			
			// if there was a user selected (shouldn't be otherwise)
			if (item) {
				// don't send subscription request if we have it already
				if (!item->GetUserID()->HaveSubscriptionTo()) {
					JabberSpeak::Instance()->SendSubscriptionRequest(item->GetUserID()->JabberHandle());
				}
			}
			
			break;
		}

		case JAB_UNSUBSCRIBE_PRESENCE: {
			RosterItem *item = _roster->CurrentItemSelection();
			
			// if there was a user selected (shouldn't be otherwise)
			if (item) {
				// don't send unsubscription request if we don't have it already
				if (item->GetUserID()->HaveSubscriptionTo()) {
					JabberSpeak::Instance()->SendUnsubscriptionRequest(item->GetUserID()->Handle());
				}
			}
			
			break;
		}

		case JAB_OPEN_CHAT_WITH_DOUBLE_CLICK: {
			// works in combination with JAB_OPEN_CHAT case
			if (!BlabberSettings::Instance()->Tag("enable-double-click")) {
				break;
			}
		}
		__attribute__ ((fallthrough));
		
		case JAB_OPEN_CHAT: {
			// if there's a current selection, begin chat with that user
			RosterItem *item = _roster->CurrentItemSelection();
			
			if (item != NULL) {
				const UserID *user = item->GetUserID();

				// open chat window
				TalkManager::Instance()->CreateTalkSession(gloox::Message::Chat, new UserID(*user), "", "");
			}
			
			break;
		}

		case JAB_SHOW_CHATLOG: {
			BString path;
			// try to get the chatlog path from message
			if(B_OK != msg->FindString("path", &path)) {
				// no path provided with message? try to open history for selected user...
  				RosterItem *item = _roster->CurrentItemSelection();
				if (item != NULL) {
					const UserID *user = item->GetUserID();
					string chatlog_path = BlabberSettings::Instance()->Data("chatlog-path");
					if(0 == chatlog_path.size()) {
						BPath path;
						find_directory(B_USER_DIRECTORY, &path);
						chatlog_path = path.Path();
					}
					chatlog_path += "/" + user->JabberHandle();
					path = chatlog_path.c_str();
				}
			}
			// now attemt to open history file with default application for "text/x-jabber-chatlog" MIME type
			if(path.Length() > 0) {
				BEntry entry(path.String());
				entry_ref ref;
				entry.GetRef(&ref);
				BMessage *msgRefs = new BMessage(B_REFS_RECEIVED);
				msgRefs->AddRef("refs", &ref);
				be_roster->Launch("text/plain", msgRefs);
				//be_roster->Launch(path.String());
			}
			break;
		}

		case JAB_OPEN_NEW_CHAT: {
			(new SendTalkWindow(gloox::Message::Chat))->Show();
			break;
		}

		case JAB_OPEN_NEW_GROUP_CHAT: {
			(new SendTalkWindow(gloox::Message::Groupchat))->Show();
			break;
		}

		case JAB_OPEN_MESSAGE: {
			// if there's a current selection, begin message with that user
			RosterItem *item = _roster->CurrentItemSelection();

			const UserID *user = item->GetUserID();

			// open message window
			TalkManager::Instance()->CreateTalkSession(gloox::Message::Normal, new UserID(*user), "", "");
			
			break;
		}

		case JAB_OPEN_NEW_MESSAGE: {
			(new SendTalkWindow(gloox::Message::Normal))->Show();
			break;
		}

		case JAB_OPEN_ADD_BUDDY_WINDOW: {
			// open buddy window
			BuddyWindow::Instance()->Show();
			break;
		}

		case JAB_OPEN_EDIT_BUDDY_WINDOW: {
			// pick out user to be removed from RosterView
			RosterItem *item = _roster->CurrentItemSelection();
			
			if (item != NULL) {
				// pick out user
				UserID *user = const_cast<UserID *>(item->GetUserID());

				// open edit buddy window
				(new ChangeNameWindow(user))->Show();
			}

			break;
		}

		case JAB_USER_INFO: {
			// pick out user to be analyzed
			RosterItem *item = _roster->CurrentItemSelection();
			
			if (item != NULL) {
				// pick out user
				UserID *user = const_cast<UserID *>(item->GetUserID());

				// open edit buddy window
				(new BuddyInfoWindow(user))->Show();
			}

			break;
		}

		case JAB_REMOVE_BUDDY: {
			// pick out user to be removed from RosterView
			RosterItem *item = _roster->CurrentItemSelection();
			
			if (item != NULL) {
				// pick out user
				const UserID *user = item->GetUserID();

				// remove the user
				JabberSpeak::Instance()->RemoveFromRoster(user);
			}

			break;
		}

		case BLAB_AVAILABLE_FOR_CHAT: {
			JabberSpeak::Instance()->SendPresence(gloox::Presence::Chat);
			_chat_item->SetMarked(true);
			BlabberSettings::Instance()->SetTag("last-used-custom-status", false);
			BlabberSettings::Instance()->SetData("last-custom-exact-status", "chat");
			BlabberSettings::Instance()->WriteToFile();

			break;
		}

		case BLAB_DO_NOT_DISTURB: {
			JabberSpeak::Instance()->SendPresence(gloox::Presence::DND);
			_dnd_item->SetMarked(true);
			BlabberSettings::Instance()->SetTag("last-used-custom-status", false);
			BlabberSettings::Instance()->SetData("last-custom-exact-status", "dnd");
			BlabberSettings::Instance()->WriteToFile();

			break;
		}

		case BLAB_AWAY_TEMPORARILY: {
			JabberSpeak::Instance()->SendPresence(gloox::Presence::Away);
			_away_item->SetMarked(true);
			BlabberSettings::Instance()->SetTag("last-used-custom-status", false);
			BlabberSettings::Instance()->SetData("last-custom-exact-status", "away");
			BlabberSettings::Instance()->WriteToFile();
			
			break;
		}

		case BLAB_AWAY_EXTENDED: {
			JabberSpeak::Instance()->SendPresence(gloox::Presence::XA);
			_xa_item->SetMarked(true);
			BlabberSettings::Instance()->SetTag("last-used-custom-status", false);
			BlabberSettings::Instance()->SetData("last-custom-exact-status", "xa");
			BlabberSettings::Instance()->WriteToFile();

			break;
		}

		case BLAB_SCHOOL: {
			JabberSpeak::Instance()->SendPresence(gloox::Presence::XA, "Off to School");
			_school_item->SetMarked(true);
			BlabberSettings::Instance()->SetTag("last-used-custom-status", false);
			BlabberSettings::Instance()->SetData("last-custom-exact-status", "school");
			BlabberSettings::Instance()->WriteToFile();

			break;
		}

		case BLAB_WORK: {
			JabberSpeak::Instance()->SendPresence(gloox::Presence::XA, "Busy at Work");
			_work_item->SetMarked(true);
			BlabberSettings::Instance()->SetTag("last-used-custom-status", false);
			BlabberSettings::Instance()->SetData("last-custom-exact-status", "work");
			BlabberSettings::Instance()->WriteToFile();

			break;
		}

		case BLAB_LUNCH: {
			JabberSpeak::Instance()->SendPresence(gloox::Presence::Away, "Lunch");
			_lunch_item->SetMarked(true);
			BlabberSettings::Instance()->SetTag("last-used-custom-status", false);
			BlabberSettings::Instance()->SetData("last-custom-exact-status", "lunch");
			BlabberSettings::Instance()->WriteToFile();

			break;
		}

		case BLAB_DINNER: {
			JabberSpeak::Instance()->SendPresence(gloox::Presence::Away, "Dinner");
			_dinner_item->SetMarked(true);
			BlabberSettings::Instance()->SetTag("last-used-custom-status", false);
			BlabberSettings::Instance()->SetData("last-custom-exact-status", "dinner");
			BlabberSettings::Instance()->WriteToFile();

			break;
		}

		case BLAB_SLEEP: {
			JabberSpeak::Instance()->SendPresence(gloox::Presence::XA, "Sleeping");
			_sleep_item->SetMarked(true);
			BlabberSettings::Instance()->SetTag("last-used-custom-status", false);
			BlabberSettings::Instance()->SetData("last-custom-exact-status", "sleep");
			BlabberSettings::Instance()->WriteToFile();

			break;
		}

		case BLAB_CUSTOM_STATUS: {
			CustomStatusWindow::Instance()->Show();
			CustomStatusWindow::Instance()->Activate();
			break;
		}
		
#if 0
		case PortTalker::COULD_NOT_RESOLVE_HOSTNAME: {
			char buffer[1024];
			
			sprintf(buffer, "The hostname '%s' couldn't be reached.  Please re-check the name and try again. There could be a networking problem as well.", msg->FindString("hostname"));
			ModalAlertFactory::Alert(buffer, "Doh!");

			JabberSpeak::Instance()->Reset();
			ShowLogin();
			
			break;
		}
#endif

		case JAB_ROTATE_CHAT_FORWARD: {
			TalkManager::Instance()->RotateToNextWindow(NULL, TalkManager::ROTATE_FORWARD);
			break;
		}

		case JAB_ROTATE_CHAT_BACKWARD: {
			TalkManager::Instance()->RotateToNextWindow(NULL, TalkManager::ROTATE_BACKWARD);
			break;
		}
		break;
	}
}

void BlabberMainWindow::MenusBeginning() {
	char buffer[1024];

	// FILE menu
	if (!_full_view->IsHidden()) {
		_connect_item->SetEnabled(false);
		_disconnect_item->SetEnabled(true);
	} else {
		_connect_item->SetEnabled(true);
		_disconnect_item->SetEnabled(false);
	}

	// EDIT menu
	if (RosterItem *item = _roster->CurrentItemSelection()) {
		// if a  item is selected
		sprintf(buffer, "Edit %s", item->GetUserID()->FriendlyName().c_str());
		_change_buddy_item->SetLabel(buffer);
		_change_buddy_item->SetEnabled(true);

		sprintf(buffer, "Remove %s", item->GetUserID()->FriendlyName().c_str());
		_remove_buddy_item->SetLabel(buffer);
		_remove_buddy_item->SetEnabled(true);

		_user_info_item->SetEnabled(true);
		_user_chatlog_item->SetEnabled(BlabberSettings::Instance()->Tag("autoopen-chatlog"));
	} else {		
		sprintf(buffer, "Edit Buddy");
		_change_buddy_item->SetLabel(buffer);
		_change_buddy_item->SetEnabled(false);

		sprintf(buffer, "Remove Buddy");
		_remove_buddy_item->SetLabel(buffer);
		_remove_buddy_item->SetEnabled(false);

		_user_info_item->SetEnabled(false);
		_user_chatlog_item->SetEnabled(false);
	}
}

bool BlabberMainWindow::QuitRequested() {
	// remember last coordinates
	BlabberSettings::Instance()->SetFloatData("main-window-left", Frame().left);
	BlabberSettings::Instance()->SetFloatData("main-window-top", Frame().top);
	BlabberSettings::Instance()->SetFloatData("main-window-width", Bounds().Width());
	BlabberSettings::Instance()->SetFloatData("main-window-height", Bounds().Height());
	BlabberSettings::Instance()->WriteToFile();

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

BlabberMainWindow::BlabberMainWindow(BRect frame)
	: BWindow(frame, "Renga", B_DOCUMENT_WINDOW, 0) {

	// editing filter for taksing
	AddCommonFilter(new RotateChatFilter(NULL));
	
	// add self to message family
	MessageRepeater::Instance()->AddTarget(this);

	// encompassing view
	_full_view = new BGroupView("main-full", B_VERTICAL);
	_full_view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	// status bar
	_status_view = new StatusView();
	_status_view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	
	// menubar
	_menubar = new BMenuBar("menubar");

	// FILE MENU
	_file_menu = new BMenu("File");

		_connect_item    = new BMenuItem("Log On", new BMessage(JAB_CONNECT));
		_connect_item->SetShortcut('N', 0);

		_disconnect_item = new BMenuItem("Log Off", new BMessage(JAB_DISCONNECT));
		_disconnect_item->SetShortcut('B', 0);

		_about_item      = new BMenuItem("About Renga" B_UTF8_ELLIPSIS, new BMessage(B_ABOUT_REQUESTED));

		_quit_item = new BMenuItem("Quit", new BMessage(JAB_QUIT));
		_quit_item->SetShortcut('Q', 0);

//	_file_menu->AddItem(_connect_item);
	_file_menu->AddItem(_disconnect_item);
	_file_menu->AddSeparatorItem();
	_file_menu->AddItem(_about_item);
	_file_menu->AddSeparatorItem();
	_file_menu->AddItem(_quit_item);
	_file_menu->SetTargetForItems(MessageRepeater::Instance());

	// EDIT MENU
	_edit_menu = new BMenu("Edit");

		_add_buddy_item = new BMenuItem("Add New Buddy", new BMessage(JAB_OPEN_ADD_BUDDY_WINDOW));
		_add_buddy_item->SetShortcut('N', 0);

		_change_buddy_item = new BMenuItem("Edit Buddy", new BMessage(JAB_OPEN_EDIT_BUDDY_WINDOW));
		_change_buddy_item->SetShortcut('E', 0);

		_remove_buddy_item = new BMenuItem("Remove Buddy", new BMessage(JAB_REMOVE_BUDDY));
		_remove_buddy_item->SetShortcut('T', 0);

		_user_info_item = new BMenuItem("Get User Info", new BMessage(JAB_USER_INFO));
		_user_info_item->SetShortcut('I', 0);
		
		_user_chatlog_item = new BMenuItem("Show Chat Log", new BMessage(JAB_SHOW_CHATLOG));
		_user_chatlog_item->SetShortcut('H', 0);
		
		_preferences_item = new BMenuItem("Preferences...", new BMessage(JAB_PREFERENCES));

	_edit_menu->AddItem(_add_buddy_item);
	_edit_menu->AddItem(_change_buddy_item);
	_edit_menu->AddItem(_remove_buddy_item);
	_edit_menu->AddSeparatorItem();
	_edit_menu->AddItem(_user_info_item);
	_edit_menu->AddSeparatorItem();
	_edit_menu->AddItem(_user_chatlog_item);
	_edit_menu->AddSeparatorItem();
	_edit_menu->AddItem(_preferences_item);
	_edit_menu->SetTargetForItems(this);

	// STATUS MENU
	_status_menu = new BMenu("Status");
	
		_chat_item = new BMenuItem("I'm available for chat.", new BMessage(BLAB_AVAILABLE_FOR_CHAT));
		_away_item = new BMenuItem("I will be away temporarily.", new BMessage(BLAB_AWAY_TEMPORARILY));
		_dnd_item = new BMenuItem("Do not disturb me.", new BMessage(BLAB_DO_NOT_DISTURB));
		_xa_item = new BMenuItem("I will be away for an extended time period.", new BMessage(BLAB_AWAY_EXTENDED));
		_school_item = new BMenuItem("Off to School", new BMessage(BLAB_SCHOOL));
		_work_item = new BMenuItem("Busy at Work", new BMessage(BLAB_WORK));
		_lunch_item = new BMenuItem("Lunch", new BMessage(BLAB_LUNCH));
		_dinner_item = new BMenuItem("Dinner", new BMessage(BLAB_DINNER));
		_sleep_item = new BMenuItem("Sleeping", new BMessage(BLAB_SLEEP));
		_custom_item = new BMenuItem("Custom...", new BMessage(BLAB_CUSTOM_STATUS));

	_status_menu->AddItem(_chat_item);
	_status_menu->AddSeparatorItem();
	_status_menu->AddItem(_away_item);
	_status_menu->AddItem(_dnd_item);
	_status_menu->AddItem(_xa_item);
	_status_menu->AddSeparatorItem();

	_status_menu->AddItem(_school_item);
	_status_menu->AddItem(_work_item);
	_status_menu->AddItem(_lunch_item);
	_status_menu->AddItem(_dinner_item);
	_status_menu->AddItem(_sleep_item);
	
	_status_menu->AddSeparatorItem();
	_status_menu->AddItem(_custom_item);

	_status_menu->SetRadioMode(true);
	_chat_item->SetMarked(true);
	
	// TALK MENU
	_talk_menu = new BMenu("Talk");

		_rotate_chat_forward_item = new BMenuItem("Rotate Chat Forward", new BMessage(JAB_ROTATE_CHAT_FORWARD));
		_rotate_chat_forward_item->SetShortcut('.', 0);

		_rotate_chat_backward_item = new BMenuItem("Rotate Chat Backward", new BMessage(JAB_ROTATE_CHAT_BACKWARD));
		_rotate_chat_backward_item->SetShortcut(',', 0);

		_send_message_item = new BMenuItem("Send Message...", new BMessage(JAB_OPEN_NEW_MESSAGE));
		_send_message_item->SetShortcut('M', 0);

		_send_chat_item = new BMenuItem("Start Chat...", new BMessage(JAB_OPEN_NEW_CHAT));
		_send_chat_item->SetShortcut('C', 0);

		_send_groupchat_item = new BMenuItem("Start Group Chat...", new BMessage(JAB_OPEN_NEW_GROUP_CHAT));
		_send_groupchat_item->SetShortcut('G', 0);

	_talk_menu->AddItem(_rotate_chat_forward_item);
	_talk_menu->AddItem(_rotate_chat_backward_item);
	_talk_menu->AddSeparatorItem();
	_talk_menu->AddItem(_send_message_item);
	_talk_menu->AddItem(_send_chat_item);
	_talk_menu->AddItem(_send_groupchat_item);
	_talk_menu->SetTargetForItems(this);

	// CHANNEL MENU
	_channel_menu = new BMenu("Channels");

	BMessage* haiku_channel = new BMessage(JAB_A_CHANNEL);
	haiku_channel->AddString("channel", "haiku-os@conference.jabber.org");
	
	BMenuItem* _a_channel = new BMenuItem("haiku-os", haiku_channel);
	_channel_menu->AddItem(_a_channel);

	_menubar->AddItem(_file_menu);
	_menubar->AddItem(_edit_menu);
	_menubar->AddItem(_status_menu);
	_menubar->AddItem(_talk_menu);
	_menubar->AddItem(_channel_menu);
	//_menubar->AddItem(_help_menu);	

	// tabbed view
	// roster view
	_roster          = new RosterView();
	_roster_scroller = new BScrollView(NULL, _roster, 0, false, true,
		B_NO_BORDER);
	_roster->TargetedByScrollView(_roster_scroller);

	// chat service
	BLayoutBuilder::Group<>(_full_view, B_VERTICAL, 0)
		.SetInsets(0, 0, 0, 0)
		.Add(_menubar)
		.Add(_roster_scroller)
		.Add(_status_view)
	.End();
	
	///// NOW DO LOGIN STUFF
	// encompassing view
	_login_full_view = new BGroupView("login-full", B_VERTICAL);
	_login_full_view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	// graphics
	_login_bulb = new PictureView("jabber-title");

	// username/password controls
	_login_realname = new BTextControl(NULL, "Nickname: ", NULL, NULL);
	
	_login_username = new BTextControl(NULL, "Jabber ID: ", NULL, NULL);
		
	_login_password = new BTextControl(NULL, "Password: ", NULL, NULL);
	_login_password->TextView()->HideTyping(true);
	
	_login_new_account = new BCheckBox(NULL, "Create this account!", NULL);

	_login_auto_login = new BCheckBox(NULL, "Auto-login", NULL);

	// login button
	_login_login = new BButton("login", "Login", new BMessage(JAB_LOGIN));
	_login_login->MakeDefault(false);
	_login_login->SetTarget(this);

	// new user notes
	rgb_color note = ui_color(B_PANEL_TEXT_COLOR);
	BFont black_9(be_plain_font);
	black_9.SetSize(std::min(black_9.Size() * 0.75f, 9.0f));

	BTextView *enter_note = new BetterTextView(NULL, &black_9, &note, B_WILL_DRAW);
	enter_note->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	enter_note->MakeEditable(false);
	enter_note->MakeSelectable(false);
	enter_note->SetAlignment(B_ALIGN_CENTER);
	enter_note->SetWordWrap(true);
	enter_note->SetText("Note: Jabber ID's are of the form username@server.\n"
		"Pick a fun username! If you don't know any servers, jabber.org is recommended.");
	
	BLayoutBuilder::Group<>(_login_full_view, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(_login_bulb)
		.AddGrid()
			.AddTextControl(_login_realname, 0, 0)
			.AddTextControl(_login_username, 0, 1)
			.AddTextControl(_login_password, 0, 2)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(_login_new_account)
			.Add(_login_auto_login)
		.End()
		.Add(_login_login)
		.Add(enter_note)
	.End();
	
	// attach all-encompassing main view to window
	BCardLayout* cl = new BCardLayout();
	SetLayout(cl);
	AddChild(_full_view);
	AddChild(_login_full_view);

	// login always hidden at start
	cl->SetVisibleItem((int32)0);

	// default
	if(BlabberSettings::Instance()->Data("last-realname")) {
		_login_realname->SetText(BlabberSettings::Instance()->Data("last-realname"));
	} else {
		_login_realname->SetText("Me");
	}
	
	if (BlabberSettings::Instance()->Data("last-login")) {
		_login_username->SetText(BlabberSettings::Instance()->Data("last-login"));
	} else {
		_login_username->SetText("newuser@jabber.org");
	}
	
	_login_password->SetText(BlabberSettings::Instance()->Data("last-password"));

	_login_auto_login->SetValue(BlabberSettings::Instance()->Tag("auto-login"));

	// focus to start
	if (BlabberSettings::Instance()->Data("last-login")) {
		_login_password->MakeFocus(true);
	} else {
		_login_username->MakeFocus(true);
	}
}

bool BlabberMainWindow::ValidateLogin() {
	// existance of username
	if (!strcmp(_login_username->Text(), "")) {
		ModalAlertFactory::Alert("Wait, you haven't specified your Jabber ID yet.\n(e.g. haikuFan@jabber.org)", "Doh!", NULL, NULL, B_WIDTH_FROM_LABEL, B_STOP_ALERT);
		_login_username->MakeFocus(true);

		return false;
	}

	// validity of username
	UserID username(gloox::JID(_login_username->Text()));

	if (username.WhyNotValidJabberHandle().size()) {
		char buffer[1024];
		
		if (_login_new_account->Value())
			sprintf(buffer, "The Jabber ID you specified is not legal for the following reason:\n\n%s\n\nYou must specify a legal Jabber ID before you may create a new account.", username.WhyNotValidJabberHandle().c_str());
		else
			sprintf(buffer, "The Jabber ID you specified must not be yours because it's invalid for the following reason:\n\n%s\n\nIf you can't remember it, it's OK to create a new one by checking the \"Create a new Jabber Account!\" box.", username.WhyNotValidJabberHandle().c_str());

		ModalAlertFactory::Alert(buffer, "OK", NULL, NULL, B_WIDTH_FROM_LABEL, B_STOP_ALERT);
		_login_username->MakeFocus(true);

		return false;
	}	

	// append default resource if missing
	if (username.JabberResource().empty()) {
		string handle     = username.Handle();
		string resource   = "/haiku";
		string new_handle = handle + resource;

		_login_username->SetText(new_handle.c_str());
	}
	
	// 	existance of password
	if (!strcmp(_login_password->Text(), "")) {
		char buffer[1024];

		if (_login_new_account->Value())
			sprintf(buffer, "To create a new account, you must specify a password to protect it, %s.", username.Handle().c_str());
		else
			sprintf(buffer, "You must specify a password so I can make sure it's you, %s.", username.Handle().c_str());

		ModalAlertFactory::Alert(buffer, "Sorry!", NULL, NULL, B_WIDTH_FROM_LABEL, B_STOP_ALERT);
		_login_password->MakeFocus(true);

		return false;
	}

	return true;
}

void BlabberMainWindow::ShowLogin() {
	// reassign button as default
	_login_login->MakeDefault(true);

	// reset
	_login_realname->SetText("");
	_login_username->SetText("");
	_login_password->SetText("");

	// default
	if(BlabberSettings::Instance()->Data("last-realname")) {
		_login_realname->SetText(BlabberSettings::Instance()->Data("last-realname"));
	} else {
		_login_realname->SetText("Me");
	}
	
	if (BlabberSettings::Instance()->Data("last-login")) {
		_login_username->SetText(BlabberSettings::Instance()->Data("last-login"));
	} else {
		_login_username->SetText("newuser@jabber.org");
	}

	_login_password->SetText(BlabberSettings::Instance()->Data("last-password"));

	// focus to start
	if (BlabberSettings::Instance()->Data("last-login")) {
		_login_password->MakeFocus(true);
	} else {
		_login_username->MakeFocus(true);
	}

	BCardLayout* cl = (BCardLayout*)GetLayout();
	cl->SetVisibleItem(1);
}

void BlabberMainWindow::SetCustomStatus(string status) {
	char buffer[2048];
	
	// create menued status message
	sprintf(buffer, "[Custom] %s", status.c_str()); 

	_custom_item->SetMarked(true);
	_custom_item->SetLabel(buffer);
}
