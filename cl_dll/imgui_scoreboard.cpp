#include "imgui.h"
#include "imgui_host.h"
#include "imgui_scoreboard.h"
#include "hud.h"
#include "cl_util.h"

static int g_panel = 0;

int ImGuiScore_Active( void )
{
	return gHUD.m_Scoreboard.ShouldDrawScoreboard() ? 1 : 0;
}

static void Row( int idx )
{
	hud_player_info_t *info = &g_PlayerInfoList[idx];
	extra_player_info_t *ex = &g_PlayerExtraInfo[idx];
	const char *status = "";
	char ping[16];

	if( !info->name || !info->name[0] )
		return;

	if( ex->dead )
		status = "morto";
	else if( ex->has_c4 )
		status = "bomba";
	else if( ex->vip )
		status = "vip";
	else if( ex->has_defuse_kit )
		status = "kit";

	if( info->ping <= 5 )
		snprintf( ping, sizeof( ping ), "BOT" );
	else
		snprintf( ping, sizeof( ping ), "%d", info->ping );

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	if( info->thisplayer )
		ImGui::TextColored( ImVec4( 1.0f, 0.85f, 0.4f, 1.0f ), "%s", info->name );
	else
		ImGui::TextUnformatted( info->name );
	ImGui::TableNextColumn();
	ImGui::TextUnformatted( status );
	if( gHUD.m_pShowHealth && gHUD.m_pShowHealth->value && ex->sb_health >= 0 && !ex->dead )
	{
		ImGui::TableNextColumn();
		ImGui::Text( "%d", ex->sb_health );
	}
	else
	{
		ImGui::TableNextColumn();
		ImGui::TextUnformatted( "-" );
	}
	if( gHUD.m_pShowMoney && gHUD.m_pShowMoney->value && ex->sb_account >= 0 )
	{
		ImGui::TableNextColumn();
		ImGui::Text( "$%d", ex->sb_account );
	}
	else
	{
		ImGui::TableNextColumn();
		ImGui::TextUnformatted( "-" );
	}
	ImGui::TableNextColumn();
	ImGui::Text( "%d", ex->frags );
	ImGui::TableNextColumn();
	ImGui::Text( "%d", ex->deaths );
	ImGui::TableNextColumn();
	ImGui::TextUnformatted( ping );
}

static void TeamBlock( const char *team, ImVec4 color )
{
	int i;
	bool any = false;

	for( i = 1; i <= MAX_PLAYERS; ++i )
	{
		if( g_PlayerInfoList[i].name && g_PlayerInfoList[i].name[0] && !stricmp( g_PlayerExtraInfo[i].teamname, team ) )
		{
			any = true;
			break;
		}
	}
	if( !any )
		return;

	ImGui::TextColored( color, "%s", team );
	if( !ImGui::BeginTable( team, 7, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp ) )
		return;
	ImGui::TableSetupColumn( "Jogador" );
	ImGui::TableSetupColumn( "" );
	ImGui::TableSetupColumn( "Vida" );
	ImGui::TableSetupColumn( "$" );
	ImGui::TableSetupColumn( "Abates" );
	ImGui::TableSetupColumn( "Mortes" );
	ImGui::TableSetupColumn( "Ping" );
	ImGui::TableHeadersRow();
	for( i = 1; i <= MAX_PLAYERS; ++i )
	{
		if( !g_PlayerInfoList[i].name || !g_PlayerInfoList[i].name[0] )
			continue;
		if( stricmp( g_PlayerExtraInfo[i].teamname, team ) )
			continue;
		Row( i );
	}
	ImGui::EndTable();
}

static void DrawScore( void * )
{
	if( !ImGuiScore_Active() )
		return;

	gHUD.m_Scoreboard.GetAllPlayersInfo();

	ImGui::SetNextWindowPos( ImVec2( 48.0f, 36.0f ), ImGuiCond_Always );
	ImGui::SetNextWindowSize( ImVec2( ImGui::GetIO().DisplaySize.x - 96.0f, 0.0f ), ImGuiCond_Always );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
	ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.0f, 0.0f, 0.0f, 0.72f ) );
	ImGui::PushStyleColor( ImGuiCol_TableHeaderBg, ImVec4( 0.15f, 0.15f, 0.15f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	if( ImGui::Begin( "##score", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize ) )
	{
		const char *title = gHUD.m_szServerName[0] ? gHUD.m_szServerName : "Counter-Strike";
		ImGui::TextColored( ImVec4( 1.0f, 0.69f, 0.0f, 1.0f ), "%s", title );
		TeamBlock( "TERRORIST", ImVec4( 1.0f, 0.55f, 0.15f, 1.0f ) );
		TeamBlock( "CT", ImVec4( 0.45f, 0.65f, 1.0f, 1.0f ) );
		TeamBlock( "SPECTATOR", ImVec4( 0.75f, 0.75f, 0.75f, 1.0f ) );
	}
	ImGui::End();
	ImGui::PopStyleColor( 3 );
	ImGui::PopStyleVar();
}

void ImGuiScore_Init( void )
{
	if( !g_panel )
		g_panel = ImGui_RegisterPanel( "scoreboard", DrawScore, NULL );
}
