# Jak projekt zveřejnit na GitHubu

Projekt je připravený ke zveřejnění. Zatím nebyl nahrán do žádného repozitáře.

## Co je pro koho

- **Pro uživatele:** `dist/Pecislav-Sound-Mixer-Windows-x64.zip`.
  ZIP je alternativa pro ruční instalaci. Hlavní instalační soubor je
  `dist/Pecislav-Sound-Mixer-Setup.exe`; ten přilož do GitHub Releases.
- **Pro repozitář:** `dist/Pecislav-Sound-Mixer-github.zip`.
  Rozbal ho a nahraj jeho obsah do kořene repozitáře. Samotný ZIP není náhrada
  za zdrojové soubory v repozitáři.
- **Archiv zdrojů:** `dist/Pecislav-Sound-Mixer-source.zip`.
- **Text vydání:** `RELEASE-NOTES.md`.

Repozitářový balíček obsahuje zdroje, testy, licenci, oba návody, snímky rozložení
a GitHub Actions. Neobsahuje kompilátor, stažené knihovny, OBS, uživatelské profily,
testovací logy ani pracovní cache.

## Přes web GitHubu

1. Založ nový prázdný repozitář, například `obs-group-mixer`. Pro veřejné sdílení
   vyber Public. Nevytvářej další README ani licenci; obojí je v balíčku.
2. Rozbal `Pecislav-Sound-Mixer-github.zip`. Přes **Add file → Upload files**
   nahraj soubory a složky do kořene, včetně `.github` a `.gitignore`.
3. Zkontroluj README a složky `src`, `tools`, `tests` a `.github`.
4. V **Actions → Build Windows plugin → Run workflow** lze spustit sestavení.
   Po dokončení jsou ZIPy dostupné jako workflow artifacts.
5. V **Releases → Draft a new release** vytvoř tag vlastní tag a název
   `Pecislav Sound Mixer`.
6. Popis zkopíruj z `RELEASE-NOTES.md`. Přilož instalátor EXE, instalační ZIP, source ZIP a
   `dist/SHA256SUMS.txt`. Repozitářový ZIP může zůstat jen pro tvou potřebu.
7. Před publikací ověř uvedenou kompatibilitu: OBS 33.0.2, Windows x64, české rozhraní.

## Alternativa přes Git

Spusť v čistém rozbaleném repozitářovém balíčku. Nahraď `TVUJ_UCET` svým účtem;
adresa níže je pouze ukázka.

```powershell
git init
git add .
git commit -m "Pecislav Sound Mixer"
git branch -M main
git remote add origin https://github.com/TVUJ_UCET/obs-group-mixer.git
git push -u origin main
```

Zápis workflow souborů může vyžadovat příslušné oprávnění GitHubu. Workflow
vytváří jen artifacts; samo nepublikuje Releases.
