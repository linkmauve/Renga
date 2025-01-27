//////////////////////////////////////////////////
// Blabber [RosterView.cpp]
//////////////////////////////////////////////////

#ifndef ROSTER_VIEW_H
	#include "RosterView.h"
#endif

#ifndef _CSTDIO
	#include <cstdio>
#endif

#ifndef _MENU_ITEM_H
	#include <MenuItem.h>
#endif

#ifndef BLABBER_SETTINGS_H
	#include "BlabberSettings.h"
#endif

#ifndef BUDDY_INFO_WINDOW_H
	#include "BuddyInfoWindow.h"
#endif

#ifndef MESSAGES_H
	#include "Messages.h"
#endif

#ifndef MODAL_ALERT_FACTORY_H
	#include "ModalAlertFactory.h"
#endif

#ifndef SOUND_SYSTEM_H
	#include "SoundSystem.h"
#endif

#include <strings.h>

RosterView::RosterView()
	: BOutlineListView(NULL, B_SINGLE_SELECTION_LIST) {
}

RosterView::~RosterView() {
	delete _popup;

	// remember superitem statuses
	BlabberSettings::Instance()->SetTag("online-collapsed", !_online->IsExpanded());
	BlabberSettings::Instance()->SetTag("unaccepted-collapsed", !_unaccepted->IsExpanded());
	BlabberSettings::Instance()->SetTag("unknown-collapsed", !_unknown->IsExpanded());
	BlabberSettings::Instance()->SetTag("offline-collapsed", !_offline->IsExpanded());
	BlabberSettings::Instance()->SetTag("transports-collapsed", !_transports->IsExpanded());
	BlabberSettings::Instance()->WriteToFile();
}

int RosterView::ListComparison(const void *a, const void *b) {
	const char *str_a = (*(RosterItem **)a)->Text();
	const char *str_b = (*(RosterItem **)b)->Text();

	return strcasecmp(str_a, str_b);
}   

void RosterView::AttachedToWindow() {
	// superclass call	
	BOutlineListView::AttachedToWindow();

	// on double-click
	SetInvocationMessage(new BMessage(JAB_OPEN_CHAT_WITH_DOUBLE_CLICK));

	// popup menu
	_popup = new BPopUpMenu(NULL, false, false);

		_chat_item         = new BMenuItem("Chat...", new BMessage(JAB_OPEN_CHAT));
		_message_item      = new BMenuItem("Send Message...", new BMessage(JAB_OPEN_MESSAGE));
		_change_user_item  = new BMenuItem("Edit Buddy", new BMessage(JAB_OPEN_EDIT_BUDDY_WINDOW));
		_remove_user_item  = new BMenuItem("Remove Buddy", new BMessage(JAB_REMOVE_BUDDY));
		_user_info_item    = new BMenuItem("Get User Info", new BMessage(JAB_USER_INFO));
		_user_chatlog_item = new BMenuItem("Show Chat Log", new BMessage(JAB_SHOW_CHATLOG));

		_presence          = new BMenu("Presence");
			
			_subscribe_presence   = new BMenuItem("Subscribe", new BMessage(JAB_SUBSCRIBE_PRESENCE));
			_unsubscribe_presence = new BMenuItem("Unsubscribe", new BMessage(JAB_UNSUBSCRIBE_PRESENCE));

			_presence->AddItem(_subscribe_presence);
			_presence->AddItem(_unsubscribe_presence);

	_popup->AddItem(_chat_item);
	_popup->AddItem(_message_item);
	_popup->AddSeparatorItem();
	_popup->AddItem(_change_user_item);
	_popup->AddItem(_remove_user_item);
	_popup->AddSeparatorItem();
	_popup->AddItem(_user_info_item);
	_popup->AddSeparatorItem();
	_popup->AddItem(_user_chatlog_item);
	_popup->AddSeparatorItem();
	_popup->AddItem(_presence);

	// initialize menu
	UpdatePopUpMenu();

	// create top level lists
	AddItem(_online  = new RosterSuperitem("Online"));
	AddItem(_unaccepted = new RosterSuperitem("Pending Presence"));
	AddItem(_unknown = new RosterSuperitem("No Presence"));
	AddItem(_offline = new RosterSuperitem("Offline"));
	AddItem(_transports = new RosterSuperitem("Live Transports"));
	
	// make maps (BUGBUG better way to do two-way map?)
	_item_to_status_map[_offline] = UserID::OFFLINE;
	_item_to_status_map[_online]  = UserID::ONLINE;
	_item_to_status_map[_unknown] = UserID::UNKNOWN;
	_item_to_status_map[_unaccepted] = UserID::UNACCEPTED;
	_item_to_status_map[_transports] = UserID::TRANSPORT_ONLINE;

	// ignore online...doesn't seem to work...?
	_offline->SetExpanded(!BlabberSettings::Instance()->Tag("offline-collapsed"));
	_unknown->SetExpanded(!BlabberSettings::Instance()->Tag("unknown-collapsed"));
	_unaccepted->SetExpanded(!BlabberSettings::Instance()->Tag("unaccepted-collapsed"));
	_transports->SetExpanded(!BlabberSettings::Instance()->Tag("transports-collapsed"));

	_status_to_item_map[UserID::OFFLINE] = _offline;
	_status_to_item_map[UserID::ONLINE]  = _online;
	_status_to_item_map[UserID::UNKNOWN] = _unknown;
	_status_to_item_map[UserID::TRANSPORT_ONLINE] = _transports;
	_status_to_item_map[UserID::UNACCEPTED] = _unaccepted;
	
	// BUGBUG events
	_presence->SetTargetForItems(Window());
	_popup->SetTargetForItems(Window());
}

RosterItem *RosterView::CurrentItemSelection() {
	int32 index = CurrentSelection();
	
	if (index >= 0) {
		return dynamic_cast<RosterItem *>(ItemAt(index));
	} else {
		return NULL;
	}
}

void RosterView::MouseDown(BPoint point) {
	// accept first click
	Window()->Activate(true);

	// get mouse info before it's too late!
	uint32 buttons = 0;
	GetMouse(&point, &buttons, true);

	// superclass stuff
	BOutlineListView::MouseDown(point);

	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		// update menu before presentation
		UpdatePopUpMenu();
		
		BPoint screen_point(point);
		ConvertToScreen(&screen_point);
		
		BRect r(screen_point.x - 4, screen_point.y - 20, screen_point.x + 24, screen_point.y + 4);
		_popup->Go(screen_point, true, true, r, false);
		//_popup->Go(screen_point, true, true, false);
	}
}

void RosterView::RemoveSelected() {
	if (CurrentItemSelection()) {
		// numeric and object based selections
		int32       selected = CurrentSelection();
		RosterItem *item     = CurrentItemSelection();
		
		if (item == NULL) {
			// not a roster item, won't remove
			return;
		}
		
		// remove item from view
		RemoveItem(CurrentSelection());

		// select next buddy for continuity
		if (ItemAt(selected))
			Select(selected);
		else if (ItemAt(selected - 1))
			Select(selected - 1);
	}
}

void RosterView::SelectionChanged() {
	// customize popup menu
	UpdatePopUpMenu();
	
	BOutlineListView::SelectionChanged();
}

void RosterView::LinkUser(const UserID *added_user) {
	AddUnder(new RosterItem(added_user), _offline);
}

void RosterView::LinkTransport(const UserID *added_transport) {
	AddUnder(new TransportItem(added_transport), _transports);
}

void RosterView::UnlinkUser(const UserID *removed_user) {
	// does user exist
	int32 index = FindUser(removed_user);
	
	if (index >= 0) {
		RemoveItem(index);	
	}
}

void RosterView::UnlinkTransport(const UserID *removed_transport) {
	// does transport exist
	int32 index = FindTransport(removed_transport);
	
	if (index >= 0) {
		RemoveItem(index);	
	}
}

int32 RosterView::FindUser(const UserID *compare_user) {
	// handle NULL argument
	if (compare_user == NULL) {
		return -1;
	}

	for (int i=0; i<FullListCountItems(); ++i) {
		// get item
		RosterItem *item = dynamic_cast<RosterItem *>(FullListItemAt(i));

		if (item == NULL || item->StalePointer()) {
			continue;
		}
				
		// compare against RosterView
		if (item->GetUserID() == compare_user) {
			return i;
		}
	}

	// no match
	return -1;
}

int32 RosterView::FindTransport(const UserID *compare_transport) {
	// handle NULL argument
	if (compare_transport == NULL) {
		return -1;
	}

	for (int i=0; i<FullListCountItems(); ++i) {
		// get item
		TransportItem *item = dynamic_cast<TransportItem *>(FullListItemAt(i));

		if (item == NULL) {
			continue;
		}
				
		// compare against RosterView
		if (item->GetUserID() == compare_transport) {
			return i;
		}
	}

	// no match
	return -1;
}

void RosterView::UpdatePopUpMenu() {
	char buffer[1024];

	RosterItem *item = CurrentItemSelection();

	if (item && !item->StalePointer()) {
		const UserID *user = item->GetUserID();

		// if an item is selected
		_chat_item->SetEnabled(true);
		_message_item->SetEnabled(true);

		sprintf(buffer, "Edit %s", item->GetUserID()->FriendlyName().c_str());
		_change_user_item->SetLabel(buffer);
		_change_user_item->SetEnabled(true);

		sprintf(buffer, "Remove %s", item->GetUserID()->FriendlyName().c_str());
		_remove_user_item->SetLabel(buffer);
		_remove_user_item->SetEnabled(true);

		_user_info_item->SetEnabled(true);
		_user_chatlog_item->SetEnabled(BlabberSettings::Instance()->Tag("autoopen-chatlog"));

		_presence->SetEnabled(true);

		if (user->HaveSubscriptionTo()) {
			_subscribe_presence->SetEnabled(false);
			_unsubscribe_presence->SetEnabled(true);
		} else {
			_subscribe_presence->SetEnabled(true);
			_unsubscribe_presence->SetEnabled(false);
		}
	} else {		
		// if not
		_chat_item->SetEnabled(false);
		_message_item->SetEnabled(false);

		sprintf(buffer, "Edit Buddy");
		_change_user_item->SetLabel(buffer);
		_change_user_item->SetEnabled(false);

		sprintf(buffer, "Remove Buddy");
		_remove_user_item->SetLabel(buffer);
		_remove_user_item->SetEnabled(false);

		_user_info_item->SetEnabled(false);
		_user_chatlog_item->SetEnabled(false);

		_presence->SetEnabled(false);
	}
}

void RosterView::UpdateRoster() {
	JRoster *roster = JRoster::Instance();

	// add entries from JRoster that are not in RosterView
	roster->Lock();

	for (JRoster::ConstRosterIter i = roster->BeginIterator(); i != roster->EndIterator(); ++i) {
		if ((*i)->IsUser() && FindUser(*i) < 0) {
			// this entry does not exist in the RosterView
			LinkUser(*i);
		} else if ((*i)->UserType() == UserID::TRANSPORT && FindTransport(*i) < 0 && (*i)->OnlineStatus() == UserID::TRANSPORT_ONLINE) {
			LinkTransport(*i);
		}
	}

	// adjust online status of users
	RESET:
	for (int i = 0; i < FullListCountItems(); ++i) {
		RosterItem *item = dynamic_cast<RosterItem *>(FullListItemAt(i));
		TransportItem *transport_item = dynamic_cast<TransportItem *>(FullListItemAt(i));
		
		// skip illegal entries
		if (item == NULL && transport_item == NULL) {
			continue;
		}

		if (item) {
			// process removals
			if (!roster->ExistingUserObject(item->GetUserID()) || !roster->FindUser(item->GetUserID())) {
				item->SetStalePointer(true);
				RemoveItem(i);

				goto RESET;
			}
		
			// change of statuses
			if (item->GetUserID()->OnlineStatus() != _item_to_status_map[Superitem(item)]) {
				UserID::online_status old_status = _item_to_status_map[Superitem(item)];
				
				// remove the item from the current superitem...
				RemoveItem(i);
			
				// and add it to the appropriate one
				AddUnder(item, _status_to_item_map[item->GetUserID()->OnlineStatus()]);

				// sound effect? :)
				if (item->GetUserID()->OnlineStatus() == UserID::ONLINE && item->GetUserID()->IsUser() && old_status == UserID::OFFLINE) {
					SoundSystem::Instance()->PlayUserOnlineSound();
				} else if (item->GetUserID()->OnlineStatus() == UserID::OFFLINE && item->GetUserID()->IsUser() && old_status == UserID::ONLINE) {
					SoundSystem::Instance()->PlayUserOfflineSound();
				}
								
				goto RESET;
			}			

			// clean it
			InvalidateItem(i);
		} else if (transport_item) {
			if (!roster->ExistingUserObject(transport_item->GetUserID()) || !roster->FindUser(transport_item->GetUserID()) || transport_item->GetUserID()->OnlineStatus() != UserID::TRANSPORT_ONLINE) {
				transport_item->SetStalePointer(true);
				RemoveItem(i);

				goto RESET;
			}

			// change of statuses
			if (transport_item->GetUserID()->OnlineStatus() != _item_to_status_map[Superitem(transport_item)]) {
				// remove the item from the current superitem...
				RemoveItem(i);
			
				// and add it to the appropriate one
				if (transport_item->GetUserID()->OnlineStatus() == UserID::TRANSPORT_ONLINE) {
					AddUnder(transport_item, _status_to_item_map[transport_item->GetUserID()->OnlineStatus()]);
				}

				// sound effect? :)
				if (transport_item->GetUserID()->OnlineStatus() == UserID::TRANSPORT_ONLINE) {
					SoundSystem::Instance()->PlayUserOnlineSound();
				} else if (transport_item->GetUserID()->OnlineStatus() == UserID::OFFLINE) {
					SoundSystem::Instance()->PlayUserOfflineSound();
				}
								
				goto RESET;
			}			

			// clean it
			InvalidateItem(i);
		}
	}

	roster->Unlock();
}
