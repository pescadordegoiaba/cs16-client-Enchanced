#include "imgui.h"
#include "imgui_host.h"
#include "imgui_menu.h"
#include "hud.h"
#include "cl_util.h"
#include "keydefs.h"

enum
{
	PAGE_ROOT = 0,
	PAGE_PISTOL,
	PAGE_SHOTGUN,
	PAGE_SMG,
	PAGE_RIFLE,
	PAGE_MG,
	PAGE_AMMO,
	PAGE_ITEM
};

struct Item
{
	const char *label;
	const char *command;
	int price;
	int slot;
};

static int g_kind = 0;
static int g_page = PAGE_ROOT;
static int g_bits = 0;
static int g_panel = 0;

static int IsCT( void )
{
	int n = gHUD.m_Scoreboard.m_iPlayerNum;
	if( n < 0 || n > MAX_PLAYERS )
		return 1;
	return g_PlayerExtraInfo[n].teamnumber == TEAM_CT;
}

static int Money( void )
{
	if( !gHUD.cscl_currentmoney )
		return 0;
	return (int)gHUD.cscl_currentmoney->value;
}

static int SlotOn( int slot )
{
	int bit;
	if( g_bits == 0 )
		return 1;
	bit = ( slot == 0 ) ? 9 : slot - 1;
	if( bit < 0 || bit > 15 )
		return 0;
	return ( g_bits & ( 1 << bit ) ) != 0;
}

static void Buy( const char *command )
{
	char buf[64];
	if( !command || !command[0] )
		return;
	snprintf( buf, sizeof( buf ), "%s\n", command );
	gEngfuncs.pfnClientCmd( buf );
}

static void CloseBuy( void )
{
	g_kind = 0;
	g_page = PAGE_ROOT;
	ImGui_SetMenuOpen( 0 );
	gEngfuncs.pfnClientCmd( "client_buy_close\n" );
}

static void CloseTeam( void )
{
	g_kind = 0;
	ImGui_SetMenuOpen( 0 );
	gEngfuncs.pfnClientCmd( "slot10\n" );
}

static bool BeginHudMenu( const char *id, const char *title )
{
	ImGui::SetNextWindowPos( ImVec2( 20.0f, 48.0f ), ImGuiCond_Always );
	ImGui::SetNextWindowSize( ImVec2( 360.0f, 0.0f ), ImGuiCond_Always );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 10.0f, 8.0f ) );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0.0f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.0f, 0.0f, 0.0f, 0.45f ) );
	ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_Header, ImVec4( 1.0f, 0.69f, 0.0f, 0.28f ) );
	ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4( 1.0f, 0.69f, 0.0f, 0.45f ) );
	ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImVec4( 1.0f, 0.55f, 0.0f, 0.65f ) );
	bool open = ImGui::Begin( id, NULL,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar );
	if( open )
	{
		ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 0.69f, 0.0f, 1.0f ) );
		ImGui::TextUnformatted( title );
		ImGui::PopStyleColor();
	}
	return open;
}

static void EndHudMenu( void )
{
	ImGui::End();
	ImGui::PopStyleColor( 5 );
	ImGui::PopStyleVar( 4 );
}

static bool HudSlot( int slot, const char *label, int price, bool enabled )
{
	char line[96];
	if( price > 0 )
		snprintf( line, sizeof( line ), "%d.  %s    $%d", slot, label, price );
	else if( slot == 0 )
		snprintf( line, sizeof( line ), "0.  %s", label );
	else
		snprintf( line, sizeof( line ), "%d.  %s", slot, label );

	if( !enabled )
		ImGui::BeginDisabled();
	bool hit = ImGui::Selectable( line );
	if( !enabled )
		ImGui::EndDisabled();
	return hit && enabled;
}

static void DrawItemButton( const Item &item )
{
	int money = Money();
	bool poor = item.price > 0 && money < item.price;
	if( HudSlot( item.slot, item.label, item.price, !poor ) )
		Buy( item.command );
}

static void DrawList( const Item *items, int count )
{
	for( int i = 0; i < count; ++i )
	{
		if( items[i].slot >= 0 && !SlotOn( items[i].slot ) )
			continue;
		DrawItemButton( items[i] );
	}
}

static void DrawBuy( void )
{
	int ct = IsCT();
	if( !BeginHudMenu( "##buy", "Buy" ) )
	{
		EndHudMenu();
		return;
	}

	ImGui::Text( "Dinheiro  $%d", Money() );

	if( g_page == PAGE_ROOT )
	{
		if( HudSlot( 1, "Pistola", 0, true ) ) g_page = PAGE_PISTOL;
		if( HudSlot( 2, "Escopeta", 0, true ) ) g_page = PAGE_SHOTGUN;
		if( HudSlot( 3, "Submetralhadora", 0, true ) ) g_page = PAGE_SMG;
		if( HudSlot( 4, "Rifle", 0, true ) ) g_page = PAGE_RIFLE;
		if( HudSlot( 5, "Metralhadora", 0, true ) ) g_page = PAGE_MG;
		if( HudSlot( 6, "Municao primaria", 0, true ) ) Buy( "primammo" );
		if( HudSlot( 7, "Municao secundaria", 0, true ) ) Buy( "secammo" );
		if( HudSlot( 8, "Equipamento", 0, true ) ) g_page = PAGE_ITEM;
		if( HudSlot( 0, "Sair", 0, true ) ) CloseBuy();
	}
	else if( g_page == PAGE_PISTOL )
	{
		Item items[] = {
			{ "Glock 18", "glock", 400, 1 },
			{ "USP", "usp", 500, 2 },
			{ "P228", "p228", 600, 3 },
			{ "Desert Eagle", "deagle", 650, 4 },
			{ ct ? "Five-Seven" : "Dual Elites", ct ? "fn57" : "elites", ct ? 750 : 800, 5 }
		};
		DrawList( items, 5 );
	}
	else if( g_page == PAGE_SHOTGUN )
	{
		Item items[] = {
			{ "M3 Super 90", "m3", 1700, 1 },
			{ "XM1014", "xm1014", 3000, 2 }
		};
		DrawList( items, 2 );
	}
	else if( g_page == PAGE_SMG )
	{
		Item items[] = {
			{ ct ? "TMP" : "MAC-10", ct ? "tmp" : "mac10", ct ? 1250 : 1400, 1 },
			{ "MP5", "mp5", 1500, 2 },
			{ "UMP-45", "ump45", 1700, 3 },
			{ "P90", "p90", 2350, 4 }
		};
		DrawList( items, 4 );
	}
	else if( g_page == PAGE_RIFLE )
	{
		if( ct )
		{
			Item items[] = {
				{ "FAMAS", "famas", 2250, 1 },
				{ "Scout", "scout", 2750, 2 },
				{ "M4A1", "m4a1", 3100, 3 },
				{ "AUG", "aug", 3500, 4 },
				{ "SG550", "sg550", 4200, 5 },
				{ "AWP", "awp", 4750, 6 }
			};
			DrawList( items, 6 );
		}
		else
		{
			Item items[] = {
				{ "Galil", "galil", 2000, 1 },
				{ "AK-47", "ak47", 2500, 2 },
				{ "Scout", "scout", 2750, 3 },
				{ "SG552", "sg552", 3500, 4 },
				{ "AWP", "awp", 4750, 5 },
				{ "G3SG1", "g3sg1", 5000, 6 }
			};
			DrawList( items, 6 );
		}
	}
	else if( g_page == PAGE_MG )
	{
		Item items[] = { { "M249", "m249", 5750, 1 } };
		DrawList( items, 1 );
	}
	else if( g_page == PAGE_AMMO )
	{
		Item items[] = {
			{ "Municao primaria", "primammo", 0, 6 },
			{ "Municao secundaria", "secammo", 0, 7 }
		};
		DrawList( items, 2 );
	}
	else if( g_page == PAGE_ITEM )
	{
		Item common[] = {
			{ "Colete", "vest", 650, 1 },
			{ "Colete e capacete", "vesthelm", 1000, 2 },
			{ "Flashbang", "flash", 200, 3 },
			{ "Granada HE", "hegren", 300, 4 },
			{ "Fumaca", "sgren", 300, 5 },
			{ "Visao noturna", "nvgs", 1250, 6 }
		};
		DrawList( common, 6 );
		if( ct )
		{
			Item extra[] = {
				{ "Kit de desarmamento", "defuser", 200, 7 },
				{ "Escudo", "shield", 2200, 8 }
			};
			DrawList( extra, 2 );
		}
	}

	if( g_page != PAGE_ROOT && HudSlot( 0, "Voltar", 0, true ) )
		g_page = PAGE_ROOT;
	EndHudMenu();
}

static void TeamButton( const char *label, int slot )
{
	char cmd[32];
	if( !SlotOn( slot ) )
		return;
	snprintf( cmd, sizeof( cmd ), "jointeam %d", slot );
	if( HudSlot( slot, label, 0, true ) )
	{
		Buy( cmd );
		g_kind = 0;
		ImGui_SetMenuOpen( 0 );
	}
}

static void DrawTeam( void )
{
	if( !BeginHudMenu( "##team", "Escolher time" ) )
	{
		EndHudMenu();
		return;
	}
	TeamButton( "Terroristas", 1 );
	TeamButton( "Contra-terroristas", 2 );
	TeamButton( "VIP", 3 );
	TeamButton( "Automatico", 5 );
	TeamButton( "Espectador", 6 );
	if( HudSlot( 0, "Cancelar", 0, true ) )
		CloseTeam();
	EndHudMenu();
}

static void ClassButton( const char *label, int slot )
{
	char cmd[32];
	if( !SlotOn( slot ) )
		return;
	snprintf( cmd, sizeof( cmd ), "joinclass %d", slot );
	if( HudSlot( slot, label, 0, true ) )
	{
		Buy( cmd );
		g_kind = 0;
		ImGui_SetMenuOpen( 0 );
	}
}

static void DrawClass( int ct )
{
	if( !BeginHudMenu( "##class", "Escolher classe" ) )
	{
		EndHudMenu();
		return;
	}
	if( ct )
	{
		ClassButton( "Urban", 1 );
		ClassButton( "GSG-9", 2 );
		ClassButton( "SAS", 3 );
		ClassButton( "GIGN", 4 );
	}
	else
	{
		ClassButton( "Phoenix", 1 );
		ClassButton( "Elite Crew", 2 );
		ClassButton( "Arctic", 3 );
		ClassButton( "Guerilla", 4 );
	}
	ClassButton( "Automatico", 5 );
	if( HudSlot( 0, "Cancelar", 0, true ) )
		CloseTeam();
	EndHudMenu();
}

static void DrawPanel( void * )
{
	if( g_kind == MENU_BUY || ( g_kind >= MENU_BUY_PISTOL && g_kind <= MENU_BUY_ITEM ) )
		DrawBuy();
	else if( g_kind == MENU_TEAM )
		DrawTeam();
	else if( g_kind == MENU_CLASS_CT )
		DrawClass( 1 );
	else if( g_kind == MENU_CLASS_T )
		DrawClass( 0 );
}

static int PageForMenu( int menuType )
{
	switch( menuType )
	{
	case MENU_BUY_PISTOL: return PAGE_PISTOL;
	case MENU_BUY_SHOTGUN: return PAGE_SHOTGUN;
	case MENU_BUY_SUBMACHINEGUN: return PAGE_SMG;
	case MENU_BUY_RIFLE: return PAGE_RIFLE;
	case MENU_BUY_MACHINEGUN: return PAGE_MG;
	case MENU_BUY_ITEM: return PAGE_ITEM;
	default: return PAGE_ROOT;
	}
}

void ImGuiMenu_Init( void )
{
	if( !g_panel )
		g_panel = ImGui_RegisterPanel( "menus", DrawPanel, NULL );
}

void ImGuiMenu_Open( int menuType, int bits )
{
	g_kind = menuType;
	g_bits = bits;
	g_page = PageForMenu( menuType );
	ImGuiMenu_Init();
	ImGui_SetMenuOpen( 1 );
}

void ImGuiMenu_Close( void )
{
	if( !g_kind )
		return;
	g_kind = 0;
	g_page = PAGE_ROOT;
	ImGui_SetMenuOpen( 0 );
}

int ImGuiMenu_IsOpen( void )
{
	return g_kind != 0;
}

static int SelectSlot( int slot )
{
	if( g_kind == MENU_TEAM )
	{
		char cmd[32];
		if( !SlotOn( slot ) )
			return 1;
		snprintf( cmd, sizeof( cmd ), "jointeam %d", slot );
		Buy( cmd );
		g_kind = 0;
		ImGui_SetMenuOpen( 0 );
		return 1;
	}
	if( g_kind == MENU_CLASS_T || g_kind == MENU_CLASS_CT )
	{
		char cmd[32];
		if( !SlotOn( slot ) )
			return 1;
		snprintf( cmd, sizeof( cmd ), "joinclass %d", slot );
		Buy( cmd );
		g_kind = 0;
		ImGui_SetMenuOpen( 0 );
		return 1;
	}
	if( g_kind == MENU_BUY || ( g_kind >= MENU_BUY_PISTOL && g_kind <= MENU_BUY_ITEM ) )
	{
		if( g_page == PAGE_ROOT )
		{
			switch( slot )
			{
			case 1: g_page = PAGE_PISTOL; break;
			case 2: g_page = PAGE_SHOTGUN; break;
			case 3: g_page = PAGE_SMG; break;
			case 4: g_page = PAGE_RIFLE; break;
			case 5: g_page = PAGE_MG; break;
			case 6: Buy( "primammo" ); break;
			case 7: Buy( "secammo" ); break;
			case 8: g_page = PAGE_ITEM; break;
			case 0: CloseBuy(); break;
			}
			return 1;
		}
		const Item *list = NULL;
		int count = 0;
		int ct = IsCT();
		Item pistols[] = {
			{ "", "glock", 0, 1 }, { "", "usp", 0, 2 }, { "", "p228", 0, 3 },
			{ "", "deagle", 0, 4 }, { "", ct ? "fn57" : "elites", 0, 5 }
		};
		Item shotguns[] = { { "", "m3", 0, 1 }, { "", "xm1014", 0, 2 } };
		Item smg[] = {
			{ "", ct ? "tmp" : "mac10", 0, 1 }, { "", "mp5", 0, 2 },
			{ "", "ump45", 0, 3 }, { "", "p90", 0, 4 }
		};
		Item rifle_ct[] = {
			{ "", "famas", 0, 1 }, { "", "scout", 0, 2 }, { "", "m4a1", 0, 3 },
			{ "", "aug", 0, 4 }, { "", "sg550", 0, 5 }, { "", "awp", 0, 6 }
		};
		Item rifle_t[] = {
			{ "", "galil", 0, 1 }, { "", "ak47", 0, 2 }, { "", "scout", 0, 3 },
			{ "", "sg552", 0, 4 }, { "", "awp", 0, 5 }, { "", "g3sg1", 0, 6 }
		};
		Item mg[] = { { "", "m249", 0, 1 } };
		Item ammo[] = { { "", "primammo", 0, 6 }, { "", "secammo", 0, 7 } };
		Item items[] = {
			{ "", "vest", 0, 1 }, { "", "vesthelm", 0, 2 }, { "", "flash", 0, 3 },
			{ "", "hegren", 0, 4 }, { "", "sgren", 0, 5 }, { "", "nvgs", 0, 6 },
			{ "", "defuser", 0, 7 }, { "", "shield", 0, 8 }
		};
		if( g_page == PAGE_PISTOL ) { list = pistols; count = 5; }
		else if( g_page == PAGE_SHOTGUN ) { list = shotguns; count = 2; }
		else if( g_page == PAGE_SMG ) { list = smg; count = 4; }
		else if( g_page == PAGE_RIFLE ) { list = ct ? rifle_ct : rifle_t; count = 6; }
		else if( g_page == PAGE_MG ) { list = mg; count = 1; }
		else if( g_page == PAGE_AMMO ) { list = ammo; count = 2; }
		else if( g_page == PAGE_ITEM ) { list = items; count = ct ? 8 : 6; }
		if( slot == 0 )
		{
			g_page = PAGE_ROOT;
			return 1;
		}
		for( int i = 0; i < count; ++i )
		{
			if( list[i].slot == slot )
			{
				Buy( list[i].command );
				return 1;
			}
		}
		return 1;
	}
	return 0;
}

int ImGuiMenu_OnKey( int down, int keynum )
{
	int slot = -1;
	if( !g_kind || !down )
		return 0;
	if( keynum == K_ESCAPE || keynum == '0' )
	{
		if( g_kind == MENU_BUY || ( g_kind >= MENU_BUY_PISTOL && g_kind <= MENU_BUY_ITEM ) )
		{
			if( g_page != PAGE_ROOT && keynum == '0' )
				g_page = PAGE_ROOT;
			else
				CloseBuy();
		}
		else
			CloseTeam();
		return 1;
	}
	if( keynum >= '1' && keynum <= '9' )
		slot = keynum - '0';
	if( slot < 0 )
		return 0;
	return SelectSlot( slot );
}
