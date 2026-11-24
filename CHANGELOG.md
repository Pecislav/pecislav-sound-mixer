# Průběžná údržba

- Jeden aktuální instalátor a balíčky bez čísel v názvu.
- Texty rozhraní se načítají přes lokalizaci OBS. Kompletní cs-CZ, en-US, sk-SK,
  de-DE a pl-PL; anglický základ pro ostatní jazyky.
- Ztlumení má pouze šedý vzhled bez překrývajícího štítku.
- Oprava životnosti otevřených nabídek a referencí na zdroje při změně kolekce.
- Starý dialog po přepnutí kolekce neuplatní změny na novou kolekci nebo odstraněný zdroj.
- Názvy zdrojů se vykreslují jako prostý text.

# Dřívější změny

- Panel se jmenuje Zvukový mixér; interní ID zůstává stejné pro zachování rozložení.
- Oblast Bez skupiny vyplní zbytek šířky svislého doku až k pravému okraji.
  Prázdný seznam ukazuje jen Žádné kanály, vysvětlení je v tooltipu.
- Přidán skutečný EXE instalátor s kontrolou OBS 33.x, ochranou před aktualizací
  běžícího OBS a odinstalací přes Nainstalované aplikace Windows.

# 0.2.1

- Nastavení směšovače jsou pod ikonou tří teček. Vedle zůstává nabídka Skryté;
  vlastní šipky Qt jsou skryté, aby se nepřekrývaly s textem.
- Mono i stereo zdroje zobrazují dvě linky. U mono se vizuálně zrcadlí stejná
  hodnota; zpracování a směrování zvuku zůstává beze změny.
- Kolečko myši nemění hlasitost, ani když má posuvník fokus. Přetažení jezdce funguje.
- Kompaktní řádky mají záhlaví, plnošířkový měřák a zarovnaný posuvník.
  Nabídka přesunutí je v tomto režimu pod třemi tečkami u názvu.
- Odstraněna pevná minimální šířka kompaktního obsahu. Šířka se řídí dokem;
  ověřeno v rozsahu 180–840 px bez vodorovného přetečení.

# 0.2.0

- Název kanálu začíná vlevo bez tečkovaného úchytu; přetahuje se přímo za název.
  Vpravo je větší ikona tří teček pro nabídku.
- Nabídky používají směry doleva/doprava nebo nahoru/dolů podle orientace.
- Skupiny lze přesouvat přetažením i přes pravé tlačítko, v obou rozloženích.
- Měřáky mají 16ms přesný časovač, průběžný pokles v dB/s, integraci VU a
  akumulaci krátkých špiček. Nastavení typu a poklesu se přebírá z OBS.
- Aktivní část posuvníku je modrá, zbývající šedá.
- Název zdroje je celý v tooltipu a záhlaví nabídky.
- Kanály lze skrýt bez změny zvuku a obnovit přes nabídku Skryté.
- Ztlumení plynule zešedne celý kanál a zobrazí červený štítek.
- Skryté zdroje a pořadí skupin se ukládají pro každou kolekci; načítání
  nastavení 0.1.0 je zachováno.
