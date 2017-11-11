#include "torque.h"
#include "discord-rpc.h"

//Discord handlers
static void handleDiscordReady()
{
	Printf("DiscordDLL | Connected to RPC");
}

static void handleDiscordDisconnected(int errcode, const char* message)
{
	Printf("DiscordDLL | Disconnected from RPC (%d: %s)", errcode, message);
}

static void handleDiscordError(int errcode, const char* message)
{
	Printf("DiscordDLL | RPC error event (%d: %s)", errcode, message);
}

static void handleDiscordJoin(const char* secret)
{
	Printf("DiscordDLL | RPC join event (%s)", secret);
}

static void handleDiscordSpectate(const char* secret)
{
	Printf("DiscordDLL | RPC spectate event (%s)", secret);
}

static void handleDiscordJoinRequest(const DiscordJoinRequest* request)
{
	Printf("DiscordDLL | RPC join request event from %s - %s - %s",
		request->username,
		request->avatar,
		request->userId);

	Discord_Respond(request->userId, DISCORD_REPLY_NO);
}

static void discordPresenceInit(DWORD* obj, int argc, const char** argv)
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	handlers.ready = handleDiscordReady;
	handlers.disconnected = handleDiscordDisconnected;
	handlers.errored = handleDiscordError;
	handlers.joinGame = handleDiscordJoin;
	handlers.spectateGame = handleDiscordSpectate;
	handlers.joinRequest = handleDiscordJoinRequest;
	Discord_Initialize("378336355320725516", &handlers, 1, NULL);

	Printf("DiscordDLL | Discord initialized");
}

static void updateDiscordPresence(DWORD* obj, int argc, const char** argv)
{
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.state = argv[1];
	discordPresence.details = argv[2];
	discordPresence.largeImageKey = "logo";
	discordPresence.largeImageText = "Blockland";
	
	if (!_stricmp(argv[3], "true") || !_stricmp(argv[3], "1") || (0 != atoi(argv[3])))
	{
		discordPresence.partyId = argv[6];
		discordPresence.partySize = atoi(argv[4]);
		discordPresence.partyMax = atoi(argv[5]);
		discordPresence.largeImageKey = argv[7];
		discordPresence.largeImageText = argv[7];
	}

	Discord_UpdatePresence(&discordPresence);

	Printf("DiscordDLL | Rich Presence updated");
}

//Setup our stuff
bool Init()
{
	if (!InitTorque())
		return false;

	ConsoleFunction(NULL, "discordPresenceInit", discordPresenceInit,
		"discordInit() - Initializes the Discord RPC connection.", 1, 1);
	ConsoleFunction(NULL, "updateDiscordPresence", updateDiscordPresence,
		"updateDiscordPresence(string details, string state, bool showPlayers, int numPlayers, int maxPlayers) - Updates the Rich Presence with the Discord RPC.", 4, 8);

	Eval("package DiscordDLLPackage { \
		function MM_UpdateDemoDisplay() { \
			Parent::MM_UpdateDemoDisplay(); \
			discordPresenceInit(); \
			updateDiscordPresence(\"In Main Menu\", \"\", 0); \
		} function NewPlayerListGui::UpdateWindowTitle(%this) { \
			Parent::UpdateWindowTitle(%this); \
			%strIP = strreplace(strreplace(ServerConnection.getAddress(), \".\", \"_\"), \":\", \"X\"); \
			%serverInfo = ServerInfoGroup.getObject($ServerSOFromIP[%strIP]); \
			%gameMode = strlwr(strreplace(%serverInfo.map, \" \", \"_\")); \
			updateDiscordPresence(\"In-Game\", $ServerInfo::Name, 1, NPL_List.rowCount(), $ServerInfo::MaxPlayers, %strIP, %gameMode); \
		} \
	}; activatePackage(DiscordDLLPackage);", false, nullptr);

	Printf("DiscordDLL | DLL loaded");

	//We're done here
	return true;
}

bool Detach()
{
	Discord_Shutdown();
	return true;
}

//Entry point
int __stdcall DllMain(HINSTANCE hInstance, unsigned long reason, void *reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		return Init();

	case DLL_PROCESS_DETACH:
		return Detach();

	default:
		return true;
	}
}