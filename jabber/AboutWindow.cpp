//////////////////////////////////////////////////
// Blabber [AboutWindow.cpp]
//////////////////////////////////////////////////

#include "BlabberApp.h"
#include "AboutWindow.h"
#include "GenericFunctions.h"

AboutWindow *AboutWindow::_instance = NULL;

AboutWindow *AboutWindow::Instance() {
	if (_instance == NULL) {
		// create window singleton
		_instance = new AboutWindow();
	}

	return _instance;
}

AboutWindow::AboutWindow()
	: BAboutWindow("Renga", "application/x-vnd.Haiku-Jabber") {
	// draw credits
	PopulateCredits();
}

AboutWindow::~AboutWindow() {
	_instance = NULL;
}
	
void AboutWindow::PopulateCredits() {
	AddDescription("An XMPP client for Haiku");
	const char * authors[] = {
		"Andrea Anzani",
		"John Blanco",
		"Adrien Destugues",
		"Frank Paul Silye",
		NULL
	};
	AddAuthors(authors);
	const char * special[] = {
		"Michele Blanco",
		NULL
	};
	AddSpecialThanks(special);
	const char * graphics[] = {
		"John Blanco",
		"Daniel Fischer",
		"Michele Frau",
		"jabber.org",
		NULL
	};
	AddText("Graphics", graphics);
	AddExtraInfo("Based on Jabber for BeOS by John Blanco");
	AddExtraInfo("Powered by Gloox by Jakob Schröter");
}

