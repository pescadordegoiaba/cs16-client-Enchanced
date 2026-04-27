# Nota sobre `steam_login` no cs16-client-Enchanced

A autenticação Steam deve ficar no XASH3D-ENCHANCED, porque é o engine que controla conexão, protocolo e pacote GoldSrc.

O `cs16-client-Enchanced` normalmente não precisa receber o Steam ticket. Use o console do engine:

```txt
steam_login 0   // padrão
steam_login 1   // tenta Steam oficial, se o Xash foi compilado com Steamworks
```

Se futuramente você quiser UX no HUD/Menu, o caminho é só ler/mostrar estado vindo do engine, por exemplo:

- "Conectando via Steam..."
- "SteamAPI_Init falhou"
- "Steam auth recusada pelo servidor"
- "Faltando libsteam_api.so 32-bit"

Não coloque geração de ticket na `cl_dll`; deixe isso no engine.
