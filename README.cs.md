# Pecislav Sound Mixer

[English README](README.md) · [Zveřejnění na GitHubu](GITHUB-PUBLISH.cs.md)

Používá se jeden aktuální instalační soubor bez čísla v názvu. Další úpravy ho
nahrazují; staré instalační balíčky se nehromadí. Technická metadata sestavení zůstávají interní.
Podrobnosti ke kontrole stability: [CHECKS.cs.md](CHECKS.cs.md).

Nativní plugin pro **OBS Studio 33.0.2, Windows x64**.
Je to skutečný panel v OBS, nikoli webový návrh. Používá zvukové zdroje OBS a
nepřidává další zachytávání zvuku.

## Instalace a spuštění

Spusť `Pecislav-Sound-Mixer-Setup.exe` při vypnutém OBS a potvrď oprávnění Windows.
Instalátor zkontroluje umístění a verzi OBS, zkopíruje plugin a přidá odinstalaci do
Nainstalovaných aplikací ve Windows. Zatím není digitálně podepsaný.
Alternativně rozbal ZIP a otevři `Instalovat.cmd`. Potom spusť OBS a
v nabídce **Panely / Docks** zapni **Zvukový mixér**. Panel lze ukotvit na
místo původního směšovače. Původní panel můžeš kdykoli znovu zapnout.

Skript kopíruje pouze plugin do
`C:\ProgramData\obs-studio\plugins\pecislav-group-mixer`.
Pokud Windows odepře přístup k tomuto adresáři, spusť instalační skript jako
správce. Žádná konfigurace scén se při instalaci nemění.

## Ovládání

- Dole **⋮ → ＋ Nová skupina…** vytvoří vlastní skupinu. Nové zdroje začínají v oblasti **Bez skupiny**.
- Kanál přetáhni za **název** na záhlaví skupiny nebo před jiný kanál.
  Fungují i sbalené skupiny; po přesunu se cílová skupina rozbalí.
- Přes nabídku **Přesunout…** pod kanálem lze totéž udělat bez přetahování.
  Volba **Bez skupiny** kanál vytáhne ven.
- Dole **⋮ → Zobrazení** přepíná režimy: **Svislé kanály** rozbalují skupiny doprava,
  **Kompaktní řádky** dolů. Kompaktní obsah se přizpůsobuje šířce doku.
- Šipka skupinu sbalí. Sbalení nemění hlasitost ani ztlumení.
- Pořadí skupin změníš přetažením jejich záhlaví před jinou skupinu. Přetažení na
  oblast Bez skupiny umístí skupinu na konec. Tato oblast je pevná, není to další skupina.
- Pravé tlačítko na název skupiny otevře posunutí, přejmenování a odstranění. Při odstranění
  skupiny zůstanou všechny její zdroje v oblasti Bez skupiny.
- Ikony reproduktoru a sluchátek ovládají ztlumení a odposlech skutečného zdroje.
- Výrazné **tři tečky vpravo od názvu** / pravé tlačítko nabízí filtry, vlastnosti, přesnou hlasitost,
  pořadí a pokročilé nastavení: synchronizaci, vyvážení, mono a výstupní stopy.
- Spodní **⋮** nabízí také sbalení / rozbalení všech skupin a filtr aktivních zdrojů.
- V kompaktních řádcích přesuneš kanál přes **⋮** u jeho názvu. Měřák i posuvník
  mají společnou šířku pod záhlavím kanálu; spodní rozbalovací nabídka přesunutí je jen ve svislém režimu.
- Kolečko myši hlasitost nemění; jezdec lze dál přetahovat myší.
- Měřáky vždy zobrazují dvě linky. Mono zdroj má obě linky stejné; jeho skutečný zvuk zůstává mono.
- Posuny se jmenují **doleva / doprava** ve svislém mixéru a **nahoru / dolů** v řádcích.
- **Skrýt kanál** v nabídce pouze odstraní kanál z tohoto panelu. Zdroj zůstane
  aktivní a zvuk, hlasitost, odposlech, výstupní stopy i přiřazení do skupiny se nemění.
  Dole přes **Skryté (počet)** zobrazíš jednotlivé kanály nebo všechny najednou.
- Celý název zdroje zobrazí bublina po najetí myší i záhlaví nabídky pravého tlačítka.
- Posuvník je pod bílým jezdcem modrý a nad ním šedý; v řádcích je aktivní část vlevo.
- Při ztlumení kanál přejde do šedé bez překrývajícího nápisu.
- Rozložení, pořadí a skupiny se ukládají do kolekce scén podle UUID zdrojů.
  Skryté kanály i pořadí skupin se ukládají také. Rozložení z verze 0.1.0 se načte automaticky.
  Přejmenování zdroje tedy nezruší jeho zařazení. Stejný zdroj v několika scénách
  má společné nastavení zvuku stejně jako ve standardním směšovači OBS.

## Stav této verze

Fungují živé měřáky, hlasitost, ztlumení, odposlech, přesuny a uložení rozložení.
Skupina umí společné ztlumení; nemá vlastní zvukovou sběrnici, společný posuvník
hlasitosti ani skupinové filtry. Verze zatím nepřidává vlastní klávesové zkratky,
Undo/Redo, vnořené skupiny ani plnou kopii všech nabídek původního mixéru.
Rozhraní přebírá jazyk OBS při načtení pluginu: češtinu, angličtinu, slovenštinu, němčinu a polštinu. Pro ostatní jazyky je anglický základ. Vlastní názvy zdrojů a skupin se nemění.

Změny se ihned projeví i v původním směšovači OBS. Pro jiné hlavní verze OBS nebo
Windows ARM64 je potřeba samostatné sestavení.

## Vývoj a ověření

Zdroje: C++17, Qt 6.11.1, libobs a obs-frontend-api 33.0.2. Zvukové callbacky
pouze zapisují atomické hodnoty; vykreslování probíhá ve vlákně Qt.
Měřáky se obnovují přesným časovačem po 16 ms, používají okamžitý náběh špičky
a časově řízený pokles jako OBS, 300ms integraci VU a podržení špičky. Krátké
špičky mezi snímky se akumulují. Typ měřáku a rychlost poklesu se přebírají
z aktuálního zvukového profilu OBS. Jde o vlastní vykreslení nad skutečnými
daty libobs; cílem je stejné chování, nikoli zaručená pixelová shoda.

Místní sestavení po připravení závislostí: `python tools/build-local.py --smoke`.
Vstupní manifest Visual Studia je z oficiálního kanálu Microsoftu. Skript
`tools/prepare-local.py` stáhne a rozbalí potřebné balíčky do `.deps` a kontroluje
jejich SHA256 podle distribučních manifestů. Nepotřebuje instalovat Visual Studio.
Pro čisté prostředí nejprve spusť `python tools/bootstrap.py`.

Testy ověřují model skupin a následně skutečné libobs s umělými zvukovými zdroji:
posuvník hlasitosti, vnější změnu hlasitosti, ztlumení, přesun přes nabídku,
vytažení ven bez změny zvuku, nativní přetažení skupiny, skrytí/zobrazení bez změny
zvuku, animaci ztlumení, celé názvy, obě rozložení, uložení a odstranění zdroje.
`tools/test-in-obs.py` navíc zkontroluje registraci DLL v samostatném přenosném OBS.
Používá oddělenou konfiguraci a neobsahuje pluginy pro záznam mikrofonu nebo plochy.

Testovací logy a snímky jsou v `test-output` vývojové složky. V instalačním balíčku
nejsou vývojové nástroje, OBS/Qt knihovny ani testovací plugin.

Licence: GNU GPL v2 nebo novější; viz `LICENSE`.
