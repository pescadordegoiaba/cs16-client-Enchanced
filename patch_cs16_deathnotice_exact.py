#!/usr/bin/env python3
# patch_cs16_deathnotice_exact.py
#
# Patch específico para o death.cpp enviado:
#   cl_dll/death.cpp
#
# Uso:
#   cd /home/gullin/claude/cs16-client-Enchanced
#   python3 /mnt/data/patch_cs16_deathnotice_exact.py

from pathlib import Path
import re
import shutil
import sys

ROOT = Path.cwd()
death = ROOT / "cl_dll" / "death.cpp"

if not death.exists():
    print("[ERRO] Não achei cl_dll/death.cpp.")
    print("Rode da raiz do cs16-client-Enchanced:")
    print("  cd /home/gullin/claude/cs16-client-Enchanced")
    print("  python3 /mnt/data/patch_cs16_deathnotice_exact.py")
    sys.exit(1)

src = death.read_text(encoding="utf-8", errors="replace")
backup = death.with_suffix(".cpp.before_deathnotice_exact.bak")
if not backup.exists():
    shutil.copy2(death, backup)
    print(f"[*] Backup criado: {backup}")

# 1) Helpers defensivos após rgDeathNoticeList.
helpers = r'''
// deathnotice_safe: valida DeathMsg vindo de servidores custom/Steam p48.
static inline bool DN_IsValidPlayerIndex( int idx )
{
	return idx >= 1 && idx <= MAX_PLAYERS;
}

static inline bool DN_IsNonPlayerVictim( int idx )
{
	return idx == 255 || ((signed char)idx) == -1;
}

static inline int DN_PlayerIndexOrZero( int idx )
{
	return DN_IsValidPlayerIndex( idx ) ? idx : 0;
}

static inline const char *DN_SafeString( const char *s )
{
	return s ? s : "";
}

static inline void DN_BuildIconName( char *dst, size_t dstSize, const char *weaponName )
{
	if( !dst || dstSize < 3 )
		return;

	dst[0] = 0;
	strncpy( dst, "d_", dstSize - 1 );
	dst[dstSize - 1] = 0;

	strncat( dst, DN_SafeString( weaponName ), dstSize - strlen( dst ) - 1 );
	dst[dstSize - 1] = 0;
}
'''

if "DN_IsValidPlayerIndex" not in src:
    needle = "DeathNoticeItem rgDeathNoticeList[ MAX_DEATHNOTICES + 1 ];"
    if needle not in src:
        print("[ERRO] Não achei rgDeathNoticeList para inserir helpers.")
        sys.exit(2)
    src = src.replace(needle, needle + "\n" + helpers, 1)
    print("[*] Helpers defensivos inseridos.")
else:
    print("[*] Helpers defensivos já estavam aplicados.")

# 2) Patch no Draw para não usar id de sprite inválido/headshot inválido.
draw_old = '''			int id = (rgDeathNoticeList[i].iId == -1) ? m_HUD_d_skull : rgDeathNoticeList[i].iId;
			x = ScreenWidth - DrawUtils::ConsoleStringLen(rgDeathNoticeList[i].szVictim) - (gHUD.GetSpriteRect(id).Width());
			if( rgDeathNoticeList[i].iHeadShotId )
				x -= gHUD.GetSpriteRect(m_HUD_d_headshot).Width();'''

draw_new = '''			int id = (rgDeathNoticeList[i].iId == -1) ? m_HUD_d_skull : rgDeathNoticeList[i].iId;
			if( id < 0 )
			{
				// deathnotice_safe: sem sprite válido, remove o item em vez de crashar.
				memmove( &rgDeathNoticeList[i], &rgDeathNoticeList[i+1], sizeof(DeathNoticeItem) * (MAX_DEATHNOTICES - i) );
				i--;
				continue;
			}

			x = ScreenWidth - DrawUtils::ConsoleStringLen(rgDeathNoticeList[i].szVictim) - (gHUD.GetSpriteRect(id).Width());
			if( rgDeathNoticeList[i].iHeadShotId && m_HUD_d_headshot >= 0 )
				x -= gHUD.GetSpriteRect(m_HUD_d_headshot).Width();'''

if "deathnotice_safe: sem sprite válido" not in src:
    if draw_old not in src:
        print("[AVISO] Bloco Draw/id exato não encontrado; pulando patch de Draw inicial.")
    else:
        src = src.replace(draw_old, draw_new, 1)
        print("[*] Draw: validação de id de sprite aplicada.")
else:
    print("[*] Draw: validação de id de sprite já aplicada.")

src = src.replace(
    "if( rgDeathNoticeList[i].iHeadShotId)\n\t\t\t{",
    "if( rgDeathNoticeList[i].iHeadShotId && m_HUD_d_headshot >= 0 )\n\t\t\t{"
)
src = src.replace(
    "if( rgDeathNoticeList[i].iHeadShotId)\r\n\t\t\t{",
    "if( rgDeathNoticeList[i].iHeadShotId && m_HUD_d_headshot >= 0 )\r\n\t\t\t{"
)

# 3) Substitui a função MsgFunc_DeathMsg inteira por versão segura.
safe_func = r'''// This message handler may be better off elsewhere
int CHudDeathNotice :: MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf )
{
	m_iFlags |= HUD_DRAW;

	BufferReader reader( pszName, pbuf, iSize );

	const int rawKiller = reader.ReadByte();
	const int rawVictim = reader.ReadByte();
	const int headshot = reader.ReadByte();

	const bool victimIsNonPlayer = DN_IsNonPlayerVictim( rawVictim );
	const int killer = DN_PlayerIndexOrZero( rawKiller );
	const int victim = victimIsNonPlayer ? rawVictim : DN_PlayerIndexOrZero( rawVictim );
	const int victimForHud = victimIsNonPlayer ? 0 : victim;

	char killedwith[64];
	DN_BuildIconName( killedwith, sizeof( killedwith ), reader.ReadString() );

	if( killer != rawKiller || (!victimIsNonPlayer && victim != rawVictim) )
	{
		Con_DPrintf( "DeathMsg: indices fora do intervalo rawKiller=%d rawVictim=%d -> killer=%d victim=%d\n",
			rawKiller, rawVictim, killer, victimForHud );
	}

	// Evita passar 255/-1 ou índices > MAX_PLAYERS para HUD/scoreboard.
	gHUD.m_Scoreboard.DeathMsg( killer, victimForHud );
	gHUD.m_Spectator.DeathMessage( victimForHud );

	int i;
	for ( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if ( rgDeathNoticeList[i].iId == 0 )
			break;
	}
	if ( i == MAX_DEATHNOTICES )
	{ // move the rest of the list forward to make room for this item
		memmove( rgDeathNoticeList, rgDeathNoticeList+1, sizeof(DeathNoticeItem) * MAX_DEATHNOTICES );
		i = MAX_DEATHNOTICES - 1;
	}

	// Limpa flags antigas do slot reaproveitado.
	memset( &rgDeathNoticeList[i], 0, sizeof( rgDeathNoticeList[i] ) );

	gHUD.m_Scoreboard.GetAllPlayersInfo();

	// Get the Killer's name
	const char *killer_name = DN_IsValidPlayerIndex( killer ) ? g_PlayerInfoList[ killer ].name : NULL;
	if ( !killer_name )
	{
		killer_name = "";
		rgDeathNoticeList[i].szKiller[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].KillerColor = GetClientColor( killer );
		strncpy( rgDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH-1] = 0;
	}

	// Get the Victim's name
	const char *victim_name = NULL;
	if ( !victimIsNonPlayer && DN_IsValidPlayerIndex( victim ) )
		victim_name = g_PlayerInfoList[ victim ].name;

	if ( !victim_name )
	{
		victim_name = "";
		rgDeathNoticeList[i].szVictim[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].VictimColor = GetClientColor( victim );
		strncpy( rgDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH-1] = 0;
	}

	// Is it a non-player object kill?
	if ( victimIsNonPlayer )
	{
		rgDeathNoticeList[i].bNonPlayerKill = true;

		// Store the object's name in the Victim slot (skip the d_ bit)
		strncpy( rgDeathNoticeList[i].szVictim, killedwith+2, MAX_PLAYER_NAME_LENGTH - 1 );
		rgDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH-1] = 0;
	}
	else
	{
		if ( killer == victim || killer == 0 )
			rgDeathNoticeList[i].bSuicide = true;

		if ( !strncmp( killedwith, "d_teammate", sizeof(killedwith)  ) )
			rgDeathNoticeList[i].bTeamKill = true;
	}

	rgDeathNoticeList[i].iHeadShotId = (headshot != 0 && m_HUD_d_headshot >= 0) ? 1 : 0;

	// Find the sprite in the list
	int spr = gHUD.GetSpriteIndex( killedwith );

	if( spr < 0 )
	{
		Con_DPrintf( "DeathMsg: sprite '%s' não encontrado; usando d_skull/fallback.\n", killedwith );
		spr = m_HUD_d_skull;
	}

	if( spr < 0 )
	{
		// Sem sprite algum: registra no console, mas não tenta desenhar.
		Con_DPrintf( "DeathMsg: nenhum sprite de fallback disponível; death notice ignorado.\n" );
		return 1;
	}

	rgDeathNoticeList[i].iId = spr;
	rgDeathNoticeList[i].flDisplayTime = gHUD.m_flTime + hud_deathnotice_time->value;

	if (rgDeathNoticeList[i].bNonPlayerKill)
	{
		ConsolePrint( rgDeathNoticeList[i].szKiller );
		ConsolePrint( " killed a " );
		ConsolePrint( rgDeathNoticeList[i].szVictim );
		ConsolePrint( "\n" );
	}
	else
	{
		// record the death notice in the console
		if ( rgDeathNoticeList[i].bSuicide )
		{
			ConsolePrint( rgDeathNoticeList[i].szVictim );

			if ( !strncmp( killedwith, "d_world", sizeof(killedwith)  ) )
			{
				ConsolePrint( " died" );
			}
			else
			{
				ConsolePrint( " killed self" );
			}
		}
		else if ( rgDeathNoticeList[i].bTeamKill )
		{
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed his teammate " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}
		else
		{
			if( headshot )
				ConsolePrint( "*** ");
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}

		if ( *killedwith && (*killedwith > 13 ) && strncmp( killedwith, "d_world", sizeof(killedwith) ) && !rgDeathNoticeList[i].bTeamKill )
		{
			if ( headshot )
				ConsolePrint(" with a headshot from ");
			else
				ConsolePrint(" with ");

			ConsolePrint( killedwith+2 ); // skip over the "d_" part
		}

		if( headshot ) ConsolePrint( " ***");
		ConsolePrint( "\n" );
	}

	return 1;
}
'''

pattern = r'// This message handler may be better off elsewhere\s*int CHudDeathNotice\s*::\s*MsgFunc_DeathMsg\s*\(\s*const char \*pszName,\s*int iSize,\s*void \*pbuf\s*\)\s*\{.*?\n\}'
new_src, n = re.subn(pattern, safe_func, src, count=1, flags=re.S)
if n != 1:
    print("[ERRO] Não consegui substituir MsgFunc_DeathMsg. O death.cpp está diferente do esperado.")
    print("Dica: mande também o arquivo atual completo depois dos patches, se houver.")
    sys.exit(3)

src = new_src
death.write_text(src, encoding="utf-8")

print("[OK] cl_dll/death.cpp patchado com segurança para DeathMsg.")
print()
print("Agora compile:")
print("  cd", ROOT)
print("  ./build_cs16_32_i386.sh clean")
print("  ./build_cs16_32_i386.sh")
print()
print("Ache o client.so:")
print("  find build -name client.so -print")
print()
print("Copie para o jogo, ajustando o caminho conforme o find:")
print("  cp -f build/cl_dll/client.so /home/gullin/Games/xash3d-fwgs-linux-i386/cstrike/cl_dlls/client.so")
