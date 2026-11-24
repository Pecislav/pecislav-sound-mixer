# Kontrola stability a zvukových nastavení

Kontrola se zaměřuje na chování pluginu a jeho práci s OBS. Nejde o záruku, že
jakákoli chyba nebo konflikt s jiným pluginem je vyloučený.

## Opravená riziková místa

- Nabídky kanálů už nemají za rodiče dočasný ovládací prvek kanálu. Při přepnutí
  kolekce scén se staré ovládací prvky odstraní; otevřená nabídka tím nesmí být
  předčasně zničena a následně znovu použita.
- Otevřená nabídka drží vlastní referenci na zvukový zdroj až do uzavření.
- Akce nabídek a dialogů kontrolují, jestli se mezitím nezměnila kolekce nebo
  nebyl odstraněn zdroj. Starý dialog tak neupraví nastavení v nové kolekci.
- Názvy zdrojů se zobrazují jako prostý text, nikoli jako HTML.

## Ověření

- Model skupin: přesuny, pořadí, odstranění skupiny, zachování kanálů, skrytí,
  uložení a načtení rozložení.
- Skrytí, sbalení a přesun nemění hlasitost, ztlumení ani výstupní stopy.
- Skutečné testovací zdroje libobs: hlasitost, ztlumení, synchronizace vnějších
  změn, odstranění zdroje a přepnutí kolekce.
- Přepnutí kolekce s otevřenou nabídkou kanálu a s otevřenou nabídkou skupiny.
- Kolečko nemění hlasitost, mono měřák je pouze vizuálně zdvojený.
- Překlady cs-CZ, en-US, sk-SK, de-DE a pl-PL přes stejný INI parser jako OBS;
  chybějící text má anglický základ. Názvy vlastních zdrojů a skupin se nepřekládají.
- Ztlumení ponechá jen šedý vzhled bez překrývajícího nápisu.

Testy používají umělé zdroje a oddělené testovací konfigurace. Nezachytávají
mikrofon ani plochu a neupravují běžné uživatelské scény.

Hlasitost, ztlumení, odposlech a pokročilé nastavení jsou skutečné ovládání OBS:
uživatelem potvrzené změny těchto ovladačů mají záměrně vliv na zvuk.
