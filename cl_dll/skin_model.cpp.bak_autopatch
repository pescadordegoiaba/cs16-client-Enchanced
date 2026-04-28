#include "port.h"
#include "hud.h"
#include "cl_util.h"
#include "skin_model.h"

// cl_util.h define min/max como macros; isso quebra <algorithm> no libstdc++.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <direct.h>
#include <io.h>
#define SKINMODEL_GETCWD _getcwd
#define SKINMODEL_STAT_STRUCT struct _stat
#define SKINMODEL_STAT _stat
#define SKINMODEL_ISDIR(mode) (((mode) & _S_IFDIR) != 0)
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define SKINMODEL_GETCWD getcwd
#define SKINMODEL_STAT_STRUCT struct stat
#define SKINMODEL_STAT stat
#define SKINMODEL_ISDIR(mode) S_ISDIR(mode)
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(p) (sizeof(p) / sizeof((p)[0]))
#endif

struct SkinWeaponAlias
{
	const char *commandName;
	const char *modelName;
};

struct SkinEntry
{
	std::string folder;
	std::string viewModelRel;
	std::string fullPath;
};

struct SkinState
{
	std::vector<SkinEntry> entries;
	int selected;
	bool dirty;

	SkinState() : selected(-1), dirty(true) {}
};

static const SkinWeaponAlias g_SkinWeapons[] =
{
	{ "ak47", "ak47" },
	{ "aug", "aug" },
	{ "awp", "awp" },
	{ "c4", "c4" },
	{ "deagle", "deagle" },
	{ "elite", "elite" },
	{ "famas", "famas" },
	{ "fiveseven", "fiveseven" },
	{ "flashbang", "flashbang" },
	{ "g3sg1", "g3sg1" },
	{ "galil", "galil" },
	{ "glock18", "glock18" },
	{ "hegrenade", "hegrenade" },
	{ "knife", "knife" },
	{ "m249", "m249" },
	{ "m3", "m3" },
	{ "m4a1", "m4a1" },
	{ "mac10", "mac10" },
	{ "mp5", "mp5" },
	{ "mp5navy", "mp5" },
	{ "p228", "p228" },
	{ "p90", "p90" },
	{ "scout", "scout" },
	{ "sg550", "sg550" },
	{ "sg552", "sg552" },
	{ "smokegrenade", "smokegrenade" },
	{ "tmp", "tmp" },
	{ "ump45", "ump45" },
	{ "usp", "usp" },
	{ "xm1014", "xm1014" },
};

static std::map<std::string, SkinState> g_SkinStates;
static std::string g_ResolvedViewModel;

static std::string ToLower(const std::string &s)
{
	std::string out = s;
	for (size_t i = 0; i < out.size(); ++i)
		out[i] = (char)std::tolower((unsigned char)out[i]);
	return out;
}

static bool HasBadPathChars(const std::string &s)
{
	if (s.find("..") != std::string::npos)
		return true;
	if (s.find(':') != std::string::npos)
		return true;
	if (!s.empty() && (s[0] == '/' || s[0] == '\\'))
		return true;
	return false;
}

static bool DirExists(const std::string &path)
{
	SKINMODEL_STAT_STRUCT st;
	return SKINMODEL_STAT(path.c_str(), &st) == 0 && SKINMODEL_ISDIR(st.st_mode);
}

static bool FileExists(const std::string &path)
{
	SKINMODEL_STAT_STRUCT st;
	return SKINMODEL_STAT(path.c_str(), &st) == 0 && !SKINMODEL_ISDIR(st.st_mode);
}

static std::string JoinPath(const std::string &a, const std::string &b)
{
	if (a.empty())
		return b;
	if (a[a.size() - 1] == '/' || a[a.size() - 1] == '\\')
		return a + b;
	return a + "/" + b;
}

static std::string GetGameDirectoryBestEffort()
{
	const char *gameDir = NULL;

	if (gEngfuncs.pfnGetGameDirectory)
		gameDir = gEngfuncs.pfnGetGameDirectory();

	char cwd[1024] = {0};
	SKINMODEL_GETCWD(cwd, sizeof(cwd) - 1);

	std::vector<std::string> candidates;

	if (gameDir && gameDir[0])
	{
		std::string gd = gameDir;
		candidates.push_back(gd);
		if (!gd.empty() && gd[0] != '/' && gd[0] != '\\')
			candidates.push_back(JoinPath(cwd, gd));
	}

	candidates.push_back(JoinPath(cwd, "cstrike"));
	candidates.push_back("cstrike");

	for (size_t i = 0; i < candidates.size(); ++i)
	{
		if (DirExists(candidates[i]))
			return candidates[i];
	}

	return gameDir && gameDir[0] ? std::string(gameDir) : std::string("cstrike");
}

static const char *AliasToModelWeapon(const std::string &cmdWeapon)
{
	std::string low = ToLower(cmdWeapon);
	for (size_t i = 0; i < ARRAYSIZE(g_SkinWeapons); ++i)
	{
		if (low == g_SkinWeapons[i].commandName)
			return g_SkinWeapons[i].modelName;
	}
	return NULL;
}

static bool ExtractCommand(const char *cmd, std::string &weapon, std::string &action)
{
	if (!cmd)
		return false;

	std::string s = ToLower(cmd);
	const std::string prefix = "skin_model_";

	if (s == "skin_model_help")
	{
		action = "help";
		return true;
	}

	if (s.compare(0, prefix.size(), prefix) != 0)
		return false;

	s = s.substr(prefix.size());

	const char *actions[] = { "_list", "_set", "_reset", "_current", "_refresh" };
	for (size_t i = 0; i < ARRAYSIZE(actions); ++i)
	{
		const std::string suffix = actions[i];
		if (s.size() > suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0)
		{
			weapon = s.substr(0, s.size() - suffix.size());
			action = suffix.substr(1);
			return AliasToModelWeapon(weapon) != NULL;
		}
	}

	return false;
}

static std::string ExtractWeaponFromDefaultViewModel(const char *defaultViewModel)
{
	if (!defaultViewModel)
		return std::string();

	std::string s = ToLower(defaultViewModel);
	std::replace(s.begin(), s.end(), '\\', '/');

	size_t slash = s.find_last_of('/');
	std::string file = slash == std::string::npos ? s : s.substr(slash + 1);

	if (file.size() < 7)
		return std::string();
	if (file.compare(0, 2, "v_") != 0)
		return std::string();

	size_t dot = file.rfind(".mdl");
	if (dot == std::string::npos)
		return std::string();

	std::string weapon = file.substr(2, dot - 2);

	if (weapon.size() > 2 && weapon.compare(weapon.size() - 2, 2, "_r") == 0)
		weapon = weapon.substr(0, weapon.size() - 2);

	return weapon;
}

static bool ScanSkins(const std::string &modelWeapon, SkinState &state, bool printErrors)
{
	state.entries.clear();
	state.dirty = false;

	if (HasBadPathChars(modelWeapon))
	{
		if (printErrors)
			gEngfuncs.Con_Printf("[skin_model] nome de arma invalido: %s\n", modelWeapon.c_str());
		return false;
	}

	const std::string gameDir = GetGameDirectoryBestEffort();
	const std::string baseRel = "models/mod/" + modelWeapon;
	const std::string baseAbs = JoinPath(gameDir, baseRel);

	if (!DirExists(baseAbs))
	{
		if (printErrors)
		{
			gEngfuncs.Con_Printf("[skin_model] pasta nao existe: %s\n", baseAbs.c_str());
			gEngfuncs.Con_Printf("[skin_model] crie: cstrike/models/mod/%s/<skin>/v_%s.mdl\n",
				modelWeapon.c_str(), modelWeapon.c_str());
		}
		return false;
	}

#if defined(_WIN32)
	struct _finddata_t fd;
	std::string mask = JoinPath(baseAbs, "*");
	intptr_t h = _findfirst(mask.c_str(), &fd);
	if (h == -1)
		return false;
	do
	{
		if (!(fd.attrib & _A_SUBDIR))
			continue;
		std::string folder = fd.name;
		if (folder == "." || folder == "..")
			continue;
#else
	DIR *dir = opendir(baseAbs.c_str());
	if (!dir)
		return false;

	struct dirent *ent = NULL;
	while ((ent = readdir(dir)) != NULL)
	{
		std::string folder = ent->d_name;
		if (folder == "." || folder == "..")
			continue;

		const std::string folderAbs = JoinPath(baseAbs, folder);
		if (!DirExists(folderAbs))
			continue;
#endif

		if (HasBadPathChars(folder))
			continue;

		SkinEntry e;
		e.folder = folder;
		e.viewModelRel = "models/mod/" + modelWeapon + "/" + folder + "/v_" + modelWeapon + ".mdl";
		e.fullPath = JoinPath(gameDir, e.viewModelRel);

		if (FileExists(e.fullPath))
			state.entries.push_back(e);

#if defined(_WIN32)
	}
	while (_findnext(h, &fd) == 0);
	_findclose(h);
#else
	}
	closedir(dir);
#endif

	std::sort(state.entries.begin(), state.entries.end(),
		[](const SkinEntry &a, const SkinEntry &b) { return ToLower(a.folder) < ToLower(b.folder); });

	return !state.entries.empty();
}

static SkinState &GetState(const std::string &modelWeapon)
{
	SkinState &state = g_SkinStates[modelWeapon];
	if (state.dirty)
		ScanSkins(modelWeapon, state, false);
	return state;
}

static void PrintList(const std::string &modelWeapon)
{
	SkinState &state = g_SkinStates[modelWeapon];
	ScanSkins(modelWeapon, state, true);

	gEngfuncs.Con_Printf("\n[skin_model] skins para %s:\n", modelWeapon.c_str());

	if (state.entries.empty())
	{
		gEngfuncs.Con_Printf("  nenhuma skin valida encontrada.\n");
		gEngfuncs.Con_Printf("  esperado: cstrike/models/mod/%s/<skin>/v_%s.mdl\n\n",
			modelWeapon.c_str(), modelWeapon.c_str());
		return;
	}

	for (size_t i = 0; i < state.entries.size(); ++i)
	{
		const bool active = (state.selected == (int)i);
		gEngfuncs.Con_Printf("  %2d. %s%s\n",
			(int)i + 1,
			state.entries[i].folder.c_str(),
			active ? "  [ativa]" : "");
	}

	gEngfuncs.Con_Printf("use: skin_model_%s_set <numero>\n\n", modelWeapon.c_str());
}

static void PrintCurrent(const std::string &modelWeapon)
{
	SkinState &state = GetState(modelWeapon);

	if (state.selected < 0 || state.selected >= (int)state.entries.size())
	{
		gEngfuncs.Con_Printf("[skin_model] %s: legacy/padrao\n", modelWeapon.c_str());
		return;
	}

	const SkinEntry &e = state.entries[state.selected];
	gEngfuncs.Con_Printf("[skin_model] %s: #%d %s -> %s\n",
		modelWeapon.c_str(), state.selected + 1, e.folder.c_str(), e.viewModelRel.c_str());
}

static void SetSkinByNumber(const std::string &modelWeapon, int number)
{
	SkinState &state = g_SkinStates[modelWeapon];
	ScanSkins(modelWeapon, state, true);

	if (number <= 0 || number > (int)state.entries.size())
	{
		gEngfuncs.Con_Printf("[skin_model] indice invalido: %d. Use skin_model_%s_list\n",
			number, modelWeapon.c_str());
		return;
	}

	state.selected = number - 1;

	const SkinEntry &e = state.entries[state.selected];
	gEngfuncs.Con_Printf("[skin_model] %s -> %s\n", modelWeapon.c_str(), e.folder.c_str());
	gEngfuncs.Con_Printf("[skin_model] viewmodel: %s\n", e.viewModelRel.c_str());

	SkinModel_ClientApplyActiveViewModel();
}

static void ResetSkin(const std::string &modelWeapon)
{
	SkinState &state = g_SkinStates[modelWeapon];
	state.selected = -1;
	gEngfuncs.Con_Printf("[skin_model] %s voltou para legacy/padrao\n", modelWeapon.c_str());

	SkinModel_ClientApplyActiveViewModel();
}

static void PrintHelp()
{
	gEngfuncs.Con_Printf("\nSistema de skins locais\n");
	gEngfuncs.Con_Printf("comandos:\n");
	gEngfuncs.Con_Printf("  skin_model_ak47_list\n");
	gEngfuncs.Con_Printf("  skin_model_ak47_set 3\n");
	gEngfuncs.Con_Printf("  skin_model_ak47_current\n");
	gEngfuncs.Con_Printf("  skin_model_ak47_reset\n");
	gEngfuncs.Con_Printf("  skin_model_ak47_refresh\n\n");
	gEngfuncs.Con_Printf("layout:\n");
	gEngfuncs.Con_Printf("  cstrike/models/mod/<arma>/<skin>/v_<arma>.mdl\n");
	gEngfuncs.Con_Printf("  cstrike/models/mod/<arma>/<skin>/p_<arma>.mdl  opcional/futuro\n");
	gEngfuncs.Con_Printf("  cstrike/models/mod/<arma>/<skin>/w_<arma>.mdl  opcional/futuro\n");
	gEngfuncs.Con_Printf("  cstrike/sprites/mod/<arma>/<skin>/...          armazenado, nao troca HUD ainda\n\n");
	gEngfuncs.Con_Printf("armas registradas:\n  ");
	for (size_t i = 0; i < ARRAYSIZE(g_SkinWeapons); ++i)
		gEngfuncs.Con_Printf("%s%s", g_SkinWeapons[i].commandName, (i + 1) % 10 == 0 ? "\n  " : " ");
	gEngfuncs.Con_Printf("\n\n");
}

static void SkinModel_Command()
{
	std::string cmdWeapon, action;

	if (!ExtractCommand(gEngfuncs.Cmd_Argv(0), cmdWeapon, action))
	{
		PrintHelp();
		return;
	}

	if (action == "help")
	{
		PrintHelp();
		return;
	}

	const char *modelWeaponC = AliasToModelWeapon(cmdWeapon);
	if (!modelWeaponC)
	{
		gEngfuncs.Con_Printf("[skin_model] arma desconhecida: %s\n", cmdWeapon.c_str());
		return;
	}

	const std::string modelWeapon = modelWeaponC;

	if (action == "list")
	{
		PrintList(modelWeapon);
		return;
	}

	if (action == "current")
	{
		PrintCurrent(modelWeapon);
		return;
	}

	if (action == "reset")
	{
		ResetSkin(modelWeapon);
		return;
	}

	if (action == "refresh")
	{
		g_SkinStates[modelWeapon].dirty = true;
		ScanSkins(modelWeapon, g_SkinStates[modelWeapon], true);
		PrintList(modelWeapon);
		return;
	}

	if (action == "set")
	{
		if (gEngfuncs.Cmd_Argc() < 2)
		{
			gEngfuncs.Con_Printf("uso: skin_model_%s_set <numero>\n", modelWeapon.c_str());
			return;
		}

		SetSkinByNumber(modelWeapon, std::atoi(gEngfuncs.Cmd_Argv(1)));
		return;
	}

	PrintHelp();
}

void SkinModel_RegisterCommands()
{
	static bool s_SkinModelCommandsRegistered = false;
	if( s_SkinModelCommandsRegistered )
		return;
	s_SkinModelCommandsRegistered = true;

	for (size_t i = 0; i < ARRAYSIZE(g_SkinWeapons); ++i)
	{
		char command[128];

		std::snprintf(command, sizeof(command), "skin_model_%s_list", g_SkinWeapons[i].commandName);
		gEngfuncs.pfnAddCommand(command, SkinModel_Command);

		std::snprintf(command, sizeof(command), "skin_model_%s_set", g_SkinWeapons[i].commandName);
		gEngfuncs.pfnAddCommand(command, SkinModel_Command);

		std::snprintf(command, sizeof(command), "skin_model_%s_reset", g_SkinWeapons[i].commandName);
		gEngfuncs.pfnAddCommand(command, SkinModel_Command);

		std::snprintf(command, sizeof(command), "skin_model_%s_current", g_SkinWeapons[i].commandName);
		gEngfuncs.pfnAddCommand(command, SkinModel_Command);

		std::snprintf(command, sizeof(command), "skin_model_%s_refresh", g_SkinWeapons[i].commandName);
		gEngfuncs.pfnAddCommand(command, SkinModel_Command);
	}

	gEngfuncs.pfnAddCommand("skin_model_help", SkinModel_Command);
}

const char *SkinModel_ResolveViewModel(const char *defaultViewModel)
{
	g_ResolvedViewModel.clear();

	const std::string weapon = ExtractWeaponFromDefaultViewModel(defaultViewModel);
	if (weapon.empty())
		return defaultViewModel;

	SkinState &state = GetState(weapon);

	if (state.selected < 0 || state.selected >= (int)state.entries.size())
		return defaultViewModel;

	g_ResolvedViewModel = state.entries[state.selected].viewModelRel;
	return g_ResolvedViewModel.c_str();
}
